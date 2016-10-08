#ifndef _FS_H_
#define _FS_H_

#include "kernel_fs.h"
#include "data_structs.h"
/*
typedef unsigned long BytesCnt;
typedef unsigned long EntryNum;

const unsigned long ENTRYCNT = 64;
const unsigned int FNAMELEN = 8;
const unsigned int FEXTLEN = 3;



typedef Entry Directory[ENTRYCNT];
*/
//class KernelFS;
//class Partition;
//class File;

class FS {
public:
	~FS();
	static char mount(Partition* partition); //montira particiju
											 // vraca dodeljeno slovo
	static char unmount(char part); //demontira particiju oznacenu datim
									// slovom vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	static char format(char part); //particija zadatu slovom se formatira;
								   // vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	static char readRootDir(char part, EntryNum n, Directory &d);
	//prvim argumentom se zadaje particija, drugim redni broj
	//validnog ulaza od kog se poèinje èitanje
	static char doesExist(char* fname); //argument je naziv fajla sa
										//apsolutnom putanjom

	static File* open(char* fname, char mode);
	static char deleteFile(char* fname);
protected:
	FS();
	static KernelFS *myImpl;
};

#endif // !1
