#!/bin/bash

set -x

DB="/mnt/db/leveldb"
exe=benchmark
log="output`date +%Y-%m-%d.%H:%M:%S`.csv"
keysize=1024
valuesize=$((keysize*4))

if [ ! -f "$exe" ]; then
	echo "Need to make benchmark"
	exit 1
fi

touch "$log"
echo "Block Size test" | tee -a "$log"
echo ",,LevelDB-Query,,,LevelDB-Insert,,,LevelDB-Delete,,,RocksDB-Query,,,RocksDB-Insert,,,RocksDB-Delete" | tee -a "$log"
echo "NodeSize,Quantity,Time(s),micros/op,MB/s,Time(s),micros/op,MB/s,Time(s),micros/op,MB/s,"\
"Time(s),micros/op,MB/s,Time(s),micros/op,MB/s,Time(s),micros/op,MB/s" | tee -a "$log"

for i in 268435456 134217728 67108864 33554432 16777216 8388608 4194304 2097152 1048576 524288; do
	if [ -d "$DB" ]; then
		rm -rf "$DB"/*
	fi

	./$exe -l -b -s $i -k $keysize -v $valuesize | tee -a "$log"

	rm -rf "$DB"/*
	./$exe -r -s $i -k $keysize -v $valuesize | tee -a "$log"
	echo | tee -a "$log"
done

