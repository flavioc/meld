#!/bin/bash

TEST="${1}"
od -t u4 -N 1 -j 1 ${TEST} | head -n 1 | awk '{print $2}'
