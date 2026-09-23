#!/bin/bash
# 整合版：eMMC 分区格式化 + SD卡固件固化到 eMMC
# 使用方式：sudo ./emmc_init_and_copy.sh

# ===================== 配置常量 =====================
SOURCE_DIR="/run/media/BOOT-mmcblk0p1"
EMMC_MNT_BASE="/emmc"
DEST_DIR1="${EMMC_MNT_BASE}/factory_data"
DEST_DIR2="${EMMC_MNT_BASE}/upgrade_data"
ROOTFS_DEST="${EMMC_MNT_BASE}/rootfs"
ROOTFS_ARCHIVE="2040s_rootfs.tar.gz"
FILES_TO_COPY=("fpga_top.bit" "system.dtb" "uImage")
EMMC_DEV="/dev/mmcblk1"

# ===================== 工具函数 =====================
log() {
    echo "[$(date +%Y-%m-%d\ %H:%M:%S)] $1"
}

# 强制卸载并清理 eMMC 占用
force_unmount_emmc() {
    log "===== 强制清理 eMMC 占用 ====="
    fuser -k "${EMMC_DEV}"* 2>/dev/null || log "无占用 eMMC 的进程"
    umount -lf "${EMMC_DEV}"* 2>/dev/null || log "eMMC 分区已卸载"
    sync
    echo 3 > /proc/sys/vm/drop_caches 2>/dev/null
    sleep 2
}

check_emmc() {
    if [ -e "${EMMC_DEV}" ]; then
        return 0
    else
        log "错误：eMMC 设备 ${EMMC_DEV} 不存在！"
        return 1
    fi
}

erase_emmc_full() {
    log "===== 第一步：删除 eMMC 所有旧分区 ====="
    force_unmount_emmc
    log "删除旧分区表和所有分区..."
    (
    echo "d"; echo "1"; echo "d"; echo "2"; echo "d"; echo "3"; echo "o"; echo "w"
    ) | fdisk "${EMMC_DEV}" >/dev/null 2>&1
    dd if=/dev/zero of="${EMMC_DEV}" bs=512 count=2048 conv=fsync >/dev/null 2>&1
    partprobe "${EMMC_DEV}" 2>/dev/null
    sleep 2
    log "旧分区已全部删除，eMMC 恢复初始状态"
}

do_partition() {
    log "===== 第二步：创建 eMMC 新分区 ====="
    force_unmount_emmc
    (
    echo "o"; echo "n"; echo "p"; echo "1"; echo "16"; echo "+500M"; echo "t"; echo "c"
    echo "n"; echo "p"; echo "2"; echo ""; echo "+500M"; echo "t"; echo "2"; echo "c"
    echo "n"; echo "p"; echo "3"; echo ""; echo ""; echo "w"
    ) | fdisk "${EMMC_DEV}" >/dev/null 2>&1
    partprobe "${EMMC_DEV}" 2>/dev/null
    sleep 3
}

format_partitions() {
    log "===== 第三步：格式化 eMMC 分区 ====="
    force_unmount_emmc
    
    # 格式化 p1（FAT32，增加 -I 强制初始化，清除只读标记）
    mkfs.vfat -F 32 -I -n FIR0 "${EMMC_DEV}p1" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p1 格式化 FAT32 成功（卷标 FIR0）"
    else
        log "错误：分区 ${EMMC_DEV}p1 格式化失败！"
        exit 1
    fi

    mkfs.vfat -F 32 -I -n FIR1 "${EMMC_DEV}p2" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p2 格式化 FAT32 成功（卷标 FIR1）"
    else
        log "错误：分区 ${EMMC_DEV}p2 格式化失败！"
        exit 1
    fi

    mkfs.ext4 -F -L ROOTFS "${EMMC_DEV}p3" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p3 格式化 EXT4 成功（卷标 ROOTFS）"
        e2fsck -y "${EMMC_DEV}p3" 2>/dev/null
        log "已执行 e2fsck 修复 ${EMMC_DEV}p3 文件系统"
    else
        log "错误：分区 ${EMMC_DEV}p3 格式化失败！"
        exit 1
    fi
}

