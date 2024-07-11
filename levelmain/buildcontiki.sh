#!/bin/bash

set -euxo pipefail

TARGET=gecko2
LEVEL_TARGET=$1

BOARD=${LEVEL_TARGET}
if [ "${LEVEL_TARGET}" = "dk" ]; then
    BOARD=brd4210a
fi

TARGET_DIR=build/${BOARD}

reversion()
{
    # default version 01.00.00.00
    if [ -f ../../build/build_version.txt ]; then
        VERSION=`head -n1 ../../build/build_version.txt`
        BUILD_SRC="Jenkins"
    else
        if [ -f app_version.txt ]; then
            VERSION=`head -n1 app_version.txt`
        else
            VERSION="1.0.0+0"
        fi
        BUILD_SRC="Develop"
    fi
    GIT_HASH=`git rev-parse HEAD`

    # convert version text to 32 bit integer

    # a) split into components on .
    IFS='.' read -ra VERCOMPSA <<< "$VERSION"

    # b) split third component on +
    IFS='+' read -ra VERCOMPSB <<< "${VERCOMPSA[2]}"

    #echo "VERS=" $VERSION
    #echo "VA=" ${VERCOMPSA[@]}
    #echo "VB=" ${VERCOMPSB[0]}

    # c) shift and add in components
    vernum=${VERCOMPSA[0]}
    vernum=$(($vernum * 256))
    vernum=$(($vernum + ${VERCOMPSA[1]}))
    vernum=$(($vernum * 256))
    vernum=$(($vernum + ${VERCOMPSB[0]}))
    vernum=$(($vernum * 256))
    vernum=$(($vernum + ${VERCOMPSB[1]}))

    echo "Using App verion number" $vernum

    local bld_dir="$1"

    version_file_src="app_properties_config.h.template"
    version_file_dst="${bld_dir}/app_properties_config.h"

    cp ${version_file_src} ${version_file_dst}

    # change SL_APPLICATION_VERSION 1 to what we want it to be
    sed -i.bak "s/#define SL_APPLICATION_VERSION.*$/#define SL_APPLICATION_VERSION $vernum/g" "${version_file_dst}"
}

main()
{
    # commander path (hack!)
    if [ "$(uname)" = 'Linux' ]; then
        commander_exe="/home/ubuntu/.local/bin/commander"
        if [ ! -f "$commander_exe" ]; then
            commander_exe="commander"
        fi
    elif [ "$(uname)" = 'Darwin' ]; then
        commander_exe="/Applications/Simplicity Studio.app/Contents/Eclipse/developer/adapter_packs/commander/Commander.app/Contents/MacOS/commander"
    fi

    mkdir -p ${TARGET_DIR}
    reversion ${TARGET_DIR}

    #make BOARD=${BOARD} TARGET=${TARGET} BOOT_LOADABLE=y DEBUG=y
    make BOARD=${BOARD} TARGET=${TARGET} DEBUG=y
    #make BOARD=${BOARD} TARGET=${TARGET}

    cp build/${TARGET}/${BOARD}/subghzcpu.${TARGET} ${TARGET_DIR}/subghzcpu
    arm-none-eabi-objcopy -O srec ${TARGET_DIR}/subghzcpu  ${TARGET_DIR}/subghzcpu.s37
    arm-none-eabi-objdump -x ${TARGET_DIR}/subghzcpu > ${TARGET_DIR}/subghzcpu.map

    #${commander_exe} gbl create ${TARGET_DIR}/subghzcpu.gbl --app ${TARGET_DIR}/subghzcpu.s37
}

main "$@"

