#ifndef _BIT_VCT_CTRL_H_
#define _BIT_VCT_CTRL_H_

#include "data_structs.h"
#include "partition_cache.h"


class BitVectCtrl		//OK
{
protected:
	Cluster** bitVector;
	int numOfBitVectors;

	ClusterNo freeCluster[F_CLUST_POOL_SIZE];
	int FCPointer;
	bool noFreeClusters;

	ClusterNo clustersInPartition;	//radi kontrole prekoracenja

	PartitionCache *part;
	void update();

//protected:
	BitVectCtrl(PartitionCache *, int);
	~BitVectCtrl();
		
	void updateFCPool();
public:
	const ClusterNo getEmptyCluster();			//vraca slobodan blok, 0 ako je puno
	void releseCluster(const ClusterNo&);			//oslobadja zadati klaster
	char allocateCluster(const ClusterNo&);		//zauzima blok, 1- ok, 0-error
	ClusterNo allocateCluster();						//vraca broj zauzteog klastera
	bool isAlocated(const ClusterNo);

	void formatBV();	//formatira bitVector
};


#endif // !_BIT_VCT_CTRL_H_

