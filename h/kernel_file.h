#ifndef _KERNEL_FILE_H_
#define _KERNEL_FILE_H_

#include "data_structs.h"
#include "glob_opn_file_table.h"
#include "part_cnt_blck.h"
#include "kernel_fs.h"


/*
#define INDEX_ENRY 4
//velicina ulaza je 4B
#define I_LEV_DATA_POINTERS 256
#define II_LEV_POINTERS 256
		//512/2
#define II_LEV_DATA_POINTERS 512

class GlobOpenFileTable;
struct GlobOpenFileTable::GOFTEntry;
*/

 class KernelFile		//svaka nit ima licni kernel file	
{
//public:
private:

	friend class KernelFS;
	friend class File;
	char myMode;
	BytesCnt cursor;		//predstavlja broj bajtova
	int GOFEntry;
	GlobOpenFileTable::GOFTEntry *thisFile;	//pokazivac na ulaz u GOFT na ovaj fajl
	
	int writeData(BytesCnt, char*);
	BytesCnt readData(BytesCnt, char*);
	
	void writePointer(Cluster*, int no, ClusterNo pointer);
	ClusterNo readPointer(Cluster*, int no);
public:
	char writeNewDataByte(char&);	//alocira i upisuje jedan bajt
	char readDataByte();	//cita zadati bajt
	char seek(BytesCnt);
	BytesCnt filePos();
	BytesCnt eof();
	char truncate();

	KernelFile();
	~KernelFile();
};


#endif // !_KERNEL_FILE_H_
