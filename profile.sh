#!/bin/sh

cd performance/euler_naive
go run main.go
mv *.csv ../../results

cd ../euler_interleaved
go run main.go
mv *.csv ../../results

cd ../euler_nosoften
go run main.go
mv *.csv ../../results

cd ../euler_base
go run main.go
mv *.csv ../../results

cd ../euler_shared
go run main.go
mv *.csv ../../results

cd ../euler_shared_prefetch
go run main.go
mv *.csv ../../results

cd ../heun_shared_prefetch
go run main.go
mv *.csv ../../results

cd ../verlet_shared_prefetch
go run main.go
mv *.csv ../../results

cd ../../accuracy/euler_avg
go run main.go
mv *.csv ../../results

cd ../heun_avg
go run main.go
mv *.csv ../../results

cd ../verlet_avg
go run main.go
mv *.csv ../../results

cd ../euler_nos
go run main.go
mv *.csv ../../results

cd ../heun_nos
go run main.go
mv *.csv ../../results

cd ../verlet_nos
go run main.go
mv *.csv ../../results

