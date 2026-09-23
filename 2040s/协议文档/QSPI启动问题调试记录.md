# 2040S QSPI 启动问题调试记录

## 1. 调试日期

2026 年 9 月 4 日

## 2. 硬件和软件环境

- 处理器：Xilinx Zynq-7000 `XC7Z045`
- Flash：Macronix `MX25L25635E`
- Flash 容量：32 MiB（256 Mbit）
- Flash ID：`C2 20 19`
- PetaLinux：2023.1
- U-Boot：2023.01
- Linux：6.1.5-xilinx-v2023.1
- 启动镜像：`BOOT.BIN`
- `BOOT.BIN` 内容：FSBL、bitstream、U-Boot
- 独立启动脚本：`boot.scr`
- 开发环境：Ubuntu 虚拟机、Vitis 2023.1、FPGA 工程师提供的 XSA

## 3. 初始问题

使用 SD 卡启动时，板卡可以正常启动并进入 U-Boot：

```text
U-Boot 2023.01
CPU:   Zynq 7z045
DRAM:  ECC disabled 1 GiB
...
Zynq>
```

将同一个 `BOOT.BIN` 和 `boot.scr` 烧录到 QSPI Flash 后，拨码开关切换到 QSPI 启动，上电不能正常进入 U-Boot。

## 4. 初步排查

### 4.1 U-Boot 可以识别 Flash

```text
Zynq> sf probe 0 0 0
SF: Detected mx25l25635e with page size 256 Bytes, erase size 64 KiB, total 32 MiB
```

说明 U-Boot 能够通过 SPI 访问 Flash，并且 JEDEC ID 识别正确。

### 4.2 BOOT.BIN 烧录和回读校验

从 SD 卡加载 BOOT.BIN：

```text
Zynq> fatload mmc 0:1 0x10000000 BOOT.BIN
1148360 bytes read
Zynq> setenv boot_size ${filesize}
```

从 SD 卡加载 boot.scr：

```text
Zynq> fatload mmc 0:1 0x10800000 boot.scr
6039 bytes read
Zynq> setenv bootscr_size ${filesize}
```

擦除并烧录 BOOT.BIN：

```text
Zynq> sf erase 0x00000000 0x009B0000
SF: 10158080 bytes @ 0x0 Erased: OK

Zynq> sf write 0x10000000 0x00000000 ${boot_size}
SF: 1148360 bytes @ 0x0 Written: OK
```

擦除并烧录 boot.scr：

```text
Zynq> sf erase 0x009C0000 0x00010000
SF: 65536 bytes @ 0x9c0000 Erased: OK

Zynq> sf write 0x10800000 0x009C0000 ${bootscr_size}
SF: 6039 bytes @ 0x9c0000 Written: OK
```

BOOT.BIN 回读校验：

```text
Zynq> sf read 0x12000000 0x00000000 ${boot_size}
SF: 1148360 bytes @ 0x0 Read: OK

Zynq> crc32 0x10000000 ${boot_size}
crc32 for 10000000 ... 101185c7 ==> 1afb0681

Zynq> crc32 0x12000000 ${boot_size}
crc32 for 12000000 ... 121185c7 ==> 1afb0681

Zynq> cmp.b 0x10000000 0x12000000 ${boot_size}
Total of 1148360 byte(s) were the same
```

boot.scr 回读校验：

```text
Zynq> sf read 0x12800000 0x009C0000 ${bootscr_size}
SF: 6039 bytes @ 0x9c0000 Read: OK

Zynq> cmp.b 0x10800000 0x12800000 ${bootscr_size}
Total of 6039 byte(s) were the same
```

结论：BOOT.BIN 和 boot.scr 的文件内容、烧录地址以及回读数据均正确。

### 4.3 Boot Header 和分区头检查

Boot Header 中的分区头偏移为：

```text
Zynq> sf read 0x13000000 0x00000098 0x4
Zynq> md.l 0x13000000 1
13000000: 000008c0
```

因此分区头地址为 `0x000008C0`。

从 QSPI 读取分区头：

```text
Zynq> sf read 0x13000000 0x000008C0 0x40
SF: 64 bytes @ 0x8c0 Read: OK

Zynq> md.b 0x13000000 0x40
13000000: 00 00 02 01 03 00 00 00 20 03 00 00 40 02 00 00
13000010: 00 00 00 00 ff ff ff ff ff ff ff ff ff ff ff ff
```

