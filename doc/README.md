# Posidon OS

## build  in virtual box
* cpu
```
# lscpu
Architecture:        x86_64
CPU op-mode(s):      32-bit, 64-bit
Byte Order:          Little Endian
CPU(s):              8
On-line CPU(s) list: 0-7
Thread(s) per core:  1
Core(s) per socket:  8
Socket(s):           1
NUMA node(s):        1
...
```

* mem
```
# lsmem
RANGE                                  SIZE  STATE REMOVABLE  BLOCK
0x0000000000000000-0x00000000dfffffff  3.5G online       yes   0-27
0x0000000100000000-0x000000041fffffff 12.5G online       yes 32-131

Memory block size:       128M
Total online memory:      16G
Total offline memory:      0B
```
* nvme disk
```
root@good-VirtualBox:~# nvme list
Node             SN                   Model                                    Namespace Usage                      Format           FW Rev  
---------------- -------------------- ---------------------------------------- --------- -------------------------- ---------------- --------
/dev/nvme0n1     VB1234-56789         ORCL-VBOX-NVME-VER12                     1           5.37  GB /   5.37  GB    512   B +  0 B   1.0     
/dev/nvme0n2     VB1234-56789         ORCL-VBOX-NVME-VER12                     2           5.37  GB /   5.37  GB    512   B +  0 B   1.0     
/dev/nvme0n3     VB1234-56789         ORCL-VBOX-NVME-VER12                     3           5.37  GB /   5.37  GB    512   B +  0 B   1.0     
/dev/nvme0n4     VB1234-56789         ORCL-VBOX-NVME-VER12                     4           5.37  GB /   5.37  GB    512   B +  0 B   1.0     
/dev/nvme0n5     VB1234-56789         ORCL-VBOX-NVME-VER12                     5           5.37  GB /   5.37  GB    512   B +  0 B   1.0

# lsblk -o +TYPE,SERIAL
NAME    MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT                    TYPE SERIAL
nvme0n1 259:0    0     5G  0 disk                               disk VB1234-56789
nvme0n2 259:1    0     5G  0 disk                               disk VB1234-56789
nvme0n3 259:2    0     5G  0 disk                               disk VB1234-56789
nvme0n4 259:3    0     5G  0 disk                               disk VB1234-56789
nvme0n5 259:4    0     5G  0 disk                               disk VB1234-56789
```
* SN is duplicated all nvme disk.

### source code
```
$ git clone https://github.com/poseidonos/poseidonos.git
```
* get dependency required to build
```
cd script
$ sudo ./pkgdep.sh
$ sudo apt install libssl-dev
$ sudo apt install libaio1
```
### build 
```
cd lib; sudo ./build_ibof_lib.sh all
...
Install the project...
-- Install configuration: "Release"
-- Installing: /home/good/code/poseidonos/lib/spdlog-1.4.2/lib/libspdlog.a
-- Installing: /home/good/code/poseidonos/lib/spdlog-1.4.2/lib/pkgconfig/spdlog.pc
-- Installing: /home/good/code/poseidonos/lib/spdlog-1.4.2/lib/spdlog/cmake/spdlogConfigTargets.cmake
-- Installing: /home/good/code/poseidonos/lib/spdlog-1.4.2/lib/spdlog/cmake/spdlogConfigTargets-release.cmake
-- Installing: /home/good/code/poseidonos/lib/spdlog-1.4.2/lib/spdlog/cmake/spdlogConfig.cmake
-- Installing: /home/good/code/poseidonos/lib/spdlog-1.4.2/lib/spdlog/cmake/spdlogConfigVersion.cmake

cd script: sudo ./build_ibofos.sh
```
* fix it to run POS

