#!/bin/sh

python3 analyze.py  $(find . -depth -name 'accuracy*.csv' -printf '%p ')

