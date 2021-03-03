/*
    Copyright 2020-2021 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "stm32h747i_discovery.h"

#include "pv_porcupine_mcu.h"

#include "pv_audio_rec.h"
#include "pv_params.h"
#include "pv_stm32h747.h"

#define MEMORY_BUFFER_SIZE (70 * 1024)

static int8_t memory_buffer[MEMORY_BUFFER_SIZE] __attribute__((aligned(16)));

int32_t num_keywords = 4;
int32_t keyword_model_sizes[] = {
        sizeof(porcupine_keyword_array),
        sizeof(picovoice_keyword_array),
        sizeof(bumblebee_keyword_array),
        sizeof(alexa_keyword_array)
};
const void *keyword_models[] = {
        porcupine_keyword_array,
        picovoice_keyword_array,
        bumblebee_keyword_array,
        alexa_keyword_array
};
static const float sensitivities[] = {
        0.75f,
        0.75f,
        0.75f,
        0.75f
};

const char *keywords_name[] = {
        "Porcupine",
        "Picovoice",
        "Bumblebee",
        "Alexa"
};

static void wake_word_callback(int32_t keyword_index) {
    printf("[wake word] %s\n", keywords_name[keyword_index]);
}

static void error_handler(void) {
    while(true);
}

int main(void) {

    pv_status_t status = pv_board_init();
    if (status != PV_STATUS_SUCCESS) {
     error_handler();
    }

    status = pv_message_init();
    if (status != PV_STATUS_SUCCESS) {
     error_handler();
    }

    const uint8_t *board_uuid = pv_get_uuid();
    printf("UUID: ");
    for (uint32_t i = 0; i < pv_get_uuid_size(); i++) {
        printf(" %.2x", board_uuid[i]);
    }
    printf("\r\n");

    status = pv_audio_rec_init();
    if (status != PV_STATUS_SUCCESS) {
     printf("Audio init failed with '%s'", pv_status_to_string(status));
     error_handler();
    }

    status = pv_audio_rec_start();
    if (status != PV_STATUS_SUCCESS) {
        printf("Recording audio failed with '%s'", pv_status_to_string(status));
        error_handler();
    }

    pv_porcupine_t *handle = NULL;

    status = pv_porcupine_init(
            MEMORY_BUFFER_SIZE,
            memory_buffer,
            num_keywords,
            keyword_model_sizes,
            keyword_models,
            sensitivities,
            &handle);

    if (status != PV_STATUS_SUCCESS) {
        printf("Porcupine init failed with '%s'", pv_status_to_string(status));
        error_handler();
    }

    while (true) {
        const int16_t *buffer = pv_audio_rec_get_new_buffer();
        if (buffer) {
         int32_t keyword_index;
         const pv_status_t status = pv_porcupine_process(handle, buffer, &keyword_index);
            if (status != PV_STATUS_SUCCESS) {
                printf("Porcupine process failed with '%s'", pv_status_to_string(status));
                error_handler();
            }
            if (keyword_index != -1) {
             wake_word_callback(keyword_index);
            }
        }

    }
    pv_board_deinit();
    pv_audio_rec_deinit();
    pv_porcupine_delete(handle);
}
