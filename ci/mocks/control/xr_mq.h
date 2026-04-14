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
#ifndef _XR_MQ_H_
#define _XR_MQ_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* xr_mq_t is an integer (OS queue descriptor), not a pointer */
typedef int xr_mq_t;

#define XR_MQ_INVALID ((xr_mq_t)-1)

typedef struct {
    uint32_t max_msg;      /* field name used by ctrlm source (must be first — matches designated init order) */
    size_t   max_msg_size;
} xr_mq_attr_t;

xr_mq_t xr_mq_create(const xr_mq_attr_t *attr);
void     xr_mq_destroy(xr_mq_t mq);
bool     xr_mq_push(xr_mq_t mq, const void *msg, size_t msg_size);
bool     xr_mq_pop(xr_mq_t mq, void *msg, size_t msg_size);
bool     xr_mq_pop_timeout(xr_mq_t mq, void *msg, size_t msg_size, int32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* _XR_MQ_H_ */
