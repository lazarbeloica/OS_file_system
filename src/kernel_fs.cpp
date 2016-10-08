#include "kernel_fs.h"


MountedPartitionTable KernelFS::MPT = MountedPartitionTable();

GlobOpenFileTable KernelFS::openFiles = GlobOpenFileTable();

void KernelFS::writePointer(Cluster *index, int no, ClusterNo pointer)
{
	for (int i = 0; i < INDEX_ENRY; i++)
		index->info[no*INDEX_ENRY + i] = pointer >> (i*BYTE_SIZE);

}

ClusterNo KernelFS::readPointer(Cluster *index, int no)
{
	unsigned char tmp = 0;
	ClusterNo res = 0;
	for (int i = 0; i < INDEX_ENRY; i++)
	{
		res <<= BYTE_SIZE;

		tmp = index->info[(no + 1)*INDEX_ENRY - i - 1];
		res |= tmp;
	}

	return res;
}

KernelFS::KernelFS()
{

}

KernelFS::~KernelFS()
{
}

char KernelFS::mount(Partition *part)
{
#ifdef TEST1
	cout << "Usao u KernelFS::mount" << endl;
#endif // TEST1


	PartitionCache *pc = new PartitionCache(part);
	PartControlBlock *pcb = new PartControlBlock(pc);
	return MPT.mountPCBpointer(pcb);
//	return 1;  //uspesno se izvrsilo
}

char KernelFS::unmount(char c)
{
#ifdef TEST1
	cout << "Usao u KernelFS::unmount" << endl;
#endif // TEST1
	MPT.unmountPCBPointer(c);
	return 1;  //uspesno se izvrsilo
}

char KernelFS::format(char c)
{
	MPT.getPCBPointer(c)->format();
	return 1;	
}

char KernelFS::readRootDir(char part, EntryNum no, Directory &d)
{
	PartControlBlock *pcb = MPT.getPCBPointer(part);
	if (!pcb->checkPremision())
		return 0;
	return pcb->readRootDir(no, d);
}

char KernelFS::doesExist(char *path)
{
	char c = PathTokenizer::getPartition(path);
	if (c  == 0) return 0;
		
	PartControlBlock *p = MPT.getPCBPointer(c);
	if (p == nullptr) return 0;
	return p->doesExist(path)==0 ? 0 : 1;
}

File * KernelFS::openFile(char *path, char mode)
	{
	PartControlBlock *pcb = MPT.getPCBPointer(PathTokenizer::getPartition(path));
	if (pcb == nullptr)
		return 0;
	if (!pcb->checkPremision())
		return 0;		//nije moguce otvoriti fajl
	if (mode != 'w' && mode != 'a' && mode != 'r') 
		return 0;

	File *file = new File();
	KernelFile *kernelFile = new KernelFile();

	long int ent = pcb->doesExist(path);

	if (ent == -1 && (mode == 'r' || mode == 'a'))
		return 0;						//otvara se fajl koji ne postoji
	
	if(ent != -1)		//fajl postoji, ali nije otvoren za pisanje
	{
		int GOFent;
		if (mode == 'w')
			GOFent = openFiles.reopenGOFEntryWithDelete(path, pcb, ent, mode);
		else
			GOFent = openFiles.reopenEntry(path, pcb, ent, mode);
	
		kernelFile->GOFEntry = GOFent;
		kernelFile->thisFile = openFiles.getGOFTablePntr(GOFent);
		if (mode == 'a') 
			kernelFile->cursor = openFiles.table[GOFent].size;
		else if(mode == 'r' || mode == 'w')
			kernelFile->cursor = 0;
	}
	else //ovde se otvara, posto nikad nije otvoren
	{
		int GOFent = openFiles.createEntry(path, pcb, mode);
		kernelFile->GOFEntry = GOFent;
		kernelFile->thisFile = openFiles.getGOFTablePntr(GOFent);
		kernelFile->cursor = 0;			//prvi put se otvara 
	}

	kernelFile->myMode = mode;
	file->myImpl = kernelFile;
	return file;
}

char KernelFS::deleteFile(char * path)
{
	PartControlBlock *pcb = MPT.getPCBPointer(path[0]);
	if (!pcb) 
		return 0;

	char **info = PathTokenizer::getNameExtNoDot(path);
	EntryNum en = pcb->checkForName(info[0], info[1]);
	if (en == -1)
		return 0;

	return openFiles.deleteFile(path, pcb, en);

}

char KernelFS::deleteRestOfFile(KernelFile *kf)
{
	if (kf->myMode == 'r')
		return 0;
	Cluster *index = kf->thisFile->index;
	const EntryNum cursor = kf->cursor;
	//unsigned long size = kf->thisFile->size;
	PartControlBlock *pcb = kf->thisFile->pcb;

	if (kf->thisFile->IILevel != nullptr) {		//cuvanje stanja
		pcb->part->writeCluster(kf->thisFile->IInum, kf->thisFile->IILevel->info);
		delete kf->thisFile->IILevel;
	}
	
	if (!pcb->deleteRestOfFile(kf->thisFile->index, kf->cursor))
		return 0;

	//azuriranje stanja

	kf->thisFile->size = cursor;
	kf->thisFile->ILevLeft = (I_LEV_DATA_POINTERS - cursor / ClusterSize) < 0 ? 0 : (I_LEV_DATA_POINTERS - cursor / ClusterSize - 1);

	if (cursor / ClusterSize < I_LEV_DATA_POINTERS)			//brisu se svi pok II nivoa
	{	//sve se postavlja u stanje kao da nije ni bilo upisa u II nivo
		kf->thisFile->IILevel = nullptr;
		kf->thisFile->IInum = 0;
		kf->thisFile->IILevLeft = II_LEV_POINTERS;
		kf->thisFile->IILevDataLeft = 0;
	}
	else
	{	//ako se ne brisu svi pok II nivoa, treba ih postaviti u novonastalo stanje
		kf->thisFile->IInum = readPointer(index, ((cursor / ClusterSize) - I_LEV_DATA_POINTERS) / II_LEV_DATA_POINTERS + I_LEV_DATA_POINTERS);
		kf->thisFile->IILevel = new Cluster();
		pcb->part->readCluster(kf->thisFile->IInum, kf->thisFile->IILevel->info);
		kf->thisFile->IILevLeft = II_LEV_POINTERS - (((cursor / ClusterSize) - I_LEV_DATA_POINTERS) / II_LEV_DATA_POINTERS + 1);

		kf->thisFile->IILevDataLeft = II_LEV_DATA_POINTERS - (((cursor / ClusterSize) - I_LEV_DATA_POINTERS) % II_LEV_DATA_POINTERS + 1);
	}
	
	return 1;
}
