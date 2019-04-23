#!/bin/bash

for i in {1..18}
do
	num=$((2 ** $i));
	echo "$num";
	sed -e "s/numSpheres = .*/numSpheres = $num/" main.go > temp;
	cat temp > main.go;
	rm temp;
	go run main.go;
done

sed -e 's/numSpheres = .*/numSpheres = 32768/' main.go > temp;
cat temp > main.go;
rm temp

