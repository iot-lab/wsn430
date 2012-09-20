#! /usr/bin/env python



import sys
import re
import json
import subprocess
import os
import array
import time


import random

from pprint import pprint

STATES = ["Alive", "Suspected"]





EXPE_STR = '''{
   "type":"physical",
   "duration":60,
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
    print ret
    return ret


def __create_experiment(nodes_list, timestamp):
    exp = json.loads(EXPE_STR)
    exp["reservation"] = timestamp
    exp["nodes"] = nodes_list
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
    selected = __random_select_nodes(len(alive), 10)
    expe_nodes = []
    [expe_nodes.append(alive.pop(id)) for id in selected]

    return expe_nodes


if __name__ == '__main__':
    try:
        j = json.loads(sys.stdin.read())
    except ValueError, e:
        print >> sys.stderr, 'Could not load JSON object from stdin.'
        sys.exit(1)


    # print re.sub("\..*$", "", str(time.time()))
    nodes_list = j["resources"]
#    print nodes_list
#    __schedule_expes(nodes_list)

    # __create_experiment()

    for i in range(0, 24):
        expe_nodes = __generate_exp_list(nodes_list, 10)








#    print [sum(range(nombre)) for nombre in range(10) if nombre % 2 == 0]

