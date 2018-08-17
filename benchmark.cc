#include <cassert>
#include <limits.h>
#include <iostream>
#include <random>
#include <string>
#include <unistd.h>

#include "leveldb/db.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

using random_bytes_engine = std::independent_bits_engine<
	std::default_random_engine, CHAR_BIT, unsigned char>;
static const char *db_name = "/mnt/db/leveldb";
size_t SEQ_WRITES = 100000000;
size_t  num_queries = 100000;
int FLAGS_key_size = 128;
int FLAGS_value_size = 512;


void print_line_header(int table_size) {
	std::cout << table_size << "," << num_queries;
}

void print_result(double time_elapsed) {
	double micros_op = (time_elapsed * 1.0E6) / num_queries;
	double throughput = (FLAGS_value_size + FLAGS_key_size) * num_queries / ( 1.0E6 * time_elapsed); 
	std::cout << "," << time_elapsed << "," << micros_op;
	std::cout << "," << throughput;
}

std::string create_key(int key_num, char letter) {
	char key[100];
	snprintf(key, sizeof(key), "%016d%c", key_num, letter);
	std::string cpp_key = key;
	cpp_key.insert(cpp_key.begin(), FLAGS_key_size - 17, ' ');
	return cpp_key;  
}

int run_leveldb(int table_size) {
	/***************************SETUP********************************/
	/* Setup and open database */
	leveldb::DB* db;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = true;
	options.max_file_size = table_size;
	options.write_buffer_size = table_size;
	options.compression = leveldb::CompressionType::kNoCompression;
	//options.block_size = table_size;
	leveldb::Status status = leveldb::DB::Open(options, db_name, &db);
	if (!status.ok()) std::cerr << status.ToString() << std::endl;
	assert(status.ok());

	/* Create random number generator and initialize variables */
	random_bytes_engine rbe;
	std::vector<unsigned char> data(FLAGS_value_size);

	/* Setup DB with SEQ_WRITES * (FLAGS_value_size + FLAGS_data_size) amount of data */
	for (size_t i = 0; i < SEQ_WRITES; i++) {
		std::generate(begin(data), end(data), std::ref(rbe));
		std::string key = create_key(i, 'a');
		std::string value (data.begin(), data.end());
		leveldb::Status s = db->Put(leveldb::WriteOptions(), key, value);
		if (!s.ok()) {
			std::cout << "Assertion failed" << std::endl;
			std::cerr << s.ToString() << std::endl;
			return 1;
		}
	}

	/*****************WARM CACHE****************************/
	srand(12321);
	std::string value;
	for(size_t i = 0; i < 10*num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'a');
		leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
		assert(s.ok());
	}
	
	/*****************TEST RANDOM QUERIES****************************/
	struct timespec start, end;
	srand(12321);
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(size_t i = 0; i < num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'a');
		leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
		assert(s.ok());
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	print_result(elapsed);

	/*****************TEST RANDOM INSERTS****************************/
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(size_t i = 0; i < num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'b');
		leveldb::Status s = db->Put(leveldb::WriteOptions(), key, value);
		assert(s.ok());
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	print_result(elapsed);

	/*****************TEST RANDOM DELETES****************************/
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(size_t i = 0; i < num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'a');
		leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
		assert(s.ok());
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	print_result(elapsed);

	/****************************CLEANUP*****************************/
	delete db;
	return 0;

}

int run_rocksdb(int table_size) {
	/***************************SETUP********************************/
	/* Setup and open database */
	rocksdb::DB* db;
	rocksdb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = true;
	//rocksdb::BlockBasedTableOptions table_options;
	//table_options.metadata_block_size = table_size;
	//table_options.block_size = table_size;
	//options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
	options.target_file_size_base = table_size;
	options.write_buffer_size = table_size;
	options.db_write_buffer_size = table_size;
	options.max_open_files = 1000;
	options.compression = rocksdb::CompressionType::kNoCompression;
	rocksdb::Status status = rocksdb::DB::Open(options, db_name, &db);
	if (!status.ok()) {
		std::cerr << status.ToString() << std::endl;
	}
	assert(status.ok());

	/* Create random number generator and initialize variables */
	random_bytes_engine rbe;
	std::vector<unsigned char> data(FLAGS_value_size);

	/* Setup DB with SEQ_WRITES * (FLAGS_value_size + FLAGS_key_size) amount of data */
	for (size_t i = 0; i < SEQ_WRITES; i++) {
		std::generate(begin(data), end(data), std::ref(rbe));
		std::string key = create_key(i, 'a');
		std::string value (data.begin(), data.end());
		rocksdb::Status s = db->Put(rocksdb::WriteOptions(), key, value);
		if (!s.ok()) {
			std::cout << "Assertion failed" << std::endl;
			std::cout << s.ToString() << std::endl;
			return 1;
		}
	}
	
	/*****************WARM CACHE****************************/
	std::string value;
	srand(12321);
	for(size_t i = 0; i < 10*num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'a');
		rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &value);
		if (!s.ok()) std::cout << s.ToString() << std::endl;
		assert(s.ok());
	}

	/*****************TEST RANDOM QUERIES****************************/
	struct timespec start, end;
	srand(12321);
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(size_t i = 0; i < num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'a');
		rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &value);
		if (!s.ok()) std::cout << s.ToString() << std::endl;
		assert(s.ok());
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	print_result(elapsed);

	/*****************TEST RANDOM INSERTS****************************/
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(size_t i = 0; i < num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'b');
		rocksdb::Status s = db->Put(rocksdb::WriteOptions(), key, value);
		if (!s.ok()) std::cout << s.ToString() << std::endl;
		assert(s.ok());
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	print_result(elapsed);

	/*****************TEST RANDOM DELETES****************************/
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(size_t i = 0; i < num_queries; i++) {
		std::string key = create_key(rand() % SEQ_WRITES, 'a');
		rocksdb::Status s = db->Delete(rocksdb::WriteOptions(), key);
		if (!s.ok()) std::cout << s.ToString() << std::endl;
		assert(s.ok());
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	print_result(elapsed);

	/****************************CLEANUP*****************************/
	delete db;
	return 0;

}

int main(int argc, char** argv) {
	int leveldb = 0;
	int rocksdb = 0;
	int table_size = 0;
	int begin_line = 0;
	int c;

	while ((c = getopt (argc, argv, "lrbs:k:v:")) != -1 ) {
		switch (c) {
		    case 'l':
			leveldb = 1;
			break;
		    case 'r':
			rocksdb = 1;
			break;
		    case 's':
			table_size = std::stoi(optarg);
			break;
		    case 'b':
			begin_line = 1;
			break;
		    case 'k':
			FLAGS_key_size = std::stoi(optarg);
			break;
		    case 'v':
			FLAGS_value_size = std::stoi(optarg);
			break;
		    default:
			std::cout << c << optarg << std::endl;
			return 1;
		}
	}

	if (table_size == 0) {
		std::cout << "Need node size input. Use -s\n";;
		return 1;
	} else if ((leveldb && rocksdb) || (!leveldb && !rocksdb)) {
		std::cout << "Specify one of RocksDB (-r) and LevelDB (-l)\n";
		return 1;
	}

	SEQ_WRITES = 16 * 1024 * 1024 / (FLAGS_key_size + FLAGS_value_size) * 1024;
	num_queries = SEQ_WRITES / 1000;

	if (begin_line) {
		print_line_header(table_size);
	}

	if (leveldb) 
		return run_leveldb(table_size);
	if (rocksdb)
		return run_rocksdb(table_size);
	return 1;
}
