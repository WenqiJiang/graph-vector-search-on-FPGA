#pragma once

#define N_CHANNEL 4 // has to be 2^n

#define FLOAT_PER_AXI 16 // 512 bit / 32 bit = 16
#define INT_PER_AXI 16
#define BYTE_PER_AXI 64
const int float_per_axi = FLOAT_PER_AXI;

// #define D_MAX 1024
#define D 128

// largest 32-bit float: 3.4028237 × 10^38
const float large_float = 1E+20f; // 1E+20f is large enough for cosine similarity

// max queue sizes
const int hardware_result_queue_size = 64; // 128;
const int hardware_candidate_queue_size = 32; // 128;

// bloom filter setting: https://hur.st/bloomfilter/?n=2500&p=&m=64000&k=4
const int bloom_num_hash_funs = 3; 
#if N_CHANNEL == 1
const int bloom_num_bucket_addr_bits = 8 + 10; // 256 * 1024
#define CHANNEL_ADDR_BITS 0
#elif N_CHANNEL == 2
const int bloom_num_bucket_addr_bits = 7 + 10; // 128 * 1024
#define CHANNEL_ADDR_BITS 1
#elif N_CHANNEL == 4
const int bloom_num_bucket_addr_bits = 6 + 10; // 64 * 1024
#define CHANNEL_ADDR_BITS 2
#elif N_CHANNEL == 8
const int bloom_num_bucket_addr_bits = 5 + 10; // 32 * 1024
#define CHANNEL_ADDR_BITS 3
#elif N_CHANNEL == 16
const int bloom_num_bucket_addr_bits = 4 + 10; // 16 * 1024
#define CHANNEL_ADDR_BITS 4
#endif

const int bloom_num_buckets = 1 << bloom_num_bucket_addr_bits;

// async batch size tracking
const int hardware_async_batch_size = 64; // to infer BRAM

// debug signals per query
const int debug_size = 1;

// FIFO depth
const int depth_network_in = 512; // data FIFOs without wide data types

const int depth_data = 512; // data FIFOs without wide data types
const int depth_control = 16;
const int depth_fetched_vectors = 512; // 512-bit width to memory
const int depth_query_vectors = 128; // 512-bit width to memory