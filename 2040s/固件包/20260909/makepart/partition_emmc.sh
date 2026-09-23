#! /bin/sh
 
# 彻底删除eMMC所有旧分区（修复总线扫描报错）
erase_emmc_full()
{
    echo "===== 第一步：删除eMMC所有旧分区 ====="
    # 1. 卸载所有已挂载的eMMC分区
    umount /dev/mmcblk1* 2>/dev/null || echo "无已挂载的eMMC分区"
    
    # 2. 用fdisk删除所有旧分区（屏蔽fdisk冗余输出）
    echo "删除旧分区表和所有分区..."
    (
    echo "d"
    echo "1"
    echo "d"
    echo "2"
    echo "d"
    echo "3"
    echo "o"
    echo "w"
    ) | fdisk /dev/mmcblk1 >/dev/null 2>&1
    
    # 3. 清空eMMC头部（适配BusyBox的dd语法）
    dd if=/dev/zero of=/dev/mmcblk1 bs=512 count=2048 conv=fsync >/dev/null 2>&1
    
    # 4. 仅用partprobe刷新，删除无效的总线扫描
    partprobe /dev/mmcblk1 2>/dev/null
    sleep 2
    
    echo " 旧分区已全部删除，eMMC恢复初始状态"
}

# 检查eMMC设备是否存在
check_emmc()
{
    if [ -e /dev/mmcblk1 ]; then
        return 1  # 存在返回1
    else
        return 0  # 不存在返回0
    fi
}
 
# 执行分区操作（屏蔽fdisk冗余输出）
do_partition()
{
    echo "===== 第二步：创建新分区 ====="
    # fdisk交互指令（适配起始扇区16，屏蔽输出）
    (
    echo "o"
    echo "n"
    echo "p"
    echo "1"
    echo "16"
    echo "+500M"
    echo "t"
    echo "c"
    echo "n"
    echo "p"
    echo "2"
    echo ""
    echo "+500M"
    echo "t"
    echo "2"
    echo "c"
    echo "n"
    echo "p"
    echo "3"
    echo ""
    echo ""
    echo "w"
    ) | fdisk /dev/mmcblk1 >/dev/null 2>&1
    
    # 刷新分区表
    partprobe /dev/mmcblk1 2>/dev/null
    sleep 3
}
 
# 检查分区并初始化（修复格式化/挂载报错）
check_partition_and_init()
{
    check_emmc
    if [ $? -eq "1" ]; then
        # 先执行全盘清空
        erase_emmc_full
        
        # 检查分区是否存在，不存在则创建
        if [ ! -e /dev/mmcblk1p1 ] || [ ! -e /dev/mmcblk1p2 ] || [ ! -e /dev/mmcblk1p3 ]; then 
            do_partition
            
            # 格式化分区（修复EXT4校验报错）
            echo "===== 第三步：格式化新分区 ====="
            mkfs.vfat -F 32 -n FIR0 /dev/mmcblk1p1 2>/dev/null
            mkfs.vfat -F 32 -n FIR1 /dev/mmcblk1p2 2>/dev/null
            # 添加-E lazy_itable_init=0 -E lazy_journal_init=0 关闭后台初始化
            mkfs.ext4 -F -L ROOTFS -O ^has_journal -E lazy_itable_init=0 -E lazy_journal_init=0 /dev/mmcblk1p3 2>/dev/null
            
            echo "Partitioning and initialization are complete"
        fi
        
        # 创建挂载目录
        mkdir -p /emmc/factory_data /emmc/upgrade_data /emmc/rootfs 2>/dev/null
        
        # 挂载分区（指定明确的挂载参数，修复umask警告）
        echo "===== 第四步：挂载分区 ====="
        mount -t vfat -o ro,utf8 /dev/mmcblk1p1 /emmc/factory_data/ 2>/dev/null
        mount -t vfat -o rw,utf8 /dev/mmcblk1p2 /emmc/upgrade_data/ 2>/dev/null
        mount -t ext4 -o rw,noatime /dev/mmcblk1p3 /emmc/rootfs/ 2>/dev/null
        
        echo "emmc mount complete"
        sync
    else
        echo "emmc failed (mmcblk1 not found)"
    fi
}
 
# 主执行逻辑
echo "===== 开始eMMC分区流程 ====="
check_partition_and_init
echo "===== eMMC分区/格式化/挂载全部完成 ====="