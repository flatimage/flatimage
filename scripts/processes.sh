#!/bin/bash

while :; do
    clear
    PID="$(pgrep -o -f ".*$1.*")"
    if [ -n "$PID" ]; then pstree -p $PID; fi
    sleep 1
done
