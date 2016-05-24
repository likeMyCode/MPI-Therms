#!/bin/bash

numbers='^[0-9]+$'

if [ "$#" -ne 3 ]; then
	echo "[ERROR]: Illegal number of parameters"
elif ! [[ $1 =~ $numbers ]]; then
	echo "[ERROR]: Parametr 1 is not a positive, real number"
elif ! [[ $2 =~ $numbers ]]; then
	echo "[ERROR]: Parametr 2 is not a positive, real number"
elif ! [[ $3 =~ $numbers ]]; then
	echo "[ERROR]: Parametr 3 is not a positive, real number"
elif ! [[ $1 -gt 0 ]]; then
	echo "ERROR]: Number of people must be greater than 0"
elif ! [[ $2 -gt 0 ]]; then
	echo "ERROR]: Number of locker rooms must be greater than 0"
elif ! [[ $3 -gt 0 ]]; then 
	echo "ERROR]: Number of lockers must be greater than 0"
elif ! [[ $1 -gt $(($2 * $3)) ]]; then
	echo "[ERROR]: Number of people is not large enough"
else
	mpirun -n $1 ./therms $2 $3
fi
