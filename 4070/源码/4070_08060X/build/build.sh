###
 # @Author: fuyanshun
 # @Date: 2022-09-13 20:43:06
 # @LastEditors: fuyanshun
 # @LastEditTime: 2022-09-17 17:09:14
 # @FilePath: \GS500程序\build\build.sh
 # @Description: 
 # 
 # Copyright (c) 2022 by error: git config user.name && git config user.email & please set dead value or install git, All Rights Reserved. 
### 

#!bin/sh -e

#创建日志文件
create_build_log()
{
    if [ -e ${BUILD_LOG_FILE} ] ; then
        rm -rf ${BUILD_LOG_FILE}
    fi
    touch ${BUILD_LOG_FILE}
}

# build breaker
#复制编译好的app文件到build目录下
build_breaker()
{
    cd $ZN_TOP_DIR
    make clean
    make 
#判断上条命令是否执行成功
    if [ $? -eq 0 ] ; then
        echo -e "\e[32m make successful ! \e[0m"         
    else
        echo -e "\e[31m make failed ! \e[0m"
	exit
    fi
    cd -
    mv $ZN_TOP_DIR/src/app $ZN_BUILD_DIR
    ls -l app 
}

#所需文件名的初始化和定义
build_dirname()
{

    export ZN_BUILD_DIR="$(cd $(dirname ${BASH_SOURCE}) && pwd)"
#判断当前文件目录是否有内容
    if [ "`ls -A ${ZN_BUILD_DIR}`" = "" ]; then
        printf "Error: The BUILD directory is empty!!!\n"
        return 1;
    else
# Adding the Directory to the Path
        export PATH=${ZN_BUILD_DIR}:$PATH    
    fi
    
    export BUILD_LOG_FILE="${ZN_BUILD_DIR}/log.txt"
#提取BUILD上层目录
    export ZN_TOP_DIR="$(dirname ${ZN_BUILD_DIR})"

    export ZN_TARGET_DIR=${ZN_TOP_DIR}/target

    export ZN_MOUNT_DIR=${ZN_TOP_DIR}/tmp
    export ZN_ROOTFS_MOUNT_POINT=${ZN_MOUNT_DIR}/rootfs

    export ZN_ROOTFS_DIR="${ZN_TOP_DIR}/rootfs"

    mkdir -p ${ZN_TARGET_DIR} ${ZN_MODULE_DIR} ${ZN_ROOTFS_MOUNT_POINT}
}

