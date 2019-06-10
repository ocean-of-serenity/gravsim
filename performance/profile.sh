#!/bin/sh

cd euler_naive
go build main.go
./main
cd ../euler_base
go build main.go
./main
cd ../euler_shared
go build main.go
./main
cd ../euler_shared_prefetch
go build main.go
./main
cd ../heun_shared_prefetch
go build main.go
./main
cd ../verlet_shared_prefetch
go build main.go
./main

