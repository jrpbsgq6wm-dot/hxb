#include <string.h>

#include "sys.h"
#include "w25qxx.h"
#include "spi.h"
#include "usart.h"

/*
 * 新、旧板卡的 Flash 都接在 SPI3，片选均为 PA15。
 * 识别阶段使用低速 SPI；识别成功后，再按芯片型号切换到经过验证的工作速率。
 */
#define W25QXX_CS_PORT                    GPIOA
#define W25QXX_CS_PIN                     GPIO_PIN_15
#define W25QXX_BUSY_TIMEOUT_MS            30000U

/* 所有支持型号共用的基本命令。 */
#define W25QXX_CMD_WRITE_ENABLE           0x06U
#define W25QXX_CMD_READ_STATUS1           0x05U
#define W25QXX_CMD_READ_JEDEC_ID          0x9FU
#define W25QXX_CMD_READ_LEGACY_ID         0x90U
#define W25QXX_CMD_ENABLE_RESET           0x66U
#define W25QXX_CMD_RESET_DEVICE           0x99U

/* W25Q128 的 3 字节地址读、写、4KB 擦除命令。 */
#define W25Q128_CMD_READ_DATA             0x03U
#define W25Q128_CMD_PAGE_PROGRAM          0x02U
#define W25Q128_CMD_SECTOR_ERASE          0x20U

/* W25Q01JV 的专用 4 字节地址读、写、4KB 擦除命令。 */
#define W25Q01JV_CMD_READ_DATA_4B         0x13U
#define W25Q01JV_CMD_PAGE_PROGRAM_4B      0x12U
#define W25Q01JV_CMD_SECTOR_ERASE_4B      0x21U

/*
 * 信息结构体只在本文件中写入。其它模块通过 W25QXX_GetInfo() 只读访问，
 * 这样 diskio.c 不需要了解具体芯片命令或地址长度。
 */
static W25QXX_Info w25qxx_info;

/*
 * 512B 逻辑扇区写入时，必须保留同一个 4KB 擦除块中的其它数据。
 * 该缓冲区使用静态存储，避免在任务栈或中断栈中分配 4KB 导致栈溢出。
 */
static uint8_t w25qxx_erase_buffer[W25QXX_ERASE_SECTOR_SIZE];


static void W25QXX_Select(void)
{
    HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
}


static void W25QXX_Deselect(void)
{
    HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
}


static uint8_t W25QXX_TransferByte(uint8_t data)
{
    return SPI_ReadWriteByte(&SPI3_Config, data);
}


static void W25QXX_SendBuffer(const uint8_t *buffer, uint32_t len)
{
    uint32_t index;

    for (index = 0U; index < len; index++) {
        W25QXX_TransferByte(buffer[index]);
    }
}


static void W25QXX_ReceiveBuffer(uint8_t *buffer, uint32_t len)
{
    uint32_t index;

    for (index = 0U; index < len; index++) {
        buffer[index] = W25QXX_TransferByte(0xFFU);
    }
}


/*
 * 根据初始化阶段确认的地址长度发送地址。W25Q128 的有效地址最高字节必须省略；
 * W25Q01JV 则必须发送完整的四字节地址，才能访问超过 16MB 的区域。
 */
static void W25QXX_SendAddress(uint32_t addr)
{
    if (w25qxx_info.address_bytes == 3U) {
        W25QXX_TransferByte((uint8_t)(addr >> 16));
        W25QXX_TransferByte((uint8_t)(addr >> 8));
        W25QXX_TransferByte((uint8_t)addr);
    } else {
        W25QXX_TransferByte((uint8_t)(addr >> 24));
        W25QXX_TransferByte((uint8_t)(addr >> 16));
        W25QXX_TransferByte((uint8_t)(addr >> 8));
        W25QXX_TransferByte((uint8_t)addr);
    }
}


static void W25QXX_InitCS(void)
{
    GPIO_InitTypeDef gpio_init;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    memset(&gpio_init, 0, sizeof(gpio_init));
    gpio_init.Pin = W25QXX_CS_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W25QXX_CS_PORT, &gpio_init);

    /* 空闲状态必须拉高，避免初始化前总线噪声被 Flash 误解释为命令。 */
    W25QXX_Deselect();
}


