#!/usr/bin/python
#
# This library reads a VM output and builds its own database.

from pyparsing import *

def parse_tuple(data):
    number = Word(nums + "Ee-.")
    token = Word(alphanums + "!-")
    address = Combine(Literal('@') + Word(nums))
    simple = address | number
    ls = Suppress('[') + Group(Optional(delimitedList(simple, Literal(',')))) + Suppress(']')
    arg = address | number | ls
    args = delimitedList(arg, Literal(', '))
    tpl = token + Suppress(Literal('(')) + Optional(args) + Suppress(')')
    return tpl.parseString(data)

def read_db(lines, only = None):
    current_node = None
    nodes = {}
    node_db = []
    for line in lines:
        line = line.rstrip('\n')
        if line == '':
            continue
        if line.isdigit():
            if current_node is not None:
                nodes[current_node] = node_db
            current_node = int(line)
            node_db = []
        else:
            tpl = parse_tuple(line)
            name = tpl[0]
            if only is None or name in only:
                args = tpl[1:]
                data = {'name': name, 'args': args}
                node_db.append(data)
    nodes[current_node] = node_db
    return nodes
