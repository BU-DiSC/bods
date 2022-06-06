CC=g++
CFLAGS=-g -std=c++0x
TARGET=sortedness_workload_generator

all: workload_generator

workload_generator: 
	$(CC) $(CFLAGS) $(TARGET).cpp -o $(TARGET)

sample: 
	$(CC) $(CFLAGS) sample.cpp -o sample

btree_insert_only: 
	$(CC) $(CFLAGS) btree_insert_only.cpp -o btree_insert_only

btree_mixed: 
	$(CC) $(CFLAGS) btree_mixed.cpp -o btree_mixed

new_workload_generator:
	$(CC) $(CFLAGS) work_gen_mod.cpp -o work_gen_mod

clean:
	rm -rf $(TARGET)
	rm -rf sample
	rm -rf work_gen_mod
	rm -rf *.o
	rm -rf btree_insert_only
	rm -rf btree_mixed