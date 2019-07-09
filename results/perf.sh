#!/bin/sh

python3 perf.py  $(find . -name 'performance*.csv' -printf '%p ')

