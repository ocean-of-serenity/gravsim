#!/bin/sh

python3 acc_nos.py  $(find . -name 'accuracy*nos*.csv' -printf '%p ')
python3 acc_avg.py  $(find . -name 'accuracy*avg*.csv' -printf '%p ')

