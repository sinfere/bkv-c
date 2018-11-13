#!/bin/bash

set -e
set -o pipefail

rm -rf mem_test || true
gcc common.c mem.c mem_test.c -o mem_test -g
./mem_test || true
rm -rf mem_test