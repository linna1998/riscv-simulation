#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include <vector>
#include <math.h>
#include "storage.h"
using namespace std;

typedef struct CacheConfig_
{
	// int size;
	// int associativity;
	// int set_num; // Number of cache sets
	int write_through; // 0|1 for back|through
	int write_allocate; // 0|1 for no-alc|alc

	int s;
	int S;  // S = 2^s sets
	int e;
	int E;  // E = 2^e lines per set
	int b;
	int B;  // B = 2b bytes per cache block (the data)
	int C;  // C = S x E x B data bytes
} CacheConfig;

typedef struct Block
{
	bool valid;
	int tag;
	vector<bool> dirty;  // dirty==true means ot has been written  
	vector<int> data_block;
};
typedef struct Set
{
	vector<Block> data_set;
	vector<int> value;  // 用于判断LRU的替换策略
};

class Cache : public Storage
{
public:
	Cache() {}
	~Cache() {}

	// Sets & Gets
	void SetConfig(CacheConfig cc);
	void GetConfig(CacheConfig cc);
	// Set lower storage level.
	void SetLower(Storage *ll) { lower_ = ll; }
	void BuildCache();
	void PrintCache();
	// Main access process
	void HandleRequest(uint64_t addr, int bytes, int read,
		char *content, int &hit, int &time);

private:
	// Bypassing
	int BypassDecision();
	// Partitioning
	void PartitionAlgorithm(uint64_t addr, uint64_t& tag,
		uint64_t & set_index, uint64_t& block_offset);
	// Replacement
	int ReplaceDecision(uint64_t tag, uint64_t set_index);
	void ReplaceAlgorithm(uint64_t tag, uint64_t set_index, StorageStats & stats_, int & time);
	// Prefetching
	int PrefetchDecision();
	void PrefetchAlgorithm();

	CacheConfig config_;
	Storage *lower_;
	vector<Set> data_cache;
	DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