/*
 * 统一的地址范围检查。使用“长度 <= 容量 - 起始地址”形式，
 * 既能检查越界，也避免 addr + len 在 uint32_t 中发生回绕。
 */
static uint8_t W25QXX_IsRangeValid(uint32_t addr, uint32_t len)
{
    if (w25qxx_info.initialized == 0U) {
        return 0U;
    }

    if (addr > w25qxx_info.capacity_bytes) {
        return 0U;
    }

    if (len > (w25qxx_info.capacity_bytes - addr)) {
        return 0U;
    }

    return 1U;
}


static uint8_t W25QXX_GetReadCommand(void)
{
    if (w25qxx_info.type == W25QXX_TYPE_W25Q128) {
        return W25Q128_CMD_READ_DATA;
    }

    return W25Q01JV_CMD_READ_DATA_4B;
}


static uint8_t W25QXX_GetPageProgramCommand(void)
{
    if (w25qxx_info.type == W25QXX_TYPE_W25Q128) {
        return W25Q128_CMD_PAGE_PROGRAM;
    }

    return W25Q01JV_CMD_PAGE_PROGRAM_4B;
}


static uint8_t W25QXX_GetSectorEraseCommand(void)
{
    if (w25qxx_info.type == W25QXX_TYPE_W25Q128) {
        return W25Q128_CMD_SECTOR_ERASE;
    }

    return W25Q01JV_CMD_SECTOR_ERASE_4B;
}


static void W25QXX_ResetDevice(void)
{
    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_CMD_ENABLE_RESET);
    W25QXX_Deselect();

    HAL_Delay(1U);

    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_CMD_RESET_DEVICE);
    W25QXX_Deselect();

    /* 复位后等待芯片重新进入空闲状态，再读取 JEDEC ID。 */
    HAL_Delay(10U);
}


static void W25QXX_WriteEnable(void)
{
    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_CMD_WRITE_ENABLE);
    W25QXX_Deselect();
}


/*
 * 页编程不能跨越 256B 页边界。跨页拆分由 W25QXX_WritePages() 负责；
 * 此函数只执行单个已确认合法的页编程命令，并等待该次编程真正完成。
 */
static uint8_t W25QXX_PageProgram(uint32_t addr, const uint8_t *buffer, uint32_t len)
{
    if ((buffer == NULL) || (len == 0U) || (len > W25QXX_PAGE_SIZE)) {
        return 0U;
    }

    if (((addr & (W25QXX_PAGE_SIZE - 1U)) + len) > W25QXX_PAGE_SIZE) {
        return 0U;
    }

    if (W25QXX_WaitBusy() == 0U) {
        return 0U;
    }

    W25QXX_WriteEnable();

    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_GetPageProgramCommand());
    W25QXX_SendAddress(addr);
    W25QXX_SendBuffer(buffer, len);
    W25QXX_Deselect();

    return W25QXX_WaitBusy();
}


/*
 * 将任意长度的数据拆分到连续的 256B 页中写入。
 * 每一页完成后均等待 BUSY 清零，避免下一条页编程命令覆盖前一条操作。
 */
static uint8_t W25QXX_WritePages(uint32_t addr, const uint8_t *buffer, uint32_t len)
{
    uint32_t page_offset;
    uint32_t write_len;

    while (len > 0U) {
        page_offset = addr & (W25QXX_PAGE_SIZE - 1U);
        write_len = W25QXX_PAGE_SIZE - page_offset;
        if (write_len > len) {
            write_len = len;
        }

        if (W25QXX_PageProgram(addr, buffer, write_len) == 0U) {
            return 0U;
        }

        addr += write_len;
        buffer += write_len;
        len -= write_len;
    }

    return 1U;
}


/*
 * 擦除一个对齐的 4KB 物理扇区。FatFs 的 512B 逻辑扇区不能直接映射到该命令，
 * 因此此函数只由 W25QXX_WriteWithErase() 在完成保护性合并后调用。
 */
