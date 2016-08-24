#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
"""
    Basic script for managing a JSON "database" file of SIM cards.
    It will look at the list of attached devices, and add their SIMs to a
    database.
    We expect to add much more functionality in the future.
"""

import argparse
import json
import acts.controllers.android_device as android_device
import acts.test_utils.tel.tel_defines as tel_defines
import acts.test_utils.tel.tel_lookup_tables as tel_lookup_tables

def add_sims(sim_card_file=None):
    if not sim_card_file:
        print("Error: file name is None.")
        return False
    try:
        f = open(sim_card_file, 'r')
        simconf = json.load(f)
        f.close()
    except FileNotFoundError:
        simconf = {}
    flag = False

    droid_list = android_device.get_all_instances()
    for droid_device in droid_list:
        droid = droid_device.get_droid(False)
        if droid.telephonyGetSimState() != tel_defines.SIM_STATE_READY:
            print("No Valid Sim! {} \n".format(droid.telephonyGetSimState()))
            continue
        serial = droid.telephonyGetSimSerialNumber()
        print("add_sims: {}".format(serial))

        # Add new entry if not exist
        if serial in simconf:
            print("Declining to add a duplicate entry: {}".format(serial))
            #TODO: Add support for "refreshing" an entry
            continue
        else:
            simconf[serial] = {}
            current = simconf[serial]
            flag = True

        try:
            #TODO: Make this list of hPLMNs a separate data structure
            current["operator"] = tel_lookup_tables.operator_name_from_plmn_id(
                droid.telephonyGetSimOperator())
        except KeyError:
            print("Unknown Operator {}".format(droid.telephonyGetSimOperator()))
            current["operator"] = ""

        # This is default capability: we need to try and
        # run-time determine in the future based on SIM queries?
        current["capability"] = ['voice', 'ims', 'volte', 'vt', 'sms',
                                 'tethering', 'data']

        phone_num = droid.telephonyGetLine1Number()
        if not phone_num:
            print("Please manually add a phone number for {}\n".format(serial))
            current["phone_num"] = ""
        else:
            # Remove the leading +
            if len(phone_num) == 10:
                phone_num = "1" + phone_num
            current["phone_num"] = ''.join(filter(lambda x: x.isdigit(), phone_num))
    if flag:
        f = open(sim_card_file, 'w')
        json.dump(simconf, f, indent=4, sort_keys=True)
        f.close()
    return True

def prune_sims(sim_card_file=None):
    if sim_card_file==None:
        sim_card_file = "./simcard_list.json"
    add_sims(sim_card_file)

    try:
        f = open(sim_card_file, 'r')
        simconf = json.load(f)
        f.close()
    except FileNotFoundError:
        print ("File not found.")
        return False

    flag = False

    droid_list = android_device.get_all_instances()

    simconf_list = list(simconf.keys())
    delete_list = []
    active_list = []

    for droid_device in droid_list:
        droid = droid_device.get_droid(False)
        if droid.telephonyGetSimState() != tel_defines.SIM_STATE_READY:
            print("No Valid SIM! {} \n".format(droid.telephonyGetSimState()))
            continue
        serial = droid.telephonyGetSimSerialNumber()
        active_list.append(serial)

    delete_list = list(set(simconf_list).difference(set(active_list)))

    print("active phones: {}".format(active_list))

    if len(delete_list) > 0:
        for sim in delete_list:
            # prune
            print("Deleting the SIM entry: ", sim)
            del simconf[sim]
            flag = True
    else:
        print("nothing to prune")

    if flag:
        f = open(sim_card_file, 'w')
        json.dump(simconf, f, indent=4, sort_keys=True)
        f.close()
    return True


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
                      description=("Script to generate, augment and prune"
                                   " SIM list"))
    parser.add_argument("--f","-file",
                       default='./simcard_list.json',
                       help="json file path", type=str)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-a",help="append to the list of SIM entries",
                      action='store_true')
    group.add_argument("-p",help="prune the list of SIM entries",
                      action='store_true')

    args = parser.parse_args()

    if args.a:
        add_sims(args.f)
    elif args.p:
        prune_sims(args.f)
    else:
        print ("Error: must select an option: -a or -p")

"""
Usage Examples

----------------------------------------------------------------
p3 manage_sim.py -h
usage: manage_sim.py [-h] [-f F] [-a | -p]

Script to generate, augment and prune SIM list

optional arguments:
  -h, --help      show this help message and exit
  -f F, --file F  name for json file
  -a              append to the list of SIM entries
  -p              prune the list of SIM entries

----------------------------------------------------------------
p3 manage_sim.py -f ./simcard_list.json -p
        OR
p3 manage_sim.py -p

Namespace(a=False, f='./simcard_list.json', p=True)
add_sims: 8901260222780922759
Please manually add a phone number for 8901260222780922759

active phones: 1
 ['8901260222780922759']
Deleting the SIM entry:  89148000001280331488
:
:
Deleting the SIM entry:  89014103277559059196

----------------------------------------------------------------
p3 manage_sim.py -f ./simcard_list.json -a
        OR
p3 manage_sim.py -a

Namespace(a=True, f='./simcard_list.json', p=False)
add_sims: 8901260222780922759
Please manually add a phone number for 8901260222780922759

----------------------------------------------------------------
"""
