#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..
date=`date`
echo '===========================================' >> test_results
echo "$date" >> test_results
make test-serial 2>&1 >> test_results
