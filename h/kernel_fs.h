#ifndef _KERNEL_FS_H_
#define _KERNEL_FS_H_

#include "data_structs.h"
#include "mount_part_table.h"
#include "glob_opn_file_table.h"
#include "part_cnt_blck.h"
#include "file.h"

class KernelFile;
class File;
class PartitionCache;

//sva polja i metode klase su static sto znaci da moze da postoji samo jedna instanca
class KernelFS
{
private:
	static void writePointer(Cluster*, int no, ClusterNo pointer);
	static ClusterNo readPointer(Cluster*, int no);

	friend class FS;
public:

	KernelFS();
	~KernelFS();
	friend PartControlBlock;
	static MountedPartitionTable MPT;
	static GlobOpenFileTable openFiles;

	static char mount(Partition *);
	static char unmount(char);
	static char format(char);
	static char readRootDir(char, EntryNum, Directory&);
	static char doesExist(char*);
	static File* openFile(char*, char);
	//static char deleteFile(Cluster *index, int, PartControlBlock *);
	static char deleteFile(char *path);
	static char deleteRestOfFile(KernelFile *);
};




#endif // !_KERNEL_FS_H_
