/**********
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/
#include "xcl2.hpp"
#include <vector>
#include <chrono>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DATA_SIZE 62500000

//Set IP address of FPGA
// #define IP_ADDR 0x0A01D498
// #define BOARD_NUMBER 0
// #define ARP 0x0A01D498

void wait_for_enter(const std::string &msg) {
    std::cout << msg << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main(int argc, char **argv) {
    if (argc < 2) {
        // Rx bytes = Tx byte (forwarding the data)
        std::cout << "Usage: " << argv[0] << " <XCLBIN File 1> [<local_FPGA_IP 2> <boardNum 3>] [<#useConn 4>] [<#RxByte 5> <RxPort 6>] [<TxIP 7> <TxPort 8> <expectedTxPkgCnt 9> <TxpkgWordCountTx 10>]" << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];

    cl_int err;
    cl::CommandQueue q;
    cl::Context context;

    cl::Kernel user_kernel;
    cl::Kernel network_kernel;

    uint32_t local_IP = 0x0A01D498;
    uint32_t boardNum = 1;

    if (argc >= 3)
    {
        std::string s = argv[2];
        std::string delimiter = ".";
        int ip [4];
        size_t pos = 0;
        std::string token;
        int i = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            ip [i] = stoi(token);
            s.erase(0, pos + delimiter.length());
            i++;
        }
        ip[i] = stoi(s); 
        local_IP = ip[3] | (ip[2] << 8) | (ip[1] << 16) | (ip[0] << 24);
    }

    if (argc >= 4)
    {
        boardNum = strtol(argv[3], NULL, 10);
    }

    printf("local_IP:%x, boardNum:%d\n", local_IP, boardNum);


    auto size = DATA_SIZE;
    
    //Allocate Memory in Host Memory
    auto vector_size_bytes = sizeof(int) * size;
    std::vector<int, aligned_allocator<int>> network_ptr0(size);
    std::vector<int, aligned_allocator<int>> network_ptr1(size);


    //OPENCL HOST CODE AREA START
    //Create Program and Kernel
    auto devices = xcl::get_xil_devices();

    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    int valid_device = 0;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context({device}, NULL, NULL, NULL, &err));
        OCL_CHECK(err,
                  q = cl::CommandQueue(
                      context, {device}, CL_QUEUE_PROFILING_ENABLE, &err));

        std::cout << "Trying to program device[" << i
                  << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
                  cl::Program program(context, {device}, bins, NULL, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i
                      << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err,
                      network_kernel = cl::Kernel(program, "network_krnl", &err));
            OCL_CHECK(err,
                      user_kernel = cl::Kernel(program, "hls_recv_send_slow", &err));
            valid_device++;
            break; // we break because we found a valid device
        }
    }
    if (valid_device == 0) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }
    
    wait_for_enter("\nPress ENTER to continue after setting up ILA trigger...");


    // Set network kernel arguments
    OCL_CHECK(err, err = network_kernel.setArg(0, local_IP)); // Default IP address
    OCL_CHECK(err, err = network_kernel.setArg(1, boardNum)); // Board number
    OCL_CHECK(err, err = network_kernel.setArg(2, local_IP)); // ARP lookup

    OCL_CHECK(err,
              cl::Buffer buffer_r1(context,
                                   CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   vector_size_bytes,
                                   network_ptr0.data(),
                                   &err));
    OCL_CHECK(err,
            cl::Buffer buffer_r2(context,
                                CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                vector_size_bytes,
                                network_ptr1.data(),
                                &err));

    OCL_CHECK(err, err = network_kernel.setArg(3, buffer_r1));
    OCL_CHECK(err, err = network_kernel.setArg(4, buffer_r2));

    printf("enqueue network kernel...\n");
    OCL_CHECK(err, err = q.enqueueTask(network_kernel));
    OCL_CHECK(err, err = q.finish());
    
    int32_t useConn = 1;
    if(argc >=5)
        useConn = strtol(argv[4], NULL, 10);

    // Rx
    int32_t basePortRx = 5001; 
    uint64_t rxByteCnt = 320000;

    if(argc >=6)
        rxByteCnt = strtol(argv[5], NULL, 10);

    if(argc >= 7)
        basePortRx = strtol(argv[6], NULL, 10);

    // Tx
    int32_t TxIPAddr = 0x0A01D46E;//alveo0
    int32_t basePortTx = 5002; 
    uint64_t expectedTxPkgCnt = 1024;
    int32_t numPacketWord = 16; // or 64, 16, etc.

    if (argc >= 8)
    {
        std::string s = argv[7];
        std::string delimiter = ".";
        int ip [4];
        size_t pos = 0;
        std::string token;
        int i = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            ip [i] = stoi(token);
            s.erase(0, pos + delimiter.length());
            i++;
        }
        ip[i] = stoi(s); 
        TxIPAddr = ip[3] | (ip[2] << 8) | (ip[1] << 16) | (ip[0] << 24);
    }

    if (argc >= 9)
    {
        basePortTx = strtol(argv[8], NULL, 10);
    }

    if (argc >= 10)
    {
        expectedTxPkgCnt = strtol(argv[9], NULL, 10);
    }

    if (argc >= 11)
    {
        numPacketWord = strtol(argv[10], NULL, 10);
    }

    //Set user Kernel Arguments
    //Set user Kernel Arguments
    int start_param = 16;

    int delay_cycle_per_pkt = 1000 * 1000;
    
    std::cout << "useConn: " << useConn << std::endl; 
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 0, useConn));
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 1, delay_cycle_per_pkt));

    std::cout << "basePortRx: " << basePortRx << std::endl; 
    std::cout << "rxByteCnt: " << rxByteCnt << std::endl; 

    OCL_CHECK(err, err = user_kernel.setArg(start_param + 2, basePortRx));
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 3, rxByteCnt));

    printf("TxIPAddr:%x \n", TxIPAddr);
    std::cout << "basePortTx: " << basePortTx << std::endl; 
    std::cout << "expectedTxPkgCnt: " << expectedTxPkgCnt << std::endl; 
    std::cout << "numPacketWord: " << numPacketWord << std::endl; 
    std::cout << "(calculated) expected Tx bytes: expectedTxPkgCnt * numPacketWord * 64: " << 
        expectedTxPkgCnt * numPacketWord * 64 << std::endl; 
    
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 4, TxIPAddr));
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 5, basePortTx));
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 6, expectedTxPkgCnt));
    OCL_CHECK(err, err = user_kernel.setArg(start_param + 7, numPacketWord));

    double durationUs = 0.0;

    //Launch the Kernel
    auto start = std::chrono::high_resolution_clock::now();
    printf("enqueue user kernel...\n");
    OCL_CHECK(err, err = q.enqueueTask(user_kernel));
    OCL_CHECK(err, err = q.finish());
    auto end = std::chrono::high_resolution_clock::now();
    durationUs = (std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() / 1000.0);
    printf("durationUs:%f\n",durationUs);
    //OPENCL HOST CODE AREA END    

    std::cout << "EXIT recorded" << std::endl;
}