static uint8_t W25QXX_SectorErase(uint32_t addr)
{
    if ((addr & (W25QXX_ERASE_SECTOR_SIZE - 1U)) != 0U) {
        return 0U;
    }

    if (W25QXX_IsRangeValid(addr, W25QXX_ERASE_SECTOR_SIZE) == 0U) {
        return 0U;
    }

    if (W25QXX_WaitBusy() == 0U) {
        return 0U;
    }

    W25QXX_WriteEnable();

    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_GetSectorEraseCommand());
    W25QXX_SendAddress(addr);
    W25QXX_Deselect();

    return W25QXX_WaitBusy();
}


uint8_t W25QXX_WaitBusy(void)
{
    uint8_t status;
    uint32_t start_tick;

    start_tick = HAL_GetTick();

    do {
        W25QXX_Select();
        W25QXX_TransferByte(W25QXX_CMD_READ_STATUS1);
        status = W25QXX_TransferByte(0xFFU);
        W25QXX_Deselect();

        if ((HAL_GetTick() - start_tick) > W25QXX_BUSY_TIMEOUT_MS) {
            return 0U;
        }
    } while ((status & 0x01U) != 0U);

    return 1U;
}


uint32_t W25QXX_ReadJEDECID(void)
{
    uint32_t jedec_id;

    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_CMD_READ_JEDEC_ID);

    jedec_id = ((uint32_t)W25QXX_TransferByte(0xFFU) << 16);
    jedec_id |= ((uint32_t)W25QXX_TransferByte(0xFFU) << 8);
    jedec_id |= (uint32_t)W25QXX_TransferByte(0xFFU);

    W25QXX_Deselect();

    return jedec_id;
}


uint16_t W25QXX_ReadLegacyID(void)
{
    uint16_t legacy_id;

    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_CMD_READ_LEGACY_ID);
    W25QXX_TransferByte(0x00U);
    W25QXX_TransferByte(0x00U);
    W25QXX_TransferByte(0x00U);

    legacy_id = (uint16_t)((uint16_t)W25QXX_TransferByte(0xFFU) << 8);
    legacy_id |= W25QXX_TransferByte(0xFFU);

    W25QXX_Deselect();

    return legacy_id;
}


 uint8_t W25QXX_Init(void)
{
    uint32_t jedec_id;
    uint16_t legacy_id;

    memset(&w25qxx_info, 0, sizeof(w25qxx_info));
    W25QXX_InitCS();

    /*
     * SPI3 已由 hardware_init() 初始化。此处先降到低速，仅用于复位和 ID 识别，
     * 避免不同批次或旧板卡在线路边沿条件较差时出现误识别。
     */
    SPI_SetSpeed(&SPI3_Config, SPI_BAUDRATEPRESCALER_16);
    HAL_Delay(1U);

    W25QXX_ResetDevice();
    jedec_id = W25QXX_ReadJEDECID();

    if (jedec_id == W25Q128_JEDEC_ID) {
        w25qxx_info.type = W25QXX_TYPE_W25Q128;
        w25qxx_info.capacity_bytes = W25Q128_TOTAL_CAPACITY;
        w25qxx_info.address_bytes = 3U;
        SPI_SetSpeed(&SPI3_Config, SPI_BAUDRATEPRESCALER_4);
        printf("    SPI Flash: W25Q128 detected (16MB, 3-byte address)\r\n");
    } else if (jedec_id == W25Q01JV_JEDEC_ID) {
        w25qxx_info.type = W25QXX_TYPE_W25Q01JV;
        w25qxx_info.capacity_bytes = W25Q01JV_TOTAL_CAPACITY;
        w25qxx_info.address_bytes = 4U;
        SPI_SetSpeed(&SPI3_Config, SPI_BAUDRATEPRESCALER_2);
        printf("    SPI Flash: W25Q01JV detected (128MB, 4-byte address)\r\n");
    } else {
        /*
         * 0x90 是对旧批次 W25Q128 的兼容回退检查。正常情况下 0x9F 已足够，
         * 但保留该路径可避免旧板卡因 ID 读取时序差异被误判为“无 Flash”。
         */
        legacy_id = W25QXX_ReadLegacyID();
        if (legacy_id == W25Q128_LEGACY_ID) {
            w25qxx_info.type = W25QXX_TYPE_W25Q128;
            w25qxx_info.capacity_bytes = W25Q128_TOTAL_CAPACITY;
            w25qxx_info.address_bytes = 3U;
            SPI_SetSpeed(&SPI3_Config, SPI_BAUDRATEPRESCALER_4);
            printf("    SPI Flash: W25Q128 detected by legacy ID (16MB)\r\n");
        } else {
            printf("    SPI Flash: unsupported ID 0x%06lX (legacy 0x%04X)\r\n",
                   jedec_id, legacy_id);
            return 0U;
        }
    }

    w25qxx_info.jedec_id = jedec_id;
    w25qxx_info.page_size = W25QXX_PAGE_SIZE;
    w25qxx_info.erase_sector_size = W25QXX_ERASE_SECTOR_SIZE;
    w25qxx_info.fatfs_sector_size = W25QXX_FATFS_SECTOR_SIZE;
    w25qxx_info.initialized = 1U;

    return 1U;
}


