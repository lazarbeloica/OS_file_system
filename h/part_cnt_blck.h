#ifndef _PART_CNT_BLCK_H_
#define _PART_CNT_BLCK_H_

#include "data_structs.h"
#include "partition_cache.h"
#include "path_tokenizer.h"
#include "bit_vec_ctrl_blck.h"
#include "root_vec_ctrl.h"
#include "mutex.h"
#include "binary_semaphore.h"
//#include "glob_opn_file_table.h"
#include <windows.h>



class PartControlBlock : public BitVectCtrl, public RootVectCtrl
{
private:

	friend class GlobOpenFileTable;
	friend class KernelFS;
	friend class KernelFile;
	PartitionCache *part;
	//za sinhronizaciju pri formatiranju pcb-a
	int filesOpen;
	int wantToFormat;
	int wantToUnmount;
	BinarySemaphore *formatBinSem;
	BinarySemaphore *unmountBinSem;

//	HANDLE formatSem;
//	HANDLE formatMutex;
	//za sinhronizaciju pri brisanju fajla
//	HANDLE deleteSem;
//	HANDLE deleteMutex;
	int waitingToDelete;

	HANDLE mutex;
	
public:
	char format();		//1-uspeh, 0-neuspeh
	void prepareUnmount();
	bool checkPremision();
	PartControlBlock(PartitionCache*);

	void writePointer(Cluster *index, int no, ClusterNo pointer);
	ClusterNo readPointer(Cluster *index, int no);


	~PartControlBlock();

//	char createEntry(char *, int);	// stvara i popunjava jedan ulaz

//	void closeGFOEntry(int entry, EntryNum);		//zatvaranje jednog ulaza i u GOF i u root
//	int createGOFEntry(char * path, Cluster * index, ClusterNo no, int entryNum, unsigned long size, char mode);
//	int reopenGOFEntry(char * path, Cluster * index, ClusterNo no, int entryNum, unsigned long size, char mode);
//	int reopenGOFEntryWithDelete(char * path, Cluster * index, ClusterNo no, int entryNum, unsigned long size, char mode);

	void incOpenFiles();
	bool decOpenFiles();
	char deleteFile(Cluster *index, int old);
	char deleteRestOfFile(Cluster *index, int start);
};



#endif // !_PART_CNT_BLCK_H_