### run POS
```
$ sudo ./start_poseidonos.sh 
[sudo] good의 암호: 
0000:00:0e.0 (80ee 4e56): Already using the nvme driver
Setting maximum # of Huge Page Size is 2/3 of Total Memory Size
0000:00:0e.0 (80ee 4e56): nvme -> uio_pci_generic
/home/good/code/poseidonos/script
apport.service is not a native service, redirecting to systemd-sysv-install.
Executing: /lib/systemd/systemd-sysv-install disable apport
Current maximum # of memory map areas per process is 65530.
Setting maximum # of memory map areas per process to 65535.
vm.max_map_count = 65535
Setup env. done!
Execute poseidonos
Wait poseidonos
./start_poseidonos.sh: line 38:  6663 세그멘테이션 오류 nohup ${ROOT_DIR}/bin/$binary_name &>> ${ROOT_DIR}/script/${logfile}
```
* script/pos.log
```
[06 Aug 14:24:46.792][5105][critical] Cpu allowed list is wrongly set   string_descripted_cpu_set_generator.cpp:185
[06 Aug 14:24:46.792][5128][warning] Spdk Event Not Initialized   event_framework_api.cpp:92
[06 Aug 14:24:46.792][5142][error] Fail to finalize IOAT   accel_engine_api.cpp:327
```

* tring_descripted_cpu_set_generator.cpp:185
```
  if (isOverlaped)
    {
        printf("==>>[%d][%d]\n", isExceed, isOverlaped)
```

* restart
```
$ sudo ./start_poseidonos.sh 
0000:00:0e.0 (80ee 4e56): uio_pci_generic -> nvme
Setting maximum # of Huge Page Size is 2/3 of Total Memory Size
0000:00:0e.0 (80ee 4e56): nvme -> uio_pci_generic
/home/good/code/poseidonos/script
apport.service is not a native service, redirecting to systemd-sysv-install.
Executing: /lib/systemd/systemd-sysv-install disable apport
Current maximum # of memory map areas per process is 65535.
Setup env. done!
Execute poseidonos
Wait poseidonos
Wait poseidonos
poseidonos is running in background...logfile=pos.log

$ ps -ef | grep poseidonos | grep -v gre
root     10390  1160 99 14:47 pts/2    00:03:42 /home/good/code/poseidonos/script/..//bin/poseidonos
```

#### create uram0
```
/poseidonos/lib/spdk-20.10/scripts$ sudo ./rpc.py bdev_malloc_create -b uram0 8192 512
uram0
```

#### device list
```
$ ./cli device list

==> pos.log 

[06 Aug 15:25:02.494][5510][warning] Initialization complete.   nvme.cpp:510
[06 Aug 15:25:02.495][5517][warning] DEVICE DUPLICATED Name:unvme-ns-1, SN: VB1234-56789           device_manager.cpp:257
[06 Aug 15:25:02.495][5517][warning] DEVICE DUPLICATED Name:unvme-ns-2, SN: VB1234-56789           device_manager.cpp:257
[06 Aug 15:25:02.495][5517][warning] DEVICE DUPLICATED Name:unvme-ns-3, SN: VB1234-56789           device_manager.cpp:257
[06 Aug 15:25:02.495][5517][warning] DEVICE DUPLICATED Name:unvme-ns-4, SN: VB1234-56789
[06 Aug 15:25:02.502][2311][warning] mbr parity check fail, mbr is reset   mbr_manager.cpp:396
[06 Aug 15:25:02.502][2313][warning] mbr data not found   mbr_manager.cpp:328
[06 Aug 15:25:02.502][2313][error] Failed to get abr list   array_manager.cpp:272
[06 Aug 15:25:02.502][2313][error] array loading failed : no array found   scan_device_command.cpp:81
{"command":"LISTDEVICE","rid":"0c791472-f67f-11eb-b14d-080027f58a4e"}
```

#### Device duplicateion  check 
```
bool DeviceManager::_CheckDuplication(UblockSharedPtr dev)
{
    DevName name(dev->GetName());
    DevUid sn(dev->GetSN());
    if (GetDev(name) == nullptr && GetDev(sn) == nullptr)   {return true;}
    POS_TRACE_WARN(POS_EVENT_ID::DEVICEMGR_CHK_DUPLICATE,"DEVICE DUPLICATED Name:{}, SN: {}",name.val, sn.val);
    //return false;
    return true;
}
```

```
int AbrManager::GetAbrList(std::vector<ArrayBootRecord>& abrList)
{
    int result = mbrManager->LoadMbr();
    //if (result != 0)
    //{
    //    return result;
    //}
    
    result = mbrManager->GetAbrList(abrList);
    return result;
}
```

#### rebuild 
```
cd script/
$ sudo ./build_ibofos.sh
$ kill -9 `ps -ef | grep pos| awk '{print $1}'`
$ sudo ./start_poseidonos.sh
```


