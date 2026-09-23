#ifndef __SVP_SENSOR_H__
#define __SVP_SENSOR_H__

#include <stdint.h>

/*
 * 声速剖面仪压力、温度采集硬件类型。
 *
 * 新旧两版硬件共用同一份应用程序，但压力采集电路不同：
 * 1. 旧版硬件：压力和温度都由 AD7124 采集；
 * 2. 新版硬件：压力由 IIC 数字压力传感器采集，温度仍由 AD7124 采集。
 *
 * 上电时由 SVP_Sensor_Init() 自动识别硬件，运行过程中不再重复判断，
 * 这样可以避免在采集周期内因为设备探测导致额外的总线访问。
 */
typedef enum
{
    SVP_SENSOR_NONE = 0,
    SVP_SENSOR_AD7124_LEGACY,
    SVP_SENSOR_IIC_PRESSURE
} SVP_SensorType;

/*
 * 初始化并识别当前硬件。
 *
 * 返回值：
 * 1：识别成功，并且对应的压力、温度采集链路已经初始化；
 * 0：没有识别到完整有效的采集链路。
 *
 * 设备的主要功能是采集。识别失败时由上层进入故障停机，
 * 不允许在传感器未确认的情况下继续记录可能无效的数据。
 */
uint8_t SVP_Sensor_Init(void);

/*
 * 执行一次采集更新。
 *
 * 本函数由 TIM2 周期回调调用。TDC 的采集流程仍然在原有代码中执行，
 * 本适配层只负责根据硬件类型选择压力、温度采集路径：
 * - 新版：AD7124 温度 + IIC 压力；
 * - 旧版：AD7124 温度 + AD7124 压力。
 */
void SVP_Sensor_Update(void);

/* 获取上电时识别出的传感器类型。 */
SVP_SensorType SVP_Sensor_GetType(void);

/* 返回当前采集链路是否已经成功初始化。 */
uint8_t SVP_Sensor_IsReady(void);

#endif
