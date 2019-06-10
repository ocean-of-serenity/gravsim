#!/bin/sh

python3 analyze.py  $(find . -depth -name 'profile*.csv' -printf '%p ')

