#!/usr/bin/env bash
#
# Build a customized rootfs archive.
#
# Usage:
#   ./build_rootfs.sh /path/to/app
#
# Run from the directory containing rootfs.tar.gz, interfaces, and S99_startapp.

#脚本功能：
#1.将当前目录下的rootfs.tar.gz，解压到当前目录下的rootfs文件夹内，该文件夹如果没有则新建一个
#2.文件拷贝
#    将指令路径下的app文件拷贝到 已经解压的rootfs/home/root/目录下
#    将interfaces文件拷贝到 已经解压的rootfs/etc/network/目录下
#    将S99_startapp 脚本拷贝到 已经解压的rootfs/etc/rc5.d/目录下
#3.脚本删除
#    删除解压目录rootfs/etc/rc5.d/
#        S12rpcbind
#        S19nfscommon
#        S20inetd.busybox
#        S15mountnfs.sh
#        S20hwclock.sh
#        S20nfsserver
#        S99tcf-agent
#4.将rootfs文件夹内所有东西打包成2040s_rootfs.tar.gz,并删除rootfs文件夹
set -euo pipefail

WORK_DIR="$(pwd)"
ROOTFS_ARCHIVE="${WORK_DIR}/rootfs.tar.gz"
ROOTFS_DIR="${WORK_DIR}/rootfs"
INTERFACES_FILE="${WORK_DIR}/interfaces"
START_APP_SCRIPT="${WORK_DIR}/S99_startapp"
OUTPUT_ARCHIVE="${WORK_DIR}/2040s_rootfs.tar.gz"

usage() {
    echo "Usage: $0 <app_path>" >&2
}

require_file() {
    local file_path="$1"
    local description="$2"

    if [[ ! -f "${file_path}" ]]; then
        echo "Error: ${description} does not exist: ${file_path}" >&2
        exit 1
    fi
}

if [[ $# -ne 1 ]]; then
    usage
    exit 1
fi

APP_PATH="$1"

require_file "${ROOTFS_ARCHIVE}" "rootfs archive"
require_file "${APP_PATH}" "app"
require_file "${INTERFACES_FILE}" "interfaces file"
require_file "${START_APP_SCRIPT}" "S99_startapp script"

if [[ -e "${ROOTFS_DIR}" ]] &&
   [[ -n "$(find "${ROOTFS_DIR}" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]; then
    echo "Error: rootfs directory is not empty: ${ROOTFS_DIR}" >&2
    echo "Remove or rename it before running this script." >&2
    exit 1
fi

mkdir -p "${ROOTFS_DIR}"

echo "Extracting ${ROOTFS_ARCHIVE} to ${ROOTFS_DIR}"
sudo tar -xzf "${ROOTFS_ARCHIVE}" -C "${ROOTFS_DIR}" --numeric-owner

APP_NAME="$(basename -- "${APP_PATH}")"
echo "Copying app to ${ROOTFS_DIR}/home/root/${APP_NAME}"
sudo install -D -m 0755 "${APP_PATH}" "${ROOTFS_DIR}/home/root/${APP_NAME}"

echo "Copying interfaces to ${ROOTFS_DIR}/etc/network/interfaces"
sudo install -D -m 0644 "${INTERFACES_FILE}" "${ROOTFS_DIR}/etc/network/interfaces"

echo "Copying S99_startapp to ${ROOTFS_DIR}/etc/rc5.d/S99_startapp"
sudo install -D -m 0755 "${START_APP_SCRIPT}" "${ROOTFS_DIR}/etc/rc5.d/S99_startapp"

echo "Removing unused rc5.d startup scripts"
for startup_script in \
    S12rpcbind \
    S19nfscommon \
    S20inetd.busybox \
    S15mountnfs.sh \
    S20hwclock.sh \
    S20nfsserver \
    S99tcf-agent; do
    sudo rm -f -- "${ROOTFS_DIR}/etc/rc5.d/${startup_script}"
done

echo "Creating ${OUTPUT_ARCHIVE}"
sudo tar -czf "${OUTPUT_ARCHIVE}" \
    --numeric-owner \
    --one-file-system \
    -C "${ROOTFS_DIR}" .

echo "Removing ${ROOTFS_DIR}"
sudo rm -rf -- "${ROOTFS_DIR}"

echo "Done: ${OUTPUT_ARCHIVE}"
