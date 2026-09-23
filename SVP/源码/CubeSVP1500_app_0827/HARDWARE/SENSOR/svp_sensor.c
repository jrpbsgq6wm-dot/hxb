#include "svp_sensor.h"

#include "ad7124.h"
#include "myiic.h"
#include "eeprom.h"
#include "usart.h"
#include "main.h"

/*
 * AD7124 的 ID 低八位用于表示芯片型号和硅版本。
 * 当前现场日志中使用的 AD7124-4 返回 0x04，旧工程还兼容过 0x12/0x14。
 * 这里只接受已知有效值，避免 SPI 总线悬空时的 0x00/0xFF 被误判为芯片存在。
 */
static uint8_t svp_sensor_is_valid_ad7124_id(uint16_t id)
{
    uint8_t id8 = (uint8_t)(id & 0xFFU);

    return (id8 == 0x04U) ||
           (id8 == 0x07U) ||
           (id8 == 0x12U) ||
           (id8 == 0x14U);
}

static SVP_SensorType svp_sensor_type = SVP_SENSOR_NONE;
static uint8_t svp_sensor_ready = 0U;
static uint8_t svp_ad7124_temperature_ready = 0U;
static uint16_t svp_ad7124_id = 0U;
/*
 * 旧板 AD7124 采用“读取本通道结果 -> 切换到下一通道 -> 等待下一次转换”的
 * 温度、压力参考电压、压力三通道轮询方式。
 *
 * 当前 TIM2 的基础节拍是 20 ms，主要用于满足 TDC 的采集节奏；而旧板正式工程
 * 原来以 50 ms 调用一次 AD7124_DATA()。若旧板也按 20 ms 连续读取，刚切换的
 * AD7124 通道还未经过滤波器稳定就会被再次读取，得到的状态和数据不能组成完整的
 * 温压采样链路，最终表现为 Depth 和 PT100_TEMP 长期为 0。
 *
 * 因此只对旧板 AD7124 分支限速到不少于 50 ms，不修改 TIM2 本身，也不影响：
 * - TDC 的 20 ms 处理；
 * - 新板 IIC 数字压力传感器的 20 ms 更新；
 * - FreeRTOS 任务调度。
 */
#define SVP_LEGACY_AD7124_SAMPLE_INTERVAL_MS    50U
static uint32_t svp_legacy_ad7124_last_sample_ms = 0U;

