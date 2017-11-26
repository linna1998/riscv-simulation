#include "memory.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
	char *content, int &hit, int &time)
{
	stats_.access_counter++;
	hit = 1;
	time = latency_.hit_latency + latency_.bus_latency;
	stats_.access_time += time;
}