从 SD 卡加载到内存后，BOOT.BIN 内的同一位置也是：

```text
Zynq> fatload mmc 0:1 0x10000000 BOOT.BIN
Zynq> md.l 0x10000094 2
10000094: 00000000 000008c0
```

说明 BOOT.BIN 内部的 Boot Header 和分区头偏移正确。

## 5. FSBL 启动日志分析

SD 卡启动时，FSBL 能够正确完成启动：

```text
Xilinx First Stage Boot Loader
Release 2023.1
Boot mode is SD
SD Init Done
Partition Header Offset:0x00000C80
Partition Count: 3
...
SUCCESSFUL_HANDOFF
FSBL Status = 0x1
```

QSPI 启动时，FSBL 能够被 BootROM 成功加载并执行，并且能够识别 Flash：

```text
Xilinx First Stage Boot Loader
Release 2023.1
Boot mode is QSPI
Single Flash Information
FlashID=0xC2 0x20 0x19
MACRONIX 256M Bits
QSPI is in single flash connection
QSPI is in 4-bit mode
QSPI Init Done
Flash Base Address: 0xFC000000
```

但是 FSBL 后续读取启动头失败：

```text
Image Start Address: 0x00000000
Partition Header Offset:0xDDDDCCCC
Bank Selection 221
BankSel 221 != Register Read 1
Bank Selection Failed
Move Image failed
Header Information Load Failed
Partition Header Load Failed
FSBL Status = 0xA00E
```

关键判断：

- FSBL 已经从 QSPI 启动，说明启动拨码和 BootROM 基本正常；
- Flash ID 读取正确，说明基本 SPI 通信正常；
- FSBL 切换到 4-bit 模式后，读取到错误的分区头偏移 `0xDDDDCCCC`；
- 正确的分区头偏移应该是 `0x000008C0`；
- `0xDDDDCCCC` 是错误读取数据，不是 BOOT.BIN 中真实的地址；
- `boot.scr` 尚未执行，因此与当前 FSBL 失败无关；
- U-Boot 和 Linux 设备树也尚未执行，因此不是本次故障的直接原因。

## 6. Vivado 配置检查

Vivado 的 PS7 Peripheral I/O Pins 中显示：

```text
Quad SPI Flash
Single SS 4bit IO
```

该配置表示单颗 Flash、单片选、4-bit IO。界面中没有 `Single SS Legacy I/O` 或明显的 x1 选项。

因此没有继续在 PetaLinux 设备树中修改 QSPI 启动模式。Linux 设备树只影响系统启动后的 QSPI 驱动，不能修复 FSBL 早期读取启动头的问题。

## 7. 解决方法

### 7.1 在虚拟机中使用 Vitis 创建 FSBL 工程

加载环境：

```bash
source /opt/pkg/petalinux/settings.sh
source /tools/Xilinx/Vitis/2023.1/settings64.sh
```

启动 Vitis：

```bash
vitis &
```

创建工程时：

1. 选择 `Create a new platform from hardware (XSA)`；
2. 选择 FPGA 工程师提供的 XSA；
3. 处理器选择 `ps7_cortexa9_0`；
4. 操作系统选择 `standalone`；
5. 创建 `Zynq FSBL` 应用工程。

最终生成的工程结构包括：

```text
2040s_platform
zynq_fsbl_system
fsbl_qspi
```

FSBL 源码位于工程的 `src` 目录中。

### 7.2 修改 FSBL 的 QSPI 总线宽度

在 Vitis FSBL 工程中打开：

```text
fsbl_qspi/src/qspi.c
```

进入函数：

```c
InitQspi(void)
```

找到由硬件参数生成的 QSPI 总线宽度宏，原配置使用：

```c
XPAR_XQSPIPS_0_QSPI_BUS_WIDTH
```

将该宏对应的总线宽度修改为 1-bit，使 FSBL 不再使用当前的 4-bit QSPI 读取方式。实际修改的核心内容为：

```c
/* 原配置使用 XSA 生成的 QSPI 总线宽度 */
XPAR_XQSPIPS_0_QSPI_BUS_WIDTH

/* 调试修改：强制使用 1-bit SPI */
1
```

