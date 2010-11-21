#!/bin/bash
./brain < $1 &
PID=$!
sleep 2
kill -s SIGTERM $PID
