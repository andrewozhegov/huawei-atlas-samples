#!/bin/bash

set -x

export FFMPEG_PATH=/usr/local/ffmpeg
export DDK_HOME=/home/HwHiAiUser/tools/che/ddk/ddk

path_cur=$(cd `dirname $0`; pwd)
build_type="Release"

source $path_cur/../Common/scripts/build_tools.sh

checkEnvs
checkArgs "$@"

buildA300 $build_target $build_type $path_cur

sed 's/graph_id:.*/graph_id: 100/g' $path_cur/graph.config > $path_cur/out/graph0.config
sed 's/graph_id:.*/graph_id: 101/g' $path_cur/graph.config > $path_cur/out/graph1.config
sed 's/graph_id:.*/graph_id: 102/g' $path_cur/graph.config > $path_cur/out/graph2.config
sed 's/graph_id:.*/graph_id: 103/g' $path_cur/graph.config > $path_cur/out/graph3.config
