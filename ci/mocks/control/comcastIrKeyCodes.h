/*
 * Stub for comcastIrKeyCodes.h
 *
 * Copyright 2024 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _COMCAST_IR_KEY_CODES_H_
#define _COMCAST_IR_KEY_CODES_H_

/* Key event types */
#define KET_KEYDOWN   0x00000008
#define KET_KEYUP     0x00000100
#define KET_KEYREPEAT 0x00000200

/* Key codes */
#define KED_POWER                  0x00000001
#define KED_TVPOWER                0x00000002
#define KED_RF_POWER               0x00000003
#define KED_DISCRETE_POWER_ON      0x00000004
#define KED_DISCRETE_POWER_STANDBY 0x00000005
#define KED_VOLUMEUP               0x00000010
#define KED_VOLUMEDOWN             0x00000011
#define KED_MUTE                   0x00000012
#define KED_INPUTKEY               0x00000013
#define KED_SETUP                  0x00000020
#define KED_PUSH_TO_TALK           0x00000030
#define KED_RF_PAIR_GHOST          0x00000040
#define KED_SCREEN_BIND_NOTIFY     0x00000050
#define KED_XR2V3                  0x00000060
#define KED_XR5V2                  0x00000061
#define KED_XR11V1                 0x00000062
#define KED_XR11V2                 0x00000063
#define KED_XR11_NOTIFY            0x00000064
#define KED_XR15V1_NOTIFY          0x00000065
#define KED_XR16V1_NOTIFY          0x00000066
#define KED_UNDEFINEDKEY           0x000000FF

/* XMP tags */
#define XMP_TAG_COMCAST    0x0001
#define XMP_TAG_PLATCO     0x0002
#define XMP_TAG_XR11V2     0x0003
#define XMP_TAG_XR15V1     0x0004
#define XMP_TAG_XR15V2     0x0005
#define XMP_TAG_XR16V1     0x0006
#define XMP_TAG_XRAV1      0x0007
#define XMP_TAG_XR20V1     0x0008
#define XMP_TAG_UNDEFINED  0x00FF

#endif /* _COMCAST_IR_KEY_CODES_H_ */
