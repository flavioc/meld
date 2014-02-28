#!/bin/bash

./meld -f $1 -d -- $2 | grep pagerank | awk -F ',' '{print $1 }' | awk -F '(' '{print $2}'
