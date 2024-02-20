#include <algorithm>    // std::sort
#include <stdint.h>
#include <chrono>
#include <cassert>

#include "host.hpp"

#include "constants.hpp"
// Wenqi: seems 2022.1 somehow does not support linking ap_uint.h to host?
// #include "ap_uint.h"

#include <sys/stat.h>

long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

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
    int runtime_queue_size = 64;
	int input_array_size = 256;
	int iter_insert_sort = 100;
	int iter_pop = 0;
	
	size_t bytes_input_array = input_array_size * sizeof(float);
	size_t bytes_sorted_array = (input_array_size * 2 + 1) * sizeof(float);

	// input vecs
	std::vector<float, aligned_allocator<float>> input_array(bytes_input_array / sizeof(float));
	std::vector<float, aligned_allocator<float>> sorted_array(bytes_sorted_array / sizeof(float));

	// randomly generate input data
	for (int i = 0; i < input_array_size; i++) {
		input_array[i] = (float)rand() / (float)RAND_MAX;
	}

	// sort the array via CPU
	std::vector<float, aligned_allocator<float>> sw_sorted_array(bytes_input_array / sizeof(float));
	std::copy(input_array.begin(), input_array.end(), sw_sorted_array.begin());
	std::sort(sw_sorted_array.begin(), sw_sorted_array.end());
	
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
    OCL_CHECK(err, cl::Buffer buffer_input_array (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            bytes_input_array, input_array.data(), &err));

	// out
	OCL_CHECK(err, cl::Buffer buffer_sorted_array (context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,	
			bytes_sorted_array, sorted_array.data(), &err));

	std::cout << "Finish allocate buffer...\n";

	int arg_counter = 0;    
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(runtime_queue_size)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(input_array_size)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(iter_insert_sort)));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, int(iter_pop)));

	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_input_array));
	OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_sorted_array));
	   
    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
        // in
		buffer_input_array,
        },0/* 0 means from host*/));

    std::cout << "Launching kernel...\n";
    // Launch the Kernel
    auto start = std::chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_sorted_array},CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    auto end = std::chrono::high_resolution_clock::now();
    double duration = (std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() / 1000.0);

    std::cout << "Duration (including memcpy out): " << duration << " sec" << std::endl; 

	// Compare the results of the Device to the CPU
	std::cout << "Comparing the results of the Device to the CPU...\n";
	bool overall_result = true;
	for (int i = 0; i < runtime_queue_size; i++) {
		if (sorted_array[i] == sw_sorted_array[i]) {
			std::cout << "Match: " << sorted_array[i] << " (hw) = " << sw_sorted_array[i] << " (sw)" << std::endl;
		} else {
			std::cout << "Mismatch: " << sorted_array[i] << " (hw) != " << sw_sorted_array[i] << " (sw)" << std::endl;
			overall_result = false;
		}
	}
	if (overall_result) {
		std::cout << "Overall: Match" << std::endl;
	} else {
		std::cout << "Overall: Mismatch" << std::endl;
	}

    return  0;
}
