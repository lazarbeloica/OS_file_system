#include "partition_cache.h"


PartitionCache::PartitionCache(Partition *p) :part(p) {}

void PartitionCache::stDirty(int en)
{
	cache[en].dirty = 1;
}

void PartitionCache::clDirty(int en)
{
	cache[en].dirty = 0;
}

bool PartitionCache::isDirty(int en)
{
	return cache[en].dirty != 0;
}

void PartitionCache::clLRU(int en)
{
	cache[en].LRU = 0;
}

void PartitionCache::updateLRU(int toUpdate)
{
	
	for (int i = 0; i < CACHE_SIZE; i++)
	{
		if (i != toUpdate && cache[i].LRU != 0 && cache[i].LRU > cache[toUpdate].LRU)
			cache[i].LRU >>= 1;
	}

	cache[toUpdate].LRU = LRU_START_VAL;
}

int PartitionCache::findVictim()
{
	int victim = 0;
	unsigned char min = ~0;
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (cache[i].LRU < min)
			min = cache[victim = i].LRU;
	}

	return victim;
}

int PartitionCache::expell(int en)
{
	if (cache[en].LRU == 0)
		return en;
		
	int res = 1;
	if (cache[en].LRU != 0 && isDirty(en)) {
		res = part->writeCluster(cache[en].clustNum, cache[en].block.info);	//azuriranje starog bloka
	}
	if (res == 0)
		return -1;
	clDirty(en);
	clLRU(en);
	return 1;
}

int PartitionCache::import(ClusterNo newBlck)
{
	int entry = findVictim();
	if (expell(entry) == -1)	//doslo je do greske i blok nije izbacen!
		return -1;
	
	//popunjavanje ulaza
	cache[entry].clustNum = newBlck;
	cache[entry].dirty = 0;
	cache[entry].LRU = 0;		//nula je jer ce da se azuriranje da se odradi u update metodi
	if (!part->readCluster(newBlck, cache[entry].block.info))
		return -1;	//doslo je do greske

	return entry;
}

int PartitionCache::findEntry(ClusterNo blckNo)
{
	
	for (int i = 0; i < CACHE_SIZE; i++)
	{
		if (cache[i].LRU != 0 && cache[i].clustNum == blckNo)	//ako je 0 taj ulaz je nevazeci! 
			return i;
	}

	return -1;
}

PartitionCache::PartitionCache(char *conf_file)
{
#ifdef TEST1
	cout << "Usao u PartitionCache::PartitionCache" << endl;
#endif // TEST1
	part = new Partition(conf_file);
}

ClusterNo PartitionCache::getNumOfClusters() const
{
	return part->getNumOfClusters();
}

int PartitionCache::readCluster(ClusterNo no, char *buffer)
{
	Mutex dummy(mutex);

	int en = findEntry(no);
	if (en == -1)
		if ((en = import(no)) == -1)
			return 0;		//dogodila se greska!

	for (int i = 0; i < ClusterSize; i++)
	{
		buffer[i] = cache[en].block.info[i];
	}
	updateLRU(en);

	return 1;
	//return part->readCluster(no, buffer);
}

int PartitionCache::writeCluster(ClusterNo no, const char *buffer)
{
	Mutex dummy(mutex);

	int en = findEntry(no);
	if (en == -1)
		if ((en = import(no)) == -1)
			return 0;		//dogodila se greska!
	
	for (int i = 0; i < ClusterSize; i++)
	{
		cache[en].block.info[i] = buffer[i];
	}
	updateLRU(en);
	stDirty(en);

	return 1;
	//return part->writeCluster(no, buffer);
}

PartitionCache::~PartitionCache()
{

	for (int i = 0; i < CACHE_SIZE; i++)
	{
		expell(i);
	}


	part = nullptr;
	//delete part; nema brisanja particije, nisam je ja ni pravio
}