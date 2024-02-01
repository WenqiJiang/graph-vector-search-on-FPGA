#include <stdint.h>
#include <chrono>

#include "host.hpp"

#include "constants.hpp"
// Wenqi: seems 2022.1 somehow does not support linking ap_uint.h to host?
// #include "ap_uint.h"


int main(int argc, char** argv)
{
    cl_int err;
    // Allocate Memory in Host Memory
    // When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the hood user ptr 
    // is used if it is properly aligned. when not aligned, runtime had no choice but to create
    // its own host side buffer. So it is recommended to use this allocator if user wish to
    // create buffer using CL_MEM_USE_HOST_PTR to align user buffer to page boundary. It will 
    // ensure that user buffer is used when user create Buffer/Mem object with CL_MEM_USE_HOST_PTR 

    std::cout << "Allocating memory...\n";

    // in init
    const int query_num = 1;
	const int ef = 64;
	const int d = 128;
	const int max_level = 16;
	const int max_link_num_upper = 32;
	const int max_link_num_base = 32;
	const int entry_point_id = 0;
	const int num_db_vec = 1000 * 1000;
	
	size_t bytes_per_vec = d * sizeof(float);
	size_t bytes_entry_vector = bytes_per_vec;
	size_t bytes_query_vectors = query_num * bytes_per_vec;
	size_t bytes_db_vectors = num_db_vec * bytes_per_vec;
	size_t bytes_ptr_to_upper_links = 1000 * 1000;
	size_t bytes_links_upper = 1000 * 1000;
	size_t bytes_links_base = 1000 * 1000;
	size_t bytes_out_id = query_num * ef * sizeof(int);
	size_t bytes_out_dist = query_num * ef * sizeof(float);	

	// input vecs
	std::vector<float ,aligned_allocator<float>> entry_vector(bytes_entry_vector / sizeof(float));
	std::vector<float ,aligned_allocator<float>> query_vectors(bytes_query_vectors / sizeof(float));

	// db vec
	std::vector<float ,aligned_allocator<float>> db_vectors(bytes_db_vectors / sizeof(float));
	
	// links
	std::vector<int ,aligned_allocator<int>> ptr_to_upper_links(bytes_ptr_to_upper_links / sizeof(int));
	std::vector<int ,aligned_allocator<int>> links_upper(bytes_links_upper / sizeof(int));
	std::vector<int ,aligned_allocator<int>> links_base(bytes_links_base / sizeof(int));
	
	// output
	std::vector<int ,aligned_allocator<int>> out_id(bytes_out_id / sizeof(int));
	std::vector<float ,aligned_allocator<float>> out_dist(bytes_out_dist / sizeof(float));


// OPENCL HOST CODE AREA START

    std::vector<cl::Device> devices = get_devices();
    cl::Device device = devices[0];
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    //Creating Context and Command Queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device);

    // Import XCLBIN
    xclbin_file_name = argv[1];
    cl::Program::Binaries vadd_bins = import_binary_file();

    // Program and Kernel
    devices.resize(1);
    cl::Program program(context, devices, vadd_bins);
    cl::Kernel krnl_vector_add(program, "vadd");

    std::cout << "Finish loading bitstream...\n";
    // in 
    OCL_CHECK(err, cl::Buffer buffer_entry_vector (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            bytes_entry_vector, entry_vector.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_query_vectors (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
			bytes_query_vectors, query_vectors.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_ptr_to_upper_links (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
			bytes_ptr_to_upper_links, ptr_to_upper_links.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_links_upper (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
			bytes_links_upper, links_upper.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_links_base (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
			bytes_links_base, links_base.data(), &err));

	// in & out (db vec is mixed with visited list)
	OCL_CHECK(err, cl::Buffer buffer_db_vectors (context,CL_MEM_USE_HOST_PTR,	
			bytes_db_vectors, db_vectors.data(), &err));

	// out
	OCL_CHECK(err, cl::Buffer buffer_out_id (context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
			bytes_out_id, out_id.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_out_dist (context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
			bytes_out_dist, out_dist.data(), &err));

	std::cout << "Finish allocate buffer...\n";

	int arg_counter = 0;    
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(query_num)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(ef)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(d)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(max_level)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(max_link_num_upper)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(max_link_num_base)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(entry_point_id)));

	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_entry_vector));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_query_vectors));

	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_db_vectors));

	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_ptr_to_upper_links));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_links_upper));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_links_base));

	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_out_id));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_out_dist));


    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
        // in
		buffer_entry_vector,
		buffer_query_vectors,
		buffer_ptr_to_upper_links,
		buffer_links_upper,
		buffer_links_base,
		// in & out
		buffer_db_vectors
        },0/* 0 means from host*/));

    std::cout << "Launching kernel...\n";
    // Launch the Kernel
    auto start = std::chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_out_id, buffer_out_dist},CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    auto end = std::chrono::high_resolution_clock::now();
    double duration = (std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() / 1000.0);

    std::cout << "Duration (including memcpy out): " << duration << " sec" << std::endl; 

    std::cout << "TEST FINISHED (NO RESULT CHECK)" << std::endl; 

    return  0;
}
