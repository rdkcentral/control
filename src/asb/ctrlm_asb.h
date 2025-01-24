/*
 * If not stated otherwise in this file or this component's LICENSE file
 * the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CTRLM_ASB_H__
#define __CTRLM_ASB_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief The current asb key derivation methods are stored in 1 byte bitmask as defined in XRC spec
 */
typedef uint8_t asb_key_derivation_bitmask_t;

/**
 * @brief The key derivation method is one bit in a key derivation bitmask
 */
typedef uint8_t asb_key_derivation_method_t;

/* -- API Function Declarations -- */
/**
 * @brief The ASB Library init function.
 *
 * This function MUST be called before calling any other ASB Library Function. The function takes care
 * of retrieving the secrets from the firmware.
 *
 * @return 0 on success, 1 on error.
 */
int                          asb_init();
/**
 * @brief The ASB Library GET API for supported key derivation methods.
 *
 * This function gets the supported key derivation bitmask.
 *
 * @return The bitmask of supported key derivation methods.
 */
asb_key_derivation_bitmask_t asb_key_derivation_methods_get();
/**
 * @brief The ASB Library API for key derivation.
 *
 * This function derives a new key in the output buffer using the requested key derivation method.
 *
 * @return 0 on success, 1 on error.
 */
int                          asb_key_derivation(uint8_t *input, uint8_t *output, asb_key_derivation_method_t method);
/**
 * @brief The ASB Library API destroy.
 *
 * This function writes over the secrets with null data and then cleans up resources.
 *
 */
void                         asb_destroy();

/* -- Defines -- */
#define AES_KEY_LEN             (16)         ///< AES Key Length
#define ASB_KEY_DERIVATION_NONE (0b00000000) ///< Key Derivation Method None bit
#define ASB_KEY_DERIVATION_1    (0b10000000) ///< Key Derivation Method 1 bit

#endif