#### check POS
```
poseidonos/bin$ ./cli system info
Request to Poseidon OS
    xrId        :  aaec8c20-f683-11eb-bf67-080027f58a4e
    command     :  GETIBOFOSINFO
Response from Poseidon OS
    Code         :  0
    Level        :  INFO
    Description  :  Success
    Problem      :  
    Solution     :  
    Data         : 
 {  "version": "pos-0.9.2" }
```

#### 
```
good@good-VirtualBox:~/code/poseidonos/bin$ ./cli device scan
good@good-VirtualBox:~/code/poseidonos/bin$ ./cli device list
Response from Poseidon OS
 {
    "devicelist": [
        {
            "addr": "0000:00:0e.0",
            "class": "SYSTEM",
            "mn": "ORCL-VBOX-NVME-VER12                    ",
            "name": "unvme-ns-0",
            "numa": "0",
            "size": 5368709120,
            "sn": "VB1234-56789        ",
            "type": "SSD"
        },
        {
            "addr": "0000:00:0e.0",
            "class": "SYSTEM",
            "mn": "ORCL-VBOX-NVME-VER12                    ",
            "name": "unvme-ns-1",
            "numa": "0",
            "size": 5368709120,
            "sn": "VB1234-56789        ",
            "type": "SSD"
        },
        {
            "addr": "0000:00:0e.0",
            "class": "SYSTEM",
            "mn": "ORCL-VBOX-NVME-VER12                    ",
            "name": "unvme-ns-2",
            "numa": "0",
            "size": 5368709120,
            "sn": "VB1234-56789        ",
            "type": "SSD"
        },
        {
            "addr": "0000:00:0e.0",
            "class": "SYSTEM",
            "mn": "ORCL-VBOX-NVME-VER12                    ",
            "name": "unvme-ns-3",
            "numa": "0",
            "size": 5368709120,
            "sn": "VB1234-56789        ",
            "type": "SSD"
        },
        {
            "addr": "0000:00:0e.0",
            "class": "SYSTEM",
            "mn": "ORCL-VBOX-NVME-VER12                    ",
            "name": "unvme-ns-4",
            "numa": "0",
            "size": 5368709120,
            "sn": "VB1234-56789        ",
            "type": "SSD"
        }
    ]
}
```

#### Create new POS array

```
# ./cli array create -b uram0 -d unvme-ns-0,unvme-ns-1,unvme-ns-2,unvme-ns-3,unvme-ns-4

Response from Poseidon OS
    Code         :  2504
    Level        :  ERROR
    Description  :  Device not found
    Problem      :  
    Solution     :  

==> pos.log    
{"command":"CREATEARRAY","rid":"de4d73c2-f686-11eb-a5be-080027f58a4e","param":{"buffer":[{"deviceName":"uram0"}],"data":[{"deviceName":"unvme-ns-0"},{"deviceName":"unvme-ns-1"},{"deviceName":"unvme-ns-2"},{"deviceName":"unvme-ns-3"},{"deviceName":"unvme-ns-4"}]}}
[06 Aug 16:21:08.060][2504][error] failed to create array POSArray   array.cpp:197


==>2 [2504]
[06 Aug 16:29:49.451][2504][error] failed to create array POSArray   array.cpp:203


==> error this point 

    for (string devName : nameSet.nvm)
    {
        DevName name(devName);
        UblockSharedPtr uBlock = sysDevMgr_->GetDev(name);
        if (nullptr == uBlock)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            return ret;
        }

```
* /var/log/pos/pos.log 
```
===>
[06 Aug 22:35:18.534][2530][info] the device added to the Array as DATA   array_device_list.cpp:145
[06 Aug 22:35:18.534][2507][error] SSD capacity is not valid. Valid capacity is from 20GB to 32TB   array_device_manager.cpp:548
[06 Aug 22:35:18.534][2507][error] failed to create array POSArray   array.cpp:203

==> update here
2GB limit
static const uint64_t MINIMUM_SSD_SIZE_BYTE = 2UL * SIZE_GB;
```

#### create pos array 
````
$ sudo  ./cli array create -b uram0 -d unvme-ns-0,unvme-ns-1,unvme-ns-2,unvme-ns-3,unvme-ns-4  --name POSArray --raidtype RAID5
 


