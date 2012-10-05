#! /usr/bin/env python



import sys
import re
import json
import subprocess
import os
import array

from pprint import pprint

STATES = ["Alive", "Suspected"]


def __node_num(nodes):

    ret = nodes["network_address"].split(".")[0]
    ret = re.sub("node", "", ret)
    ret = int(ret)
    return ret


def __node_array_string(nodes):
    if (len(nodes) == 0):
        return ""
    # Manipulates the "prec" and current value
    # to be able to know if values are groupable
    prec = nodes.pop(0)
    not_in_a_group = True
    ret = ""
    for id in nodes:
        if (id - prec) == 1:
            if not_in_a_group:
                ret += "%d-" % prec
                not_in_a_group = False
        else:
            ret += "%d+" % prec
            not_in_a_group = True
        prec = id
    ret += "%d" % prec # Print last node
    return ret





def __print_for_state(state, nodes_list):
    sites_nodes = {"grenoble": [], "rennes": []}
    [sites_nodes[nodes["site"]].append(__node_num(nodes)) for nodes in nodes_list if nodes["state"] == state]
    for site in sites_nodes.iterkeys():
        print "%-10s" % (state + ":"),
        print "%-16s" % (site + ":"),
        print "%s" % __node_array_string(sites_nodes[site])




Color_Off='\033[0m'       # Text Reset

UBlack='\033[4;30m'       # Black
URed='\033[4;31m'         # Red
UGreen='\033[4;32m'       # Green
UYellow='\033[4;33m'      # Yellow
UBlue='\033[4;34m'        # Blue
UPurple='\033[4;35m'      # Purple
UCyan='\033[4;36m'        # Cyan
UWhite='\033[4;37m'       # White

def color_node(node):
    case = {"Alive": UGreen,
        "Suspected": UPurple
        }
    state = str(node["state"])
    id = re.sub(".senslab.info", "", node["network_address"])
    # 31 =  20 for the name + 4 from color_off, 7 from color
    return "%-31s" % (case[state] + id + Color_Off)


def joey(nodes_list):
    width = int(subprocess.check_output(["tput", "cols"]))
    cols = width / 20

    num_nodes = len(nodes_list)
    lines = num_nodes / cols + (1 if num_nodes % cols else 0)


    out_str = {}

    for row in range(0, lines):
        out_str[row] = ""

    x = 0
    for i in nodes_list:
        out_str[x] = out_str[x] + color_node(i)
        x = x + 1 if x != (lines -1) else 0

    for row in range(0, lines):
        print out_str[row]










if __name__ == '__main__':
    try:
        j = json.loads(sys.stdin.read())
    except ValueError, e:
        print >> sys.stderr, 'Could not load JSON object from stdin.'
        sys.exit(1)


    nodes_list = j["resources"]
    if len(sys.argv) == 2 and sys.argv[1] == "-joey":
        joey(nodes_list)
    else:
        [__print_for_state(state, nodes_list) for state in STATES]






#    print [sum(range(nombre)) for nombre in range(10) if nombre % 2 == 0]

