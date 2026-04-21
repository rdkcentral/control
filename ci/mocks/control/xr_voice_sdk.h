/*
 * Stub for xr_voice_sdk.h
 *
 * Copyright 2026 RDK Management
 * Licensed under the Apache License, Version 2.0
 */

/*
 * CI compatibility header for the small portion of the VSDK API used by ctrlm.
 *
 * The real xr_voice_sdk.h pulls in additional SDK and logger headers, including
 * the installed rdkx_logger surface, which are not self-contained in this
 * reduced native build. This mock keeps only the declarations ctrlm uses.
 */
#ifndef _XR_VOICE_SDK_H_
#define _XR_VOICE_SDK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* xr_voice_sdk API */
int xr_voice_sdk_init(void);
int xr_voice_sdk_term(void);

/* vsdk API used by ctrlm-main */
#define VSDK_VERSION_QTY_MAX (16)

typedef struct {
    const char *name;
    const char *version;
    const char *branch;
    const char *commit_id;
} vsdk_version_info_t;

typedef void (*vsdk_thread_poll_response_t)(void *data);

void vsdk_version(vsdk_version_info_t *info, uint32_t *qty);
bool vsdk_init(bool console, const char *filename, uint32_t file_size_max);
void vsdk_term(void);
void vsdk_thread_poll(vsdk_thread_poll_response_t response, void *data);

#ifdef __cplusplus
}
#endif

#endif /* _XR_VOICE_SDK_H_ */