uint8_t SVP_Sensor_Init(void)
{
    uint8_t iic_pressure_ready;

    /*
     * 先探测 IIC 压力传感器。
     *
     * 新版硬件的识别依据是 IIC 地址 0x28 存在。先判断 IIC，
     * 可以避免新版硬件因为 SPI2 上存在其他电平或噪声而被误选为旧版。
     */
    iic_pressure_ready = IIC_PressureSensor_IsReady();

    if (iic_pressure_ready != 0U)
    {
        /*
         * 新版硬件：压力走 IIC，温度由 AD7124 提供。
         * 两个传感器缺一不可，不能仅有压力值就继续工作或记录文件。
         *
         * 新板 AD7124 需要先执行复位和寄存器配置，之后 ID 读取才稳定。
         * 原工程顺序就是 AD7124_Init() 后读取到 0x04；若在初始化前直接读，
         * 芯片上电时可能返回 0x00，从而把正常新板误判为传感器缺失。
         */
        AD7124_Init();
        svp_ad7124_id = Get_AD7124_ID();
        svp_ad7124_temperature_ready = svp_sensor_is_valid_ad7124_id(svp_ad7124_id);

        if (svp_ad7124_temperature_ready == 0U)
        {
            svp_sensor_type = SVP_SENSOR_NONE;
            svp_sensor_ready = 0U;
            printf("    Sensor detection failed: IIC pressure found, AD7124 missing\r\n");
            printf("    AD7124 ID:0x%x\r\n", svp_ad7124_id);
            return 0U;
        }

        svp_sensor_type = SVP_SENSOR_IIC_PRESSURE;
        PressureFilter_Init(&PressureFilter_t);
        printf("    New sensor mode: IIC pressure + AD7124 temperature\r\n");
        printf("    AD7124 Init Success ID:0x%x\r\n", svp_ad7124_id);
        printf("    IIC pressure sensor detected, address:0x%02x\r\n", IIC_ADDR);
        svp_sensor_ready = 1U;
        return 1U;
    }

    /*
     * 没有 IIC 压力传感器时按旧板处理。旧板的 AD7124 初始化会配置
     * 温度、压力参考电压和压力三个通道，因此也必须在读取 ID 前完成。
     */
    AD7124_Legacy_Init();
    svp_ad7124_id = Get_AD7124_ID();
    svp_ad7124_temperature_ready =
        svp_sensor_is_valid_ad7124_id(svp_ad7124_id);

    if (svp_ad7124_temperature_ready != 0U)
    {
        /*
         * 旧版硬件：IIC 压力传感器不存在，使用 AD7124 的三通道轮询。
         * AD7124_Legacy_Init() 会配置温度、压力参考电压和压力通道。
         */
        svp_sensor_type = SVP_SENSOR_AD7124_LEGACY;
        /*
         * 每次传感器识别后重新开始旧板采样节拍，避免复位、重新初始化或后续扩展时
         * 继承上一次运行的时间戳。
         */
        svp_legacy_ad7124_last_sample_ms = 0U;
        printf("    Legacy sensor mode: AD7124 pressure + temperature\r\n");
        printf("    AD7124 Init Success ID:0x%x\r\n", svp_ad7124_id);
        svp_sensor_ready = 1U;
        return 1U;
    }

    /*
     * 两种硬件都没有确认成功。继续运行会导致压力或温度保持旧值或为零，
     * 这类数据不允许进入直读输出和自容文件。
     */
    svp_sensor_type = SVP_SENSOR_NONE;
    svp_sensor_ready = 0U;
    printf("    Sensor detection failed: no valid pressure sensor\r\n");
    return 0U;
}

void SVP_Sensor_Update(void)
{
    if (svp_sensor_ready == 0U)
    {
        return;
    }

    switch (svp_sensor_type)
    {
        case SVP_SENSOR_IIC_PRESSURE:
            /*
             * 新版硬件由 AD7124 提供温度、IIC 提供压力。
             * 初始化阶段已确认两个传感器都存在，因此采集时必须同时更新。
             */
            AD7124_DATA();
            GetPressure(&PressureFilter_t);
            break;

        case SVP_SENSOR_AD7124_LEGACY:
            /*
             * 旧版硬件必须按照 AD7124 状态寄存器返回的通道号，
             * 依次完成温度、压力参考电压、压力数据的三通道轮询。
             */
            {
                uint32_t now_ms = HAL_GetTick();

                /*
                 * TIM2 本身为 20 ms，旧板 AD7124 的三通道切换则必须保留至少
                 * 50 ms 的转换与滤波稳定时间。使用 HAL tick 判断而不是简单计数，
                 * 后续即使修改 TIM2 周期，旧板 AD7124 的最小等待时间仍可保持正确。
                 */
                if ((svp_legacy_ad7124_last_sample_ms == 0U) ||
                    ((uint32_t)(now_ms - svp_legacy_ad7124_last_sample_ms) >=
                     SVP_LEGACY_AD7124_SAMPLE_INTERVAL_MS))
                {
                    svp_legacy_ad7124_last_sample_ms = now_ms;
                    AD7124_Legacy_DATA();
                }
            }
            break;

        default:
            /*
             * 未识别状态不执行任何采集，防止错误数据被写入文件。
             * 正常情况下该分支不会出现，因为初始化失败会停机。
             */
            break;
    }
}

SVP_SensorType SVP_Sensor_GetType(void)
{
    return svp_sensor_type;
}

uint8_t SVP_Sensor_IsReady(void)
{
    return svp_sensor_ready;
}