mount_partitions() {
    log "===== 第四步：挂载 eMMC 分区 ====="
    mkdir -p "${DEST_DIR1}" "${DEST_DIR2}" "${ROOTFS_DEST}" 2>/dev/null

    # 第一步：先挂载 p1（即使显示只读也没关系）
    mount -t vfat "${EMMC_DEV}p1" "${DEST_DIR1}" 2>/dev/null
    # 第二步：强制重新挂载为可写（核心修复！）
    mount -o remount,rw "${EMMC_DEV}p1" "${DEST_DIR1}" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p1 强制挂载为可写模式（/emmc/factory_data）"
    else
        log "错误：p1 分区强制可写挂载失败！"
        exit 1
    fi

    # p2 直接可写
    mount -t vfat -o rw,utf8 "${EMMC_DEV}p2" "${DEST_DIR2}" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p2 挂载到 ${DEST_DIR2} 成功"
    else
        log "错误：分区 ${EMMC_DEV}p2 挂载失败！"
        exit 1
    fi

    # p3 可写
    mount -t ext4 -o rw,noatime "${EMMC_DEV}p3" "${ROOTFS_DEST}" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p3 挂载到 ${ROOTFS_DEST} 成功"
    else
        log "错误：分区 ${EMMC_DEV}p3 挂载失败！"
        exit 1
    fi
}

# 拷贝完成后，将 p1 重新挂载为只读
remount_p1_ro() {
    log "===== 第七步：将 p1 分区重新挂载为只读 ====="
    umount "${DEST_DIR1}" 2>/dev/null
    mount -t vfat -o ro,utf8 "${EMMC_DEV}p1" "${DEST_DIR1}" 2>/dev/null
    if [ $? -eq 0 ]; then
        log "分区 ${EMMC_DEV}p1 已重新挂载为只读模式"
    else
        log "警告：p1 分区改只读失败，但文件已拷贝完成，不影响使用"
    fi
}

check_prerequisites() {
    log "===== 执行前置检查 ====="
    if [ "$(id -u)" -ne 0 ]; then
        log "错误：请使用 root 权限运行此脚本（sudo ./emmc_init_and_copy.sh）"
        exit 1
    fi
    check_emmc || exit 1
    if [ ! -d "${SOURCE_DIR}" ]; then
        log "错误：SD 卡源目录 ${SOURCE_DIR} 不存在！"
        exit 1
    fi
    for file in "${FILES_TO_COPY[@]}"; do
        if [ ! -f "${SOURCE_DIR}/${file}" ]; then
            log "错误：SD 卡中缺少固件文件 ${SOURCE_DIR}/${file}！"
            exit 1
        fi
    done
    if [ ! -f "${SOURCE_DIR}/${ROOTFS_ARCHIVE}" ]; then
        log "错误：SD 卡中缺少根文件系统压缩包 ${SOURCE_DIR}/${ROOTFS_ARCHIVE}！"
        exit 1
    fi
    log "前置检查全部通过！"
}

copy_files() {
    log "===== 第五步：拷贝固件文件到 eMMC ====="
    # 再次验证 p1 分区是否可写（打印挂载属性）
    mount | grep "${EMMC_DEV}p1"
    for file in "${FILES_TO_COPY[@]}"; do
        log "拷贝 ${file} 到 ${DEST_DIR1}..."
        cp -f "${SOURCE_DIR}/${file}" "${DEST_DIR1}/"
        if [ $? -ne 0 ]; then
            log "错误：拷贝 ${file} 到 ${DEST_DIR1} 失败！"
            exit 1
        fi
        log "拷贝 ${file} 到 ${DEST_DIR2}..."
        cp -f "${SOURCE_DIR}/${file}" "${DEST_DIR2}/"
        if [ $? -ne 0 ]; then
            log "错误：拷贝 ${file} 到 ${DEST_DIR2} 失败！"
            exit 1
        fi
    done
    log "所有固件文件拷贝完成！"
}

extract_rootfs() {
    log "===== 第六步：解压根文件系统到 eMMC p3 ====="
    tar -zxf "${SOURCE_DIR}/${ROOTFS_ARCHIVE}" -C "${ROOTFS_DEST}"
    if [ $? -eq 0 ]; then
        log "根文件系统解压完成！"
    else
        log "错误：根文件系统解压失败！"
        exit 1
    fi
}

cleanup() {
    log "===== 收尾：卸载 eMMC 分区 ====="
    umount "${DEST_DIR1}" "${DEST_DIR2}" "${ROOTFS_DEST}" 2>/dev/null
    sync
    log "eMMC 分区已卸载，所有操作完成！"
}

main() {
    log "===== 开始 eMMC 分区格式化 + 固件固化流程 ====="
    check_prerequisites
    erase_emmc_full
    do_partition
    format_partitions
    mount_partitions
    copy_files
    extract_rootfs
    remount_p1_ro
    cleanup
    log "===== eMMC 初始化 + 固件固化全部完成！====="
}

main