修改目的：绕过当前 FSBL 在 4-bit QSPI 模式下读取启动头异常的问题。

### 7.3 重新编译 FSBL

在 Vitis 中执行：

```text
右键 fsbl_qspi
→ Clean Project
→ Build Project
```

生成新的 FSBL ELF 文件，例如：

```text
/home/startest4070/workspace/fsbl_qspi/Debug/fsbl_qspi.elf
```

### 7.4 重新生成 BOOT.BIN

在 PetaLinux 工程目录执行：

```bash
cd ~/share/2040s_prj/2040s
source /opt/pkg/petalinux/settings.sh
```

如果 BOOT.BIN 包含 bitstream：

```bash
petalinux-package --boot \
  --fsbl /home/startest4070/workspace/fsbl_qspi/Debug/fsbl_qspi.elf \
  --fpga images/linux/system.bit \
  --u-boot images/linux/u-boot.elf \
  --force
```

如果 BOOT.BIN 不包含 bitstream，则去掉 `--fpga` 参数：

```bash
petalinux-package --boot \
  --fsbl /home/startest4070/workspace/fsbl_qspi/Debug/fsbl_qspi.elf \
  --u-boot images/linux/u-boot.elf \
  --force
```

### 7.5 重新烧录 BOOT.BIN 和 boot.scr

在 U-Boot 中从 SD 卡加载文件：

```text
mmc dev 0
fatload mmc 0:1 0x10000000 BOOT.BIN
setenv boot_size ${filesize}

fatload mmc 0:1 0x10800000 boot.scr
setenv bootscr_size ${filesize}
```

擦除并写入 QSPI：

```text
sf probe 0 0 0

sf erase 0x00000000 0x009B0000
sf write 0x10000000 0x00000000 ${boot_size}

sf erase 0x009C0000 0x00010000
sf write 0x10800000 0x009C0000 ${bootscr_size}
```

建议继续执行回读校验：

```text
sf read 0x12000000 0x00000000 ${boot_size}
crc32 0x10000000 ${boot_size}
crc32 0x12000000 ${boot_size}
cmp.b 0x10000000 0x12000000 ${boot_size}

sf read 0x12800000 0x009C0000 ${bootscr_size}
cmp.b 0x10800000 0x12800000 ${bootscr_size}
```

## 8. 最终验证结果

修改 FSBL、强制 QSPI 使用 1-bit 模式并重新生成 BOOT.BIN 后，板卡切换到 QSPI 启动可以正常启动，并进入 U-Boot。

最终结果：

```text
QSPI 启动成功
FSBL 正常运行
U-Boot 正常接管
```

## 9. 最终结论

本次问题不是 BOOT.BIN 烧录地址错误，也不是 boot.scr 地址错误。经过回读、CRC 和字节比较，确认 QSPI 中的数据与 SD 卡文件完全一致。

问题发生在：

```text
FSBL 初始化 QSPI 为 4-bit 模式后，
读取 BOOT.BIN 启动头/分区头数据异常。
```

FSBL 读取到的分区头偏移为错误值：

```text
0xDDDDCCCC
```

而正确值为：

```text
0x000008C0
```

最终通过 Vitis 基于 XSA 生成 FSBL 源码，并在 `src/qspi.c` 的 `InitQspi()` 相关配置中将：

```text
XPAR_XQSPIPS_0_QSPI_BUS_WIDTH
```

修改为 1，使 FSBL 使用 x1 SPI 总线读取 Flash，问题得到解决。

### 经验总结

1. U-Boot 能够 `sf probe`、擦写 Flash，不代表 FSBL 的 x4 读取一定正常；
2. 能看到 FSBL 的 QSPI 打印，说明 BootROM 已经能够加载并执行 FSBL；
3. FSBL 阶段失败时，不应优先修改 U-Boot、boot.scr 或 Linux 设备树；
4. `boot.scr` 只在 U-Boot 阶段执行，与 FSBL 读取分区头失败无关；
5. 对 QSPI 启动问题，必须区分：
   - Flash 基本 SPI 访问；
   - FSBL 的 x1/x2/x4 读取；
   - 4-byte 地址和 Bank 处理；
   - BOOT.BIN 分区头内容；
6. 当前工程的可用解决方案是让 FSBL 使用 x1 模式启动。
