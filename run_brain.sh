#!/bin/bash
./brain $1 < $2 >/dev/null 2>&1 >/dev/null 2>brain.log &
PID=$!
sleep 2
if :
then
kill -s SIGTERM $PID
wait
fi 2>/dev/null
cat brain.log | tee >(grep ctxt | tail -1 > ctxt.last) \
		    >(grep page | tail -1 > page.last) \
		    >(grep ops | tail -1 > ops.last) >/dev/null
sleep 1
cat ctxt.last
cat page.last
cat ops.last
rm ctxt.last page.last ops.last

