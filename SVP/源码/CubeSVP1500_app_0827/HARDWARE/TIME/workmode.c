#include  "usart.h"
#include  "time.h"
#include "file_fsm.h"
#include "power_app.h"

/*
 * 工作模式和子模式取值集中定义在这里，避免在定时器控制代码中直接使用魔术数。
 * 自容模式只开启 TIM5 文件服务节拍；是否真正建文件仍由 TDC 的稳定入水条件决定。
 */
#define WORK_MODE_SELF_CONTAINED    0x00    /* 自容模式 */
#define WORK_MODE_REAL_TIME         0x01    /* 串口直读模式 */

#define WORK_SUBMODE_PRESSURE       0x00    /* 自容压力变化记录模式 */
#define WORK_SUBMODE_FREQUENCY      0x01    /* 自容固定频率记录模式 */

#define DEFAULT_PRESSURE_THRESHOLD  0.5f    /* 压力阈值为零时采用的默认值 */
#define DEFAULT_FREQUENCY_DIVISOR   1000    /* 频率参数无效时采用的默认分频值 */

/* 电池电量上报开关及上一次已上报的电量记录。 */
/* 显控连接标志：收到连接指令后置 1，复位或断电后自动恢复为 0。 */
volatile uint8_t workmode_flag = 0;
Battry_t battry_t;
static volatile uint8_t battery_report_initialized = 0U;

/* 自容固定频率模式下，协议参数到内部 100 ms 节拍计数的映射表。 */
typedef struct {
    uint8_t cmd_value;
    uint16_t bound;
} FrequencyBoundMap;

static const FrequencyBoundMap freq_map[] = {
    {1,  1000},
    {5,  200},
    {10, 100}
};
#define FREQ_MAP_SIZE (sizeof(freq_map) / sizeof(freq_map[0]))

/* 串口直读模式下，协议参数到 TIM4 周期配置值的映射表。 */
typedef struct {
    uint8_t cmd_value;
    uint16_t period_ms;
} TimerPeriodMap;

static const TimerPeriodMap timer_map[] = {
    {1,  10000},    /* 1 秒，对应 TIM4 当前 0.1 ms 计数基准 */
    {5,  2000},     /* 5 次每秒 */
    {10, 1000}      /* 10 次每秒 */
};
#define TIMER_MAP_SIZE (sizeof(timer_map) / sizeof(timer_map[0]))

/* 根据协议频率参数查表；未命中时回退到默认分频值。 */
static uint16_t get_frequency_bound(uint8_t cmd_value) {
    for (int i = 0; i < FREQ_MAP_SIZE; i++) {
        if (freq_map[i].cmd_value == cmd_value) {
            return freq_map[i].bound;
        }
    }
    return DEFAULT_FREQUENCY_DIVISOR;
}

/* 根据直读频率参数查表；未命中时默认使用 1 秒输出周期。 */
static uint16_t get_timer_period(uint8_t cmd_value) {
    for (int i = 0; i < TIMER_MAP_SIZE; i++) {
        if (timer_map[i].cmd_value == cmd_value) {
            return timer_map[i].period_ms;
        }
    }
    return 1000;
}

/*
 * 配置自容模式的 TIM5。TIM5 每 100 ms 只置位文件任务事件，实际 FatFs 写入在
 * SVP_FILE 任务中执行，不能在 TIM5 中断内访问文件系统。
 */
static void config_timer5_for_self_contained(void) {
    HAL_TIM_Base_Stop_IT(&TIM_Config_5);
    TIM5_Init(1000, CLOCK_PSC);  /* 100 ms */
    HAL_TIM_Base_Start_IT(&TIM_Config_5);
}

/* 配置直读模式的 TIM4；TIM4 到期后由命令任务输出一行最新测量数据。 */
static void config_timer4_for_real_time(uint16_t period_ms) {
    HAL_TIM_Base_Stop_IT(&TIM_Config_4);
    TIM4_Init(period_ms, CLOCK_PSC);
    HAL_TIM_Base_Start_IT(&TIM_Config_4);
}

/*
 * 将 WORK_VALUE 的四字节内容解释为 float 压力阈值，并写入自容记录参数。
 * 主机传入零表示采用默认阈值，防止零阈值导致每个微小压力波动都写入文件。
 */
static void config_pressure_mode(void) {
    float threshold;
    memcpy(&threshold, &svp_cmd.WORK_VALUE, 4);
    
    pri_file.auto_wmode = 1;
    pri_file.pri_auto_pa_value = (threshold == 0.0f) ? DEFAULT_PRESSURE_THRESHOLD : threshold;
}

/* 配置固定频率记录模式，并将协议频率参数转换为内部节拍计数。 */
static void config_frequency_mode(void) {
    pri_file.auto_wmode = 0;
    pri_file.pri_auto_bound = get_frequency_bound(svp_cmd.WORK_VALUE);
}

