#ifndef _BOARD_CONFIG_
#define _BOARD_CONFIG_

#define DISABLE	0
#define ENABLE	1

#define  OV5640             0
#define  OV2640             1

#define  BOARD_KD233        0
#define  BOARD_LICHEEDAN    0
#define  BOARD_K61          0
#define  BOARD_Micro_K210	1

#if OV5640 + OV2640 != 1
#error ov sensor only choose one
#endif

#if BOARD_KD233 + BOARD_LICHEEDAN + BOARD_K61 + BOARD_Micro_K210!= 1
#error board only choose one
#endif

#endif
