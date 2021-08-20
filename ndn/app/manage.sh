#!/usr/bin/env bash
set -x

#source ./native_setup.sh
source ./testbed.sh

rm -f idaddr.inc
rm -f routesdown.inc
rm -f routesup.inc

for i in "${ROUTES[@]}"; do
    set -- $i
    node=$1; dest=$2; next=$3; dir=$4;
    set -- ${NODES[$node]}
    hwaddr=$1;
    set -- ${NODES[$dest]}
    gaddr=${2/#fe80/2001:db8}
    set -- ${NODES[$next]}
    naddr=$1
    printf "ROUTE(${node#tap},\"${hwaddr}\",\"${gaddr}\",\"${naddr}\")\n" >> routes${dir}.inc
done
for i in "${!NODES[@]}"; do
    set -- ${NODES[$i]}
    gaddr=${1/#fe80/2001:db8}
    printf "MYMAP(${i#tap},\"${gaddr}\")\n" >> idaddr.inc
    if [ "$3" -eq "1" ];then
        GWADDR="\"${gaddr}\",${GWADDR}"
    fi
    if [ "$3" -eq "2" ];then
        NARR="${i#tap},${NARR}"
    fi
done

NARR="-DNARR='{ ${NARR::-1} }'"
GWADDRS="-DGWADDRS='{ ${GWADDR::-1} }'"

VERSIONS=2
VERSIONSCF="-DVERSIONS=${VERSIONS}"

CFLAGS="${NARR} ${GWADDRS} ${VERSIONSCF}" make -j4 clean all BOARD=iotlab-m3
cp bin/iotlab-m3/app.elf ndn.elf
#cp bin/native/app.elf ndn.elf

set +x
