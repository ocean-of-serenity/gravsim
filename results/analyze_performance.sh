#!/bin/sh

python3 analyze.py  $(find . -depth -name 'performance*.csv' -printf '%p ')

