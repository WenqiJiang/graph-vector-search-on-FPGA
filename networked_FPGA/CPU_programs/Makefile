# hacc build server default gcc version is 9.4.0, does not support semaphore in c++20
# In case the following error occurs, /usr/bin/ld: final link failed: Input/output error, manually type in the g++ command
CC = /home/wejiang/.local/gcc-12.3.0/bin/g++
# CC = g++

CLAGS=-Wall -std=c++20
LINK = -lpthread
LINK_OMP = -fopenmp

all: FPGA_simulator \
	CPU_client_simulator \
	# CPU_to_single_FPGA \
	# host_multi_FPGA \

FPGA_simulator: FPGA_simulator.cpp
	${CC} ${CLAGS} FPGA_simulator.cpp ${LINK} -o FPGA_simulator.bin

CPU_client_simulator: CPU_client_simulator.cpp
	${CC} ${CLAGS} CPU_client_simulator.cpp ${LINK} ${LINK_OMP} -o CPU_client_simulator.bin

.PHONY: clean, cleanall

cleanall: clean

clean:
	rm FPGA_simulator.bin CPU_client_simulator.bin 