/* 配置直读输出格式：压力子模式为正常格式，其余取值进入调试格式。 */
static void config_real_time_submode(void) {
    if (svp_cmd.WORK_SET_MODE_FLAG == WORK_SUBMODE_PRESSURE) {
        pri_workmode_flag = 0;
    } else {
        pri_workmode_flag = 1;
    }
}

/* 返回 EEPROM 当前配置的工作模式，供串口协议和状态服务判断使用。 */
uint8_t work_status(void){
	return svp_cmd.WORK_MODE_FLAG;
}

/*
 * 应用工作模式切换。
 * - 自容模式：关闭直读 TIM4，启动 TIM5 文件服务节拍；TDC 仍按 TIM2 固定节拍采集。
 * - 直读模式：先同步关闭正在记录的文件，再关闭 TIM5，最后启动 TIM4 数据输出。
 * - 无效模式：按直读调试模式处理，确保不会保留未关闭的自容记录文件。
 */
void timework_init(void) {
    if (svp_cmd.WORK_MODE_FLAG == WORK_MODE_SELF_CONTAINED) {
        /* 自容模式不直接创建文件，文件创建必须等待 TDC 判定稳定入水。 */
        HAL_TIM_Base_Stop_IT(&TIM_Config_4);
        config_timer5_for_self_contained();
        
        /* 根据子模式选择压力变化记录或固定频率记录参数。 */
        switch (svp_cmd.WORK_SET_MODE_FLAG) {
            case WORK_SUBMODE_PRESSURE:
                config_pressure_mode();
                break;
                
            case WORK_SUBMODE_FREQUENCY:
                config_frequency_mode();
                break;
                
            default:
                /* 无效子模式统一回退到固定频率记录，保持行为可预测。 */
                config_frequency_mode();
                break;
        }
    } 
    else if (svp_cmd.WORK_MODE_FLAG == WORK_MODE_REAL_TIME) {
        /* 切换直读前必须先刷写并关闭自容记录文件，保证尾部缓冲数据完整。 */
        pri_flush_record_now();
        HAL_TIM_Base_Stop_IT(&TIM_Config_5);
        
        config_real_time_submode();
        
        /* WORK_VALUE 在直读模式中表示上位机希望的数据输出周期。 */
        uint16_t period_ms = get_timer_period(svp_cmd.WORK_VALUE);
        config_timer4_for_real_time(period_ms);
    } 
    else {
        /* 无效模式按安全的直读调试模式处理，并确保 TIM5 与记录文件均已关闭。 */
        pri_flush_record_now();
        HAL_TIM_Base_Stop_IT(&TIM_Config_5);
        config_timer4_for_real_time(1000);
        
        pri_workmode_flag = 1;
    }
}

/*
 * 自容模式下按电量变化发送上报。文件下载、删除或格式化期间不发送，避免与文件
 * 协议数据交叉；直读模式已有实时输出，因此也不额外发送该上报。
 */
void battery_level_report_reset(void)
{
    battery_report_initialized = 0U;
    battry_t.NOW_SOC = 0;
    battry_t.LAST_SOC = 0;
}

void battery_level_report(void)
{
    int current_soc;
    float soc_delta;
    float soc_snapshot;
    uint8_t buf[sizeof(SOC)];

    /* Only report while the display is connected. */
    if (workmode_flag != 1U) {
        return;
    }

    /*
     * This function is only for the independent battery report in
     * self-contained mode. Real-time mode carries SOC in telemetry output.
     */
    if (svp_cmd.WORK_MODE_FLAG != WORK_MODE_SELF_CONTAINED) {
        return;
    }

    /*
     * FileManagerState covers file-list query, download, single/batch delete,
     * and format. Keep the last SOC pending so it is reported after the
     * file operation returns to STATE_IDLE.
     */
    if (file_manager_fsm_get_state() != STATE_IDLE) {
        return;
    }

    soc_snapshot = SOC;
    current_soc = (int)soc_snapshot;
    if (current_soc < 0) {
        current_soc = 0;
    } else if (current_soc > 100) {
        current_soc = 100;
    }
    battry_t.NOW_SOC = current_soc;

    soc_delta = current_soc - battry_t.LAST_SOC;
    if (soc_delta < 0) {
        soc_delta = -soc_delta;
    }

    /*
     * Report once after connection, then report every integer 1% change.
     * The payload remains the original 4-byte float SOC for protocol
     * compatibility.
     */
    if ((battery_report_initialized == 0U) || (soc_delta >= 0.5f)) {
        memcpy(buf, &soc_snapshot, sizeof(soc_snapshot));
        send_response(READ_CURRENT_POWER, buf, (uint8_t)sizeof(buf));
        battry_t.LAST_SOC = current_soc;
        battery_report_initialized = 1U;
    }
}