Request to Poseidon OS
    xrId        :  28c38719-f6bd-11eb-9691-080027f58a4e
    command     :  CREATEARRAY
    Param       :
{
    "buffer": [
        {"deviceName": "uram0"}
    ],
    "data": [
        {"deviceName": "unvme-ns-0"},
        {"deviceName": "unvme-ns-1"},
        {"deviceName": "unvme-ns-2"},
        {"deviceName": "unvme-ns-3"},
        {"deviceName": "unvme-ns-4"}
    ]
}
Response from Poseidon OS
    Code         :  0
    Level        :  INFO
    Description  :  Success
    Problem      :  
    Solution     :
````

#### factory reset 

````
$ sudo  ./factory_reset.py 

 ================ POS FACTORY RESET ================ 
 ====================== WARNING ======================= 
 All information of volume and array including user data will be deleted.
 - Array and Volume configuration information will be removed
 - All user data will be removed
 - All POS logs will be removed
 - Configuration will be initialized

 I fully understand the precautions and agree to the initialization.
 The user is responsible for any data loss due to initialization.
 Would you like to continue?(y,n)y
kill pos
kill process pid : 3984
preparing reset config
config is initialized
preparing clear logs
all logs are removed
preparing reset mbr
Try to remove udev bind rule file
The rule file exists
The rule file removed
/home/good/code/poseidonos/script
 
===> After ./factory_reset.py log change...

{"command":"CREATEARRAY","rid":"ebe5be0d-f6ca-11eb-9e47-080027f58a4e","param":{"buffer":[{"deviceName":"uram0"}],"data":[{"deviceName":"unvme-ns-0"},{"deviceName":"unvme-ns-1"},{"deviceName":"unvme-ns-2"},{"deviceName":"unvme-ns-3"},{"deviceName":"unvme-ns-4"}]}}
==> ArrayDeviceManager::Import(DeviceSet<string> nameSet) 
[07 Aug 00:28:16.647][2590][error] ERROR Import()....    array_device_manager.cpp:70
==>2 [2504]
[07 Aug 00:28:16.647][2504][error] failed to create array POSArray   array.cpp:203

````

#### Create Array

```
$ sudo  ./cli array create -b uram0 -d unvme-ns-0,unvme-ns-1,unvme-ns-2,unvme-ns-3,unvme-ns-4  --name POSArray --raidtype RAID5


Request to Poseidon OS
    xrId        :  f1a78103-f8e2-11eb-bfe5-080027f58a4e
    command     :  CREATEARRAY
    Param       :
{
    "name": "POSArray",
    "raidtype": "RAID5",
    "buffer": [{"deviceName": "uram0"}],
    "data": [
        {"deviceName": "unvme-ns-0"},
        {"deviceName": "unvme-ns-1"},
        {"deviceName": "unvme-ns-2"},
        {"deviceName": "unvme-ns-3"},
        {"deviceName": "unvme-ns-4"}
    ]
}


Response from Poseidon OS
    Code        :  2525   ==>>  2525 : ARRAY_ALREADY_EXIST | DEBUG | - |
    Description :  failed to create POSArray(code:2525)
```

#### get POS array 

```
$ sudo ./cli array info --name POSArray


Request to Poseidon OS
    xrId        :  6ab9bbe1-f8e3-11eb-ab78-080027f58a4e
    command     :  ARRAYINFO
    Param       :
{"name": "POSArray"}

Response from Poseidon OS
    Code         :  0
    Level        :  INFO
    Description  :  Success
    Problem      :  
    Solution     :  
    Data         : 
 {
    "capacity": "0GB (0B)",
    "createDatetime": "2021-08-06 22:49:45 +0900",
    "devicelist": [
        {"name": "uram0","type": "BUFFER"},
        {"name": "unvme-ns-0","type": "DATA"}
    ],
    "name": "POSArray",
    "rebuildingProgress": "0",
    "situation": "DEFAULT",
    "state": "OFFLINE",
    "updateDatetime": "2021-08-06 22:49:45 +0900",
    "used": "0GB (0B)"
}
```

#### delete pos array 
````
$ sudo ./cli array delete   --name POSArray

Request to Poseidon OS
    xrId        :  193878fd-f8e7-11eb-bf33-080027f58a4e
    command     :  DELETEARRAY
    Param       :
{"name": "POSArray"}


