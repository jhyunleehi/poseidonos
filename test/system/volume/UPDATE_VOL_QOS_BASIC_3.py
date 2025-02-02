#!/usr/bin/env python3
import subprocess
import os
import sys
import json
sys.path.append("../lib/")
sys.path.append("../array/")

import json_parser
import pos
import cli
import test_result
import UPDATE_VOL_QOS_BASIC_2
import volume

NAME = UPDATE_VOL_QOS_BASIC_2.NAME
SIZE = UPDATE_VOL_QOS_BASIC_2.SIZE
IOPS = (2**64-1) // 1000    # Refer to SRS: http://globalwiki.itplatform.sec.samsung.net:8099/display/ibof/2.3.1+%5BIBOFOS_SW_FRID_0301%5D+Create+Volume
BW = 0
ARRAYNAME = UPDATE_VOL_QOS_BASIC_2.ARRAYNAME

def clear_result():
    if os.path.exists( __file__ + ".result"):
        os.remove( __file__ + ".result")

def check_result(detail):
    expected_list = []
    expected_list.append(volume.Volume(NAME, SIZE, IOPS, BW))

    data = json.loads(detail)
    actual_list = []
    for item in data['Response']['result']['data']['volumes']:
        vol = volume.Volume(item['name'], item['total'], item['maxiops'], item['maxbw'])
        actual_list.append(vol)

    if len(actual_list) != len(expected_list):
        return "fail"
    
    for actual in actual_list:
        checked = False
        for expected in expected_list:
            if actual.name == expected.name and actual.total == expected.total and actual.maxiops == expected.maxiops and actual.maxbw == expected.maxbw:
                checked = True
                break
        if checked == False:
            return "fail"
    return "pass"

def set_result():
    out = cli.list_volume(ARRAYNAME)
    result = check_result(out)
    code = json_parser.get_response_code(out)
    with open(__file__ + ".result", "w") as result_file:
        result_file.write(result + " (" + str(code) + ")" + "\n" + out)

def execute():
    clear_result()
    UPDATE_VOL_QOS_BASIC_2.execute()
    out = cli.update_volume_qos(NAME, str(IOPS), str(BW), ARRAYNAME)
    return out

if __name__ == "__main__":
    execute()
    set_result()
    pos.kill_pos()
