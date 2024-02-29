// g++ -o bloom bloom_murmur2.cpp && ./bloom

#include <iostream>
#include <cstring>
#include <stdint.h>
#include <chrono>
#include <cassert> 

#include "murmur2.hpp"

class BloomFilter {

public:
	int num_buckets_;
	int num_hashes_;
	bool* byte_array_;

  BloomFilter(int num_buckets, int num_hashes) : num_buckets_(num_buckets), num_hashes_(num_hashes){
	byte_array_ = (bool*) malloc(num_buckets);
	memset(byte_array_, 0, num_buckets);
  }

  ~BloomFilter() {
	free(byte_array_);
  }

  void insert(int key) {
	for (int i = 0; i < num_hashes_; i++) {
	  uint32_t hash = MurmurHash2_KeyLen4((char*) &key, i + 1);
	  uint32_t bucket = hash % num_buckets_;
	  byte_array_[bucket] = true;
	}
  }

  bool contains(int key) {
	for (int i = 0; i < num_hashes_; i++) {
	  uint32_t hash = MurmurHash2_KeyLen4((char*) &key, i + 1);
	  uint32_t bucket = hash % num_buckets_;
	  if (!byte_array_[bucket]) {
		return false;
	  }
	}
	return true;
  }
};


int main() {

	// bloom filter calculator: https://hur.st/bloomfilter/?n=10000&p=0.0001&m=&k=4
	int num_buckets = 512000;
	int num_hashes = 5;
	int num_insertions = 10000;

	int num_checks = 1000 * num_insertions;

	// instantiate bloom filter
	BloomFilter bloom(num_buckets, num_hashes);
	
	// insert values
	std::cout << "Inserting values..." << std::endl;
	for (int i = 0; i < num_insertions; i++) {
    	uint32_t key = i;
		bloom.insert(key);
	}

	// make sure all of the inserted values are conflict
	for (int i = 0; i < num_insertions; i++) {
    	uint32_t key = i;
		assert(bloom.contains(key));
	}

	// check conflicts
	int conflict_cnt = 0;
	for (int i = num_insertions; i < num_insertions + num_checks; i++) {
		uint32_t key = i;
		bool conflict = bloom.contains(key);
		if (conflict) {
			conflict_cnt += 1;
		}
	}
	std::cout << "Conflict count: " << conflict_cnt << std::endl;
	if (conflict_cnt > 0) {
		std::cout << "Conflict rate: 1 out of " << num_checks / conflict_cnt << std::endl;
	}

	return 0;
}
