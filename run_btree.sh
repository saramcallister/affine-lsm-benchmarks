#!/bin/bash
set -x

keysize=1024
valuesize=$((keysize*4))
mntpnt="/mnt/db"
num_keys_to_insert=3356000
num_op=$num_keys_to_insert
threads=1
query_threads=25
generic_options="--value_size=$valuesize --key_size=$keysize --num=$num_op"
sqlite_db="$mntpnt/sqlite"
tree_db="$mntpnt/tree_db"

if [ ! -d $sqlite_db ]; then
	mkdir $sqlite_db
fi
if [ ! -d $tree_db ]; then
	mkdir $tree_db
fi

rm -f $sqlite_db/*
rm -f $tree_db/*
for i in 65536 32768 16384 8192 4096 2048 1024; do
	# no compression in sqlite3
	./db_bench_sqlite3 --benchmarks=affine --page_size=$i $generic_options --db=$sqlite_db
	rm -f $sqlite_db/*

	# tree db
	./db_bench_tree_db --benchmarks=affine --page_size=$i $generic_options --db=$tree_db --compression=0
	rm -f $tree_db/*
done