Response from Poseidon OS
    Code         :  0
    Level        :  INFO
    Description  :  Success
    Problem      :  
    Solution     :
````


#### create pos array

```$ sudo  ./cli array create -b uram0 -d unvme-ns-0,unvme-ns-1,unvme-ns-2,unvme-ns-3,unvme-ns-4  --name POSArray1 --raidtype RAID5


Request to Poseidon OS
    xrId        :  3563470d-f8e7-11eb-8aa0-080027f58a4e
    command     :  CREATEARRAY
    Param       :
{
    "name": "POSArray1",
    "raidtype": "RAID5",
    "buffer": [
        {"deviceName": "uram0"}
    ],
    "data": [
        {"deviceName": "unvme-ns-0"},
        {"deviceName": "unvme-ns-1"},
        {"deviceName": "unvme-ns-2"},
        {"deviceName": "unvme-ns-3"},
        {"deviceName": "unvme-ns-4"}
    ]
}


Response from Poseidon OS
    Code         :  0
    Level        :  INFO
    Description  :  Success
    Problem      :  
    Solution     :  


$ sudo ./cli array info --name POSArray1


Request to Poseidon OS
    xrId        :  445e4593-f8e7-11eb-985c-080027f58a4e
    command     :  ARRAYINFO
    Param       : {"name": "POSArray1"}


Response from Poseidon OS
    Code         :  0
    Level        :  INFO
    Description  :  Success
    Problem      :  
    Solution     :  
    Data         : 
 {
    "capacity": "0GB (0B)",
    "createDatetime": "2021-08-09 16:55:48 +0900",
    "devicelist": [
        {"name": "uram0","type": "BUFFER"},
        {"name": "unvme-ns-0","type": "DATA"},
        {"name": "unvme-ns-1","type": "DATA"},
        {"name": "unvme-ns-2","type": "DATA"},
        {"name": "unvme-ns-3","type": "DATA"},
        {"name": "unvme-ns-4","type": "DATA"}
    ],
    "name": "POSArray1",
    "rebuildingProgress": "0",
    "situation": "DEFAULT",
    "state": "OFFLINE",
    "updateDatetime": "2021-08-09 16:55:48 +0900",
    "used": "0GB (0B)"
}

```

#### resize uram 
````
good@good-VirtualBox:~/code/poseidonos/lib/spdk-20.10/scripts$ sudo ./rpc.py bdev_malloc_create -b uram0 4096  512uram0

namespace pos { class MetaRegionContent {

===>>>[1]
==pointer=>>>[0x20002e400e00]
===>>>[1]
==pointer=>>>[0x200003813e00]
===>>>[2]
==pointer=>>>[0x200003811c00]
===>>>[2048]
==pointer=>>>[0x200088a00000]
===>>>[1]
==pointer=>>>[0x200003810a00]
===>>>[2]
==pointer=>>>[0x20000380e800]
===>>>[2048]
==pointer=>>>[0x200089400000]
[09 Aug 17:49:34.805][4353][error] Filesystem MBR has been corrupted or Filesystem cannot be found.   metafs_system_manager.cpp:145


````

#### mount array

````
$ sudo ./cli array mount  --name POSArray
FATA[0015] EOF                              

==pointer=>>>[0x200089400000]
[09 Aug 17:53:23.892][4340][error] Volume catalog is invalid. Try to recover broken volume catalog catalogs..   /home/good/code/poseidonos/src/metafs/mvm/volume/volume_catalog_manager.cpp:125
[09 Aug 17:53:23.892][4000][error] Load volume meta failed...   /home/good/code/poseidonos/src/metafs/mvm/volume/meta_volume.cpp:221
[09 Aug 17:53:23.892][4334][error] Meta volume open failed.   meta_volume_container.cpp:83
[09 Aug 17:53:23.892][4334][error] Meta volume open is failed. Volume is corrupted or isn't created yet   meta_volume_context.cpp:84
[09 Aug 17:53:23.892][4334][warning] Ran into an error while executing array mount sequence for POSArray1   array_mount_sequence.cpp:152
[09 Aug 17:53:23.892][4339][warning] All volume already closed. Ignore duplicate volume close   meta_volume_container.cpp:122

````