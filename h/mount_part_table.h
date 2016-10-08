#ifndef _MOUNT_PART_TABLE_H_
#define _MOUNT_PART_TABLE_H_

#include "data_structs.h"
#include "part_cnt_blck.h"
#include "mutex.h"
#include <Windows.h>

class PartControlBlock;

class MountedPartitionTable
{
private:
//	friend class KernelFS;
//	friend class PartControlBlock;

	HANDLE mutex;		//OPREZ, IMA POTPOZIVA!!
	
	char free[NUM_OF_PARTS + 1];
	unsigned int sp;
	PartControlBlock* mounted[NUM_OF_PARTS];
	
public:	
	char getFreeMPoint(); //0 za prazno
	void returnMountPoint(char);	

	

	MountedPartitionTable();
	~MountedPartitionTable();
	PartControlBlock* getPCBPointer(char);	//vraca particiju sa datim slovom, 0 za gresku
	
	char mountPCBpointer(PartControlBlock*); //vraca 0 ako nema prostora
	int unmountPCBPointer(char);		//!!!POSTOJI OPASNOST OD DEDLOCK-A		//0 doslo je do greske, nema te particije
};




#endif // !_MOUNT_PART_TABLE

