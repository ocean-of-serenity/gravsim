#!/bin/sh

cd euler_shared_prefetch
go build main.go
./main
cd ../heun_shared_prefetch
go build main.go
./main
cd ../verlet_shared_prefetch
go build main.go
./main

