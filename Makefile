CXXFLAGS=-g -std=c++0x

all: work_gen_mod btree_insert_only btree_mixed sample sortedness_workload_generator

clean:
	rm -rf sample sortedness_workload_generator work_gen_mod btree_insert_only btree_mixed
