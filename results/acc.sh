#!/bin/sh

python3 acc.py  $(find . -name 'accuracy*.csv' -printf '%p ')

