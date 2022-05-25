CC=g++
CFLAGS=-g -std=c++0x
TARGET=sortedness_workload_generator

all: workload_generator

workload_generator: 
	$(CC) $(CFLAGS) $(TARGET).cpp -o $(TARGET)

sample: 
	$(CC) $(CFLAGS) sample.cpp -o sample

clean:
	rm -rf $(TARGET)
	rm -rf sample