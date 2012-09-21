#! /usr/bin/env python



import sys
import re
import json
import subprocess
import os
import array
import time

import requests


import random

from pprint import pprint

URL = "https://portal.senslab.info"
USERNAME="harter"
PASSWORD="clochette"


XP_LENGTH        = 2
DELAY_BETWEEN_XP = 2
NUMBER_OF_XP     = 2

NUMBER_OF_NODES = 10


EXPE_STR = '''{
   "type":"physical",
   "duration":50,
   "reservation":"0",
   "name":"calendarDemo",
   "nodes":null,
   "profileassociations": null,
   "profiles": null
}'''



def __alive_nodes_list(nodes_list):
    ''' Returns '''
    ret = []
    [ret.append(str(node["network_address"])) for node in nodes_list if node["state"] == "Alive"]
    return ret


def __create_experiment(nodes_list, timestamp, duration):
    exp = json.loads(EXPE_STR)
    exp["reservation"] = timestamp
    exp["nodes"] = nodes_list
    exp["duration"] = duration
    return json.dumps(exp, indent=2)



def __random_select_nodes(list_len, number):
    ''' Give a list of random numbers to pop items from a list (max value is decreasing for each value)

    n'th value of returned list is < "len(nodes_list) - n" (n from 0 to number -1)
    '''
    random.seed(time.time())
    rand_list = []

    [rand_list.append(random.randint(0, len -1)) for len in  range(list_len, list_len - number, -1)]
    return rand_list

def __generate_exp_list(nodes_list, number):

    alive = __alive_nodes_list(nodes_list)
    selected = __random_select_nodes(len(alive), number)
    expe_nodes = []
    [expe_nodes.append(alive.pop(id)) for id in selected]

    return expe_nodes


def __start_exp(json_str):
    post_url = "/rest/experiment?body"
    url = URL + post_url
    headers = {'content-type': 'application/json'}
    auth = requests.auth.HTTPBasicAuth(USERNAME, PASSWORD)

    r = requests.post(url, data=json_str, headers=headers, auth=auth)
    j =  json.loads(r.text)
    print json.dumps(j, indent=2)



if __name__ == '__main__':
    try:
        j = json.loads(sys.stdin.read())
    except ValueError, e:
        print >> sys.stderr, 'Could not load JSON object from stdin.'
        sys.exit(1)




    nodes_list = j["resources"]

    current_time = int(time.time())
    current_time += 2*3600 # Ugly patch because fred does not know how to manage time


    exp_start = current_time + DELAY_BETWEEN_XP * 60
    for i in range(0, NUMBER_OF_XP):
        expe_nodes = __generate_exp_list(nodes_list, NUMBER_OF_NODES)
        exp_json = __create_experiment(expe_nodes, exp_start, XP_LENGTH)
        __start_exp(exp_json)
        exp_start += (XP_LENGTH + DELAY_BETWEEN_XP) * 60




