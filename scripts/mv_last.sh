#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 FROM-DIR TO-DIR"
    exit 1
fi

if [ ! -d "$1" ]; then
    echo "Error: $1 is not a directory"
    exit 1
fi

if [ ! -d "$2" ]; then
    echo "Error: $2 is not a directory"
    exit 1
fi

prefix_to_move=$(ls $1/2015*_info.json | sort -V | tail -n 1)
cmd="mv ${prefix_to_move%info.*}* $2"
$cmd
