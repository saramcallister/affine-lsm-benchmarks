#!/bin/bash
file="/mnt/db/affine_benchmark_data"
output="betree.out"

for keysize in 1024
do
	valuesize=$((keysize*4))
	echo
	echo
	echo $keysize
	echo
	echo

	#for block_size in "0.25" "0.5" 1 2 4 8 16 32 64 
	for block_size in 128
	#for block_size in "0.125" "0.0625"
	do
		[[ -e $file ]] && rm $file

		./affine_bench_prep_data $block_size $keysize $valuesize | tee -a $output
		./affine_bench_warm_cache $block_size $keysize $valuesize | tee -a $output
		
		#log="iostat_${block_size}.out"
		#date >> $log
		#echo >> $log
		#iostat -t -d /dev/sda3 1 180 >> $log &
		#sleep 10
		#echo "Query start" >> $log
		./affine_bench_query $block_size $keysize $valuesize | tee -a $output
		#echo "Insert start" >> $log 
		./affine_bench_insert $block_size $keysize $valuesize | tee -a $output
		#echo "Delete start" >> $log
		./affine_bench_delete $block_size $keysize $valuesize | tee -a $output
		#date >> $log
		#sleep 10
	done
done

