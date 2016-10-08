#include "fs.h"

KernelFS* FS::myImpl = new KernelFS();

FS::~FS()
{
	delete myImpl;
}

char FS::mount(Partition * partition)
{
	return KernelFS::mount(partition);
}

char FS::unmount(char part)
{
	return KernelFS::unmount(part);
}

char FS::format(char part)
{
	return KernelFS::format(part);
}

char FS::readRootDir(char part, EntryNum n, Directory & d)
{
	return KernelFS::readRootDir(part, n, d);
}

char FS::doesExist(char * fname)
{
	return KernelFS::doesExist(fname);
}

File * FS::open(char * fname, char mode)
{
	return KernelFS::openFile(fname, mode);
}

char FS::deleteFile(char * fname)
{
	return KernelFS::deleteFile(fname);
}

FS::FS()
{
}
