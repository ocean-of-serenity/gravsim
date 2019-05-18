#!/bin/sh

cd euler_naive
./main
cd ../euler_base
./main
cd ../euler_shared
./main
cd ../euler_shared_prefetch
./main
cd ../heun_shared_prefetch
./main
cd ../verlet_shared_prefetch
./main

