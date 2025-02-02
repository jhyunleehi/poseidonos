#!/bin/bash
#VOLUME_SIZE=2147483648
VOLUME_SIZE=1073741824
#VirtualMachine?
IS_VM=false
SPDK_PATH=../lib/spdk-20.10
BIN_PATH=../bin
SCRIPT_PATH=../script

sudo  $SCRIPT_PATH/factory_reset.py
sudo  $SCRIPT_PATH/start_poseidonos.sh
sudo  $SPDK_PATH/scripts/rpc.py bdev_malloc_create -b uram0 2048 512
sleep 5
sudo  $BIN_PATH/cli system info 
sudo  $BIN_PATH/cli device scan
sudo  $BIN_PATH/cli device list 
sudo  $BIN_PATH/cli array delete --name POSArray 
sudo  $BIN_PATH/cli array create -b uram0 -d unvme-ns-0,unvme-ns-1,unvme-ns-2,unvme-ns-3,unvme-ns-4  --name POSArray --raidtype RAID5
sudo  $BIN_PATH/cli array info  --name POSArray 
echo  -----mount------
sudo  $BIN_PATH/cli array mount --name POSArray 

echo  -----create_subsystem------
sudo  $SPDK_PATH/scripts/rpc.py nvmf_create_subsystem nqn.2019-04.ibof:subsystem1 -a -s IBOF00000000000001 -d IBOF_VOLUME_EXTENSION -m 256
sudo  $SPDK_PATH/scripts/rpc.py nvmf_create_transport -t tcp -b 64 -n 4096
sudo  $SPDK_PATH/scripts/rpc.py nvmf_subsystem_add_listener nqn.2019-04.ibof:subsystem1 -t tcp -a 192.168.56.12 -s 1158
sudo  $SPDK_PATH/scripts/rpc.py nvmf_get_subsystems

echo  -->> create volume  
sudo  $BIN_PATH/cli volume create --name vol1 --size $VOLUME_SIZE --maxiops 0 --maxbw 0 --array POSArray
#sudo  $BIN_PATH/cli volume create --name vol2 --size 1073741824   --maxiops 0 --maxbw 0 --array POSArray
#sudo  $BIN_PATH/cli volume create --name vol3 --size 1073741824   --maxiops 0 --maxbw 0 --array POSArray
sudo  $BIN_PATH/cli volume list --array POSArray
sudo  $BIN_PATH/cli volume mount --name vol1 --array POSArray
#sudo  $BIN_PATH/cli volume mount --name vol2 --array POSArray
#sudo  $BIN_PATH/cli volume mount --name vol3 --array POSArray
sudo  $BIN_PATH/cli volume list --array POSArray

#sudo  $BIN_PATH/cli volume unmount --name vol2 --array POSArray
#sudo  $BIN_PATH/cli volume delete --name vol2 --array POSArray
sudo  kill -9 `ps -ef | grep  8000| grep  python3| awk '{print $2 }'`
sudo  $SPDK_PATH/scripts/rpc_http_proxy.py localhost 8000 good good &

sudo  $SPDK_PATH/scripts/rpc.py nvmf_get_subsystems

#sudo  $BIN_PATH/cli array unmount --name POSArray
#sudo  $BIN_PATH/cli array delete --name POSArray
#sudo  $BIN_PATH/cli system exit