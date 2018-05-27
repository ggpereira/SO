#!/bin/bash

COUNTER=0;
n=100;
k=2;
exec_n=1;

echo ================first round ====================
while [ $COUNTER -lt 10 ]
do
  echo ----- Execution $exec_n thread: $k worksize: $n ----------
  ./t2 $k $n
  echo ---------End-----------------
   n=$(($n + 100));
   k=$(($k + 2));
   exec_n=$(($exec_n + 1));
   COUNTER=$(($COUNTER+1));
   sleep 2;
done

echo ========================End first round============================
COUNTER=0;
n=1000;
k=4;exec_n=1;

echo ========== Second round =========
while [ $COUNTER -lt 10 ]
do
  echo ----- Execution $exec_n thread: $k worksize: $n ----------
  ./t2 $k $n
  echo ---------Fim-----------------
   n=$(($n + 1000));
   k=$(($k + 2));
   exec_n=$(($exec_n + 1));
   COUNTER=$(($COUNTER+1));
   sleep 2;
done

echo ==================End Second Round=======================
echo --END--
