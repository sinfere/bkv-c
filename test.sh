#!/bin/sh

set -e
set -o pipefail

gcc bkv_test.c bkv.c -o test
./test || true
rm -rf test
