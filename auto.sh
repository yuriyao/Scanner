#!/bin/bash
cd lib
rm libscanner.so
make
cd ../
rm scanner
make

