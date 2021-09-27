CC=g++
CFLAGS=-g -std=c++0x
TARGET=sortedness_workload_generator

all: workload_generator

workload_generator: 
	$(CC) $(CFLAGS) $(TARGET).cpp -o $(TARGET)


clean:
	rm -rf $(TARGET)