
#ifndef _PARTITION_CACHE_H_
#define _PARTITION_CACHE_H_

#include "part.h"
#include "mutex.h"
#include <Windows.h>
#define CACHE_SIZE 7
// za 7 ulaza se vodi evidencija, 0 je nepopunjen
#define LRU_START_VAL 64
		//1000 0000b

class PartitionCache
{
private:

	struct  CacheEntry
	{
		unsigned char LRU;
		Cluster block;
		char dirty;
		ClusterNo clustNum;

		CacheEntry() :LRU(0), dirty(0) {}
	};

	Partition *part;
	CacheEntry cache[CACHE_SIZE];		//LRU reg. je velicine jednog bajta, to je 8 bita => moguce je voditi evidenciju o najvise 8 ulaza

	void stDirty(int en);
	void clDirty(int en);
	bool isDirty(int en);
	void clLRU(int en);

	void updateLRU(int toUpdate);
	int findVictim();
	int expell(int en);				//vraca gresku (-1) ako blok nije izbacen
	int import(ClusterNo newBlck);		//vraca ulaz na kome se nalazi blok, ili gresku *(-1)

	int findEntry(ClusterNo blckNo);	//vraca broj ulaza u kesu gde se nalazi klaster sa zadatim brojem ili -1 ako takvog nema

	//sinhronizacija
	HANDLE mutex = CreateSemaphore(NULL, 1, 32, NULL);
public:
	PartitionCache(Partition *);
	PartitionCache(char *);

	ClusterNo getNumOfClusters() const;
	
	int readCluster(ClusterNo, char *buffer); 
	int writeCluster(ClusterNo, const char *buffer); 
	
	~PartitionCache();
};


#endif // !_PARTITION_CACHE_H_
