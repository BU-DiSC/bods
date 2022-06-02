CC=g++
CFLAGS=-g -std=c++0x
TARGET=sortedness_workload_generator

all: workload_generator

workload_generator: 
	$(CC) $(CFLAGS) $(TARGET).cpp -o $(TARGET)

sample: 
	$(CC) $(CFLAGS) sample.cpp -o sample

new_workload_generator:
	$(CC) $(CFLAGS) work_gen_mod.cpp -o work_gen_mod

clean:
	rm -rf $(TARGET)
	rm -rf sample
	rm -rf work_gen_mod
	rm -rf *.o