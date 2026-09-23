#ifndef __SIGNAL_SWITCH_H_
#define __SIGNAL_SWITCH_H_

#include "sys.h"

/* HEADING */
typedef enum
{
    HEADING_INPUT_RS232 = 0,
    HEADING_INPUT_RS422
} heading_input_mode_t;

void heading_gpio_init(void);
void heading_set_input_mode(heading_input_mode_t mode);


/* MOTION */
typedef enum
{
    MOTION_INPUT_RS232 = 0,
    MOTION_INPUT_RS422
} motion_input_mode_t;

void motion_gpio_init(void);
void motion_set_input_mode(motion_input_mode_t mode);


/* GNSS */
typedef enum
{
    GNSS_INPUT_RS232 = 0,
    GNSS_INPUT_RS422
} gnss_input_mode_t;

void gnss_gpio_init(void);
void gnss_set_input_mode(gnss_input_mode_t mode);


/* PPS */
typedef enum
{
    PPS_INPUT_RS422 = 0,
    PPS_INPUT_TTL
} pps_input_mode_t;

void pps_gpio_init(void);
void pps_set_input_mode(pps_input_mode_t mode);

extern volatile uint8_t g_pps_flag;
extern volatile uint32_t g_pps_count;


/* SVS / 表声 */
typedef enum
{
    SVS_INPUT_RS232 = 0,
    SVS_INPUT_RS485
} SVS_input_mode_t;

void SVS_init(void);
void SVS_set_input_mode(SVS_input_mode_t mode);


/* 同步 */
typedef enum
{
    SYNC_MODE_INPUT = 0,                         /* 外部同步输入 -> 湿端 */
    SYNC_MODE_OUTPUT                             /* 湿端同步 -> 同步输出口 */
} sync_mode_t;

typedef enum
{
    SYNC_EDGE_RISING = 0,                        /* 上升沿触发 */
    SYNC_EDGE_FALLING,                           /* 下降沿触发 */
    SYNC_EDGE_BOTH                               /* 双边沿触发 */
} sync_edge_t;

extern volatile uint8_t g_sync_event_flag;
extern volatile uint8_t g_sync_in_flag;
extern volatile uint8_t g_sync_out_flag;
extern volatile uint32_t g_sync_in_count;
extern volatile uint32_t g_sync_out_count;
extern volatile uint8_t g_sync_last_level;

void sync_init(void);
void sync_config(sync_mode_t mode, sync_edge_t edge);
sync_edge_t sync_get_edge(void);

#endif
