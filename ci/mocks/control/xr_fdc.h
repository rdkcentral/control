/*
 * Stub for xr_fdc.h - File Descriptor Check
 *
 * Copyright 2024 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _XR_FDC_H_
#define _XR_FDC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t xr_fdc_check(uint32_t limit_soft, uint32_t limit_hard, bool log_fds);

#ifdef __cplusplus
}
#endif

#endif /* _XR_FDC_H_ */
