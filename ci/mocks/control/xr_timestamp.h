/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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
#ifndef _XR_TIMESTAMP_H_
#define _XR_TIMESTAMP_H_

#include <stdint.h>
#include <time.h>

/* Use timespec-compatible layout so ctrlm can assign directly between types. */
typedef struct timespec rdkx_timestamp_t;

#ifdef __cplusplus
extern "C" {
#endif

void             rdkx_timestamp_get(rdkx_timestamp_t *timestamp);
void             rdkx_timestamp_get_realtime(rdkx_timestamp_t *timestamp);
double           rdkx_timestamp_subtract(const rdkx_timestamp_t *a, const rdkx_timestamp_t *b);
signed long long rdkx_timestamp_subtract_us(rdkx_timestamp_t a, rdkx_timestamp_t b);
signed long long rdkx_timestamp_subtract_ms(rdkx_timestamp_t a, rdkx_timestamp_t b);
void             rdkx_timestamp_add_ms(rdkx_timestamp_t *timestamp, unsigned long milliseconds);
int              rdkx_timestamp_cmp(const rdkx_timestamp_t *a, const rdkx_timestamp_t *b);

#ifdef __cplusplus
}
#endif

#endif /* _XR_TIMESTAMP_H_ */
