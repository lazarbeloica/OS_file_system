#ifndef  _GLOB_OPN_FILE_TABLE_H_
#define _GLOB_OPN_FILE_TABLE_H_

#include "data_structs.h"
#include "path_tokenizer.h"
#include "partition_cache.h"
#include "mutex.h"
//#include "kernel_fs.h"		//MOZE DA PRAVI PROBLEME!!!
#include <Windows.h>
#include <windows.h>
#include "part_cnt_blck.h"

class PartitionCache;


class GlobOpenFileTable
{

	friend class PartControlBlock;
	friend class KernelFS;
	friend class KernelFile;
public:
	struct GOFTEntry
	{
		bool free;
		Cluster *index;
		ClusterNo num;		//broj index klastera
		char name[FNAMELEN];
		char ext[FEXTLEN];
		PartitionCache *part;
		PartControlBlock *pcb;
		int entryNum;		//ulaz u direktorijumu
		unsigned long size;		//velicina u Byte
		char mode;				//mod citanja

		SRWLOCK *sync;			//POKAZIVAC NA SWR

		unsigned int CNT;
		Cluster *IILevel;	//pok. na indeks II nivoa
		ClusterNo IInum;
		int ILevLeft;		//ostalo mesta za pok na DATA
		int IILevDataLeft;	//ostalo mesta za DATA u II nivou
		int IILevLeft;		//ostalo mesta za pok na II nivo
							//lokacija POKAYIVAC NA POCETAK I KRAJ FAJLA
							//semafori za readers/writers problem
		void operator=(GOFTEntry&);

		GOFTEntry();		//neophodno zbog otvaranja semafora
		~GOFTEntry();		//neophodno zbog zatvaranja semafora
	};


	int createEntry(char*path, PartControlBlock*, char mode); //poslednji arg cuva mesto u tabeli  ybog yatvaranja
	int reopenEntry(char*path, PartControlBlock*, int entryNum, char mode);
	int reopenGOFEntryWithDelete(char * path, PartControlBlock * pcb, int entryNum, char mode);
	char deleteFile(char * path, PartControlBlock * pcb, int entryNum);
	
	int redoEntry(char * path, PartControlBlock *pcb, int entryNum, char mode);
	
	//obavezno prvo proveriti CNT==0
	void closeEntry(int entry);
	//bool doesExist(char *path);

	GlobOpenFileTable();		//OK
	~GlobOpenFileTable();
	int checkForFile(char *path, PartControlBlock *pcb);																					
	
	int createRewriteEntry(char*path, PartControlBlock*, PartitionCache *part, Cluster *index, ClusterNo, int entryNum, unsigned long size, char mode);
															
private:
	void update();		//OK
	int getFreeEntry();		//OK
	int getFreeEntryPublic();
	void returnEntry(int);	//OK
	EntryNum getRootEntryNum(int entry);
	GOFTEntry *getGOFTablePntr(int entry);
	int checkForFilePrivate(char *path, PartControlBlock *pcb);


	//pointeri se upisuju ili citaju iz klastera index iz ulaza broj no, a upisuje se pointer
	void writePointer(Cluster*, int no, ClusterNo pointer); //OK
	ClusterNo readPointer(Cluster*, int no);	//OK

	HANDLE mutex;		//semafor koji se koristi za ostvarivanje efekta monitora
	int size;
	GOFTEntry *table;
};



#endif // ! _GLOB_OPN_FILE_TABLE_H_
