#!/bin/bash

SAVEIFS=$IFS
IFS=$(echo -en "\n\b")
FILES=`find $1 -maxdepth 4 -name '*.mp3'`
FILES=$FILES`find $1 -maxdepth 4 -name '*.flac'`
FILES=$FILES`find $1 -maxdepth 4 -name '*.wav'`
PL_PATH=`dirname $0`

#PL_NAME='player_dev'
PL_NAME='player_static_working'
#PL_NAME='player_working_6fire'

#RAND_LIST=`for i in $FILES; do echo $i; done | sort --random-sort`
RAND_LIST=`for i in $FILES; do echo $i; done | sort `
for i in $RAND_LIST
do
	echo "playing: $i"
	$PL_PATH/$PL_NAME "$i"
    PID=$!
	echo $PID
done

IFS=$SAVEIFS
