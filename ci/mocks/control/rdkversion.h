/*
 * Stub for rdkversion.h
 *
 * Copyright 2024 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _RDKVERSION_H_
#define _RDKVERSION_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *image_name;
    char *stb_name;
    char *branch_name;
    char *version_name;
    char *image_build_time;
    bool  production_build;
    char *parse_error;
} rdk_version_info_t;

int  rdk_version_parse_version(rdk_version_info_t *info);
void rdk_version_object_free(rdk_version_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* _RDKVERSION_H_ */
