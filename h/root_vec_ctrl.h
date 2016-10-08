#ifndef _ROOT_VEC_CTRL_H_
#define _ROOT_VEC_CTRL_H_

#include "data_structs.h"
#include "partition_cache.h"
#include "path_tokenizer.h"


class RootVectCtrl {
//public:
protected:
	Cluster rootIndex;
	int ROOT_IND_POSS;

	PartitionCache *part;

	void update();			//OK
public:

	RootVectCtrl(PartitionCache *, int);		//OK
	~RootVectCtrl();		//OK

	void writeDirEntry(EntryNum&, Entry*);	//OK
	Entry* readDirEntry(EntryNum);	//OK    //nijedna metoda ne proverava entry num, to ce da rade public metode
	
	void clearDirEntry(EntryNum);	//OK
	void clearDirEntryName(EntryNum);
	void clearDirEntryExt(EntryNum);
	void copyEnry(Entry&, Entry&);	//OK

	EntryNum checkForName(char*, char*);	//OK	//vraca broj ulaza ili -1 ako ne postoji
	void updateFileSize(int entryNum, unsigned long size);		//OK
	void setIndex(int entryNum, EntryNum index);		//OK

	void formatRI();		//OK


	EntryNum getFreeEntry();	//OK		//-1 je kod da je sve puno
//	char createEntry(char *, int);	// stvara i popunjava jedan ulaz
	
	EntryNum doesExist(char *);		//OK	//vraca -1 ako ne postoji

	char readRootDir(EntryNum no, Directory &d);		//OK
};


#endif // !1
