#!/bin/bash

PROGRAM=$1
DELTA=$2

bash get_all_pageranks.sh $PROGRAM 1 > pagerank-conv1.txt
for ((c=1; ; c++))
do
   d=`expr $c + 1`
   bash get_all_pageranks.sh $PROGRAM $d > pagerank-conv2.txt
   python pagerank_convergence.py pagerank-conv1.txt pagerank-conv2.txt $DELTA
   if [ $? = 0 ]; then
      echo $c
      rm -f pagerank-conv1.txt
      rm -f pagerank-conv2.txt
      exit 0
   fi
   mv pagerank-conv2.txt pagerank-conv1.txt
done

rm -f pagerank-conv1.txt
rm -f pagerank-conv2.txt
