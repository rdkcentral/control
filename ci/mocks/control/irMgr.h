/*
 * Stub for irMgr.h - IR Manager types
 *
 * Copyright 2024 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _IR_MGR_H_
#define _IR_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IARM_BUS_IRMGR_NAME "IRMgr"

typedef enum {
    IARM_BUS_IRMGR_EVENT_IRKEY   = 0,
    IARM_BUS_IRMGR_EVENT_CONTROL = 1,
    IARM_BUS_IRMGR_EVENT_MAX     = 2
} IARM_Bus_IRMgr_EventId_t;

#define IARM_BUS_IRMGR_KEYSRC_IR  0
#define IARM_BUS_IRMGR_KEYSRC_FP  1
#define IARM_BUS_IRMGR_KEYSRC_RF  2

typedef struct {
    union {
        struct {
            int keyType;
            int keyCode;
            int keySrc;
            int keyTag;
            int keySourceId;
        } irkey;
    } data;
} IARM_Bus_IRMgr_EventData_t;

#ifdef __cplusplus
}
#endif

#endif /* _IR_MGR_H_ */