#SD卡所需根文件制作
make_rootfs_SD()
{
    if [ "`ls -A ${ZN_TARGET_DIR}`" != "" ]; then
        sudo rm -f  ${ZN_TARGET_DIR}/ramdisk.image
        sudo rm -f  ${ZN_TARGET_DIR}/ramdisk.image.gz
        sudo rm -f  ${ZN_TARGET_DIR}/uramdisk.image.gz
    else
        echo -e "\e[31m can't find file \e[0m"
    fi
#查找rootfs文件下的根文件系统文件
    if [ ! -f "${ZN_ROOTFS_DIR}/uramdisk.image.gz" ]; then
        error_exit -e "\e[31m 找不到uramdisk.image.gz !!! \e[0m"
    else
        cp ${ZN_ROOTFS_DIR}/uramdisk.image.gz ${ZN_TARGET_DIR}
    fi
#删除挂载目录内的所有文件
    sudo rm -rf ${ZN_ROOTFS_MOUNT_POINT}/*
#将ramdisk.image挂载到rootfs目录
    dd if=${ZN_TARGET_DIR}/uramdisk.image.gz bs=64 skip=1 of=${ZN_TARGET_DIR}/ramdisk.image.gz
    gunzip ${ZN_TARGET_DIR}/ramdisk.image.gz

    echo -e "\e[32m 将 ramdisk.image 挂载到 ${ZN_ROOTFS_MOUNT_POINT} 目录... \e[0m"  
    sudo mount -o loop ${ZN_TARGET_DIR}/ramdisk.image ${ZN_ROOTFS_MOUNT_POINT}
#赋值build目录下编译生成的执行文件app到挂载的根文件目录
    sudo cp ${ZN_BUILD_DIR}/app ${ZN_ROOTFS_MOUNT_POINT}
#初始化程序使用，用在出厂程序中
    #sudo cp ${ZN_BUILD_DIR}/cubeam_init_system ${ZN_ROOTFS_MOUNT_POINT}
    sudo cp ${ZN_BUILD_DIR}/init_system.sh ${ZN_ROOTFS_MOUNT_POINT}
    sudo rm -rf ${ZN_ROOTFS_MOUNT_POINT}/cubeam_update_system

    sudo cp ${ZN_BUILD_DIR}/update_app_bit.sh ${ZN_ROOTFS_MOUNT_POINT}
    sudo cp ${ZN_BUILD_DIR}/update_system.sh ${ZN_ROOTFS_MOUNT_POINT}
#后续更新程序的固件使用
    sudo rm -rf ${ZN_ROOTFS_MOUNT_POINT}/cubeam_init_system
    #sudo cp ${ZN_BUILD_DIR}/cubeam_update_system ${ZN_ROOTFS_MOUNT_POINT}
    #sudo rm -rf ${ZN_ROOTFS_MOUNT_POINT}/init_system.sh

    if [ $? -eq 0 ] ; then
        echo -e "\e[32m app&update.sh cp successful  ! \e[0m"         
    else
        echo -e "\e[31m app cp failed ! \e[0m"
    fi
    #sudo rm -rf ${ZN_BUILD_DIR}/app
#卸载挂载的根文件目录
    echo -e "\e[32m Unmount ramdisk image... \e[0m"                           
    sudo umount ${ZN_ROOTFS_MOUNT_POINT}
#压缩根文件目录
    echo -e "\e[32m Compress ramdisk image... \e[0m"                          
    gzip ${ZN_TARGET_DIR}/ramdisk.image
#编译根文件系统文件在ZN_TARGET_DIR目录中
    if type mkimage >${BUILD_LOG_FILE} 2>&1; then
        echo -e "\e[32m Wrapping the image with a U-Boot header... \e[0m"     
        mkimage -A arm -T ramdisk -C gzip -d ${ZN_TARGET_DIR}/ramdisk.image.gz ${ZN_TARGET_DIR}/uramdisk.image.gz
    else
        error_exit "Missing mkimage command !!!"              >>${BUILD_LOG_FILE}
    fi

    echo "$(date "+%Y.%m.%d-%H.%M.%S") : Finished ${ZN_TARGET_DIR}"
#查找build文件下是否有文件，有的话删掉
    if [ -f "${ZN_BUILD_DIR}/uramdisk.image.gz" ]; then
        rm -f "${ZN_BUILD_DIR}/uramdisk.image.gz"                            
    fi
    dd if=${ZN_TARGET_DIR}/uramdisk.image.gz of=${ZN_BUILD_DIR}/uramdisk.image.gz bs=2048 count=8192 conv=sync
    echo -e "\e[32m Creating a root file image is complete \e[0m" 

    sudo cp ${ZN_BUILD_DIR}/app /mnt/app-share
    sudo cp ${ZN_BUILD_DIR}/uramdisk.image.gz /mnt/app-share
}

build_main()
{

    build_dirname

    BASE_NAME=$(basename ${BASH_SOURCE})
    echo "当前文件名$BASE_NAME"
    if [ "$BASE_NAME" == "build.sh" ] ; then
        echo ""
        echo "+++++++++$ZN_BUILD_DIR"
        echo ""
    fi

    if [ -e "$ZN_TOP_DIR/program" ] ; then
        rm -rf $ZN_TOP_DIR/program
        echo "---- rm -rf $ZN_TOP_DIR/program -----"
    fi

    create_build_log
    build_breaker
    
    make_rootfs_SD

}

build_main