uint8_t W25QXX_IsReady(void)
{
    return w25qxx_info.initialized;
}


const W25QXX_Info *W25QXX_GetInfo(void)
{
    return &w25qxx_info;
}


uint8_t W25QXX_ReadData(uint32_t addr, uint8_t *buffer, uint32_t len)
{
    if ((buffer == NULL) || (len == 0U)) {
        return 0U;
    }

    if (W25QXX_IsRangeValid(addr, len) == 0U) {
        return 0U;
    }

    /*
     * 读取前等待上一笔擦写完成。这样 FatFs 的 CTRL_SYNC、后续读操作和
     * 驱动内部的读-改-擦-写流程都能获得一致的数据，不会读到擦除中的扇区。
     */
    if (W25QXX_WaitBusy() == 0U) {
        return 0U;
    }

    W25QXX_Select();
    W25QXX_TransferByte(W25QXX_GetReadCommand());
    W25QXX_SendAddress(addr);
    W25QXX_ReceiveBuffer(buffer, len);
    W25QXX_Deselect();

    return 1U;
}


uint8_t W25QXX_WriteWithErase(uint32_t addr, const uint8_t *buffer, uint32_t len)
{
    uint32_t remaining;
    uint32_t current_addr;
    uint32_t erase_addr;
    uint32_t erase_offset;
    uint32_t write_len;
    const uint8_t *source;

    if ((buffer == NULL) || (len == 0U)) {
        return 0U;
    }

    if (W25QXX_IsRangeValid(addr, len) == 0U) {
        return 0U;
    }

    remaining = len;
    current_addr = addr;
    source = buffer;

    while (remaining > 0U) {
        erase_addr = current_addr & ~(W25QXX_ERASE_SECTOR_SIZE - 1U);
        erase_offset = current_addr - erase_addr;
        write_len = W25QXX_ERASE_SECTOR_SIZE - erase_offset;

        if (write_len > remaining) {
            write_len = remaining;
        }

        if ((erase_offset == 0U) && (write_len == W25QXX_ERASE_SECTOR_SIZE)) {
            /*
             * 请求恰好覆盖完整的物理擦除块，可直接擦除并写入，
             * 不必执行 4KB 预读，减少一次 SPI 传输。
             */
            if (W25QXX_SectorErase(erase_addr) == 0U) {
                return 0U;
            }

            if (W25QXX_WritePages(erase_addr, source, write_len) == 0U) {
                return 0U;
            }
        } else {
            /*
             * FatFs 的一次 512B 写通常只覆盖 4KB 物理块的一部分。
             * NOR Flash 擦除会清空整个 4KB，因此必须先完整读出旧数据，
             * 覆盖本次更新的字节，再擦除并写回整个 4KB。
             */
            if (W25QXX_ReadData(erase_addr, w25qxx_erase_buffer,
                                W25QXX_ERASE_SECTOR_SIZE) == 0U) {
                return 0U;
            }

            memcpy(&w25qxx_erase_buffer[erase_offset], source, write_len);

            if (W25QXX_SectorErase(erase_addr) == 0U) {
                return 0U;
            }

            if (W25QXX_WritePages(erase_addr, w25qxx_erase_buffer,
                                  W25QXX_ERASE_SECTOR_SIZE) == 0U) {
                return 0U;
            }
        }

        current_addr += write_len;
        source += write_len;
        remaining -= write_len;
    }

    return 1U;
}
