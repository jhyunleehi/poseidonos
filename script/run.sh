sudo  /home/good/code/poseidonos/script/factory_reset.py
sudo  /home/good/code/poseidonos/script/start_poseidonos.sh
sudo  /home/good/code/poseidonos/lib/spdk-20.10/scripts/rpc.py bdev_malloc_create -b uram0 8192 512
sudo  /home/good/code/poseidonos/bin/cli system info 
sudo  /home/good/code/poseidonos/bin/cli device scan
sudo  /home/good/code/poseidonos/bin/cli device list 
sudo  /home/good/code/poseidonos/bin/cli array delete --name POSArray 
sudo  /home/good/code/poseidonos/bin/cli array create -b uram0 -d unvme-ns-0,unvme-ns-1,unvme-ns-2,unvme-ns-3,unvme-ns-4  --name POSArray --raidtype RAID5
sudo  /home/good/code/poseidonos/bin/cli array info  --name POSArray 
sudo  /home/good/code/poseidonos/bin/cli array mount --name POSArray 

