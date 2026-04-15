/*
 * Stub for xraudio.h - Audio types
 *
 * Copyright 2026 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _XRAUDIO_H_
#define _XRAUDIO_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XRAUDIO_ENCODING_PCM       = 0,
    XRAUDIO_ENCODING_ADPCM     = 1,
    XRAUDIO_ENCODING_OPUS      = 2,
    XRAUDIO_ENCODING_INVALID   = 3
} xraudio_encoding_t;

typedef struct {
    xraudio_encoding_t type;
} xraudio_encoding_info_t;

typedef struct {
    xraudio_encoding_info_t encoding;
    uint16_t                sample_rate;
    uint8_t                 sample_size;
    uint8_t                 channel_qty;
} xraudio_output_format_t;

typedef enum {
    XRAUDIO_RESULT_OK    = 0,
    XRAUDIO_RESULT_ERROR = 1
} xraudio_result_t;

int32_t xraudio_container_header_parse_wave(int audio_fd, const uint8_t *header, size_t size, xraudio_output_format_t *format, uint32_t *data_length);

#ifdef __cplusplus
}
#endif

#endif /* _XRAUDIO_H_ */
