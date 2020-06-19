#!/bin/bash

path_cur=$(cd `dirname $0`; pwd)
build_type="Release"

export FFMPEG_PATH=/home/HwHiAiUser/itv-ascend-decoder/ffmpeg-4.2.1
export DDK_HOME=~/tools/che/ddk/ddk/

source $path_cur/../Common/scripts/build_tools.sh

checkEnvs
checkArgs "$@"

buildA300 $build_target $build_type $path_cur
