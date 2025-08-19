#!/bin/bash
# Test 5 simultaneous requests
for i in {1..5}
do
   curl http://localhost:8080/index.html > /dev/null 2>&1 &
done
wait
echo "All 5 requests completed"