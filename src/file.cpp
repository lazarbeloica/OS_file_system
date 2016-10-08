#include "file.h"

File::~File()
{
	delete myImpl;
}

char File::write(BytesCnt n, char * buffer)
{
	if (myImpl->myMode == 'r')
		return 0;
	return myImpl->writeData(n, buffer);
}

BytesCnt File::read(BytesCnt n, char * buffer)
{
	return myImpl->readData(n, buffer);
}

char File::seek(BytesCnt n)
{
	return myImpl->seek(n);
}

BytesCnt File::filePos()
{
	return myImpl->filePos();
}

char File::eof()
{
	return myImpl->eof();
}

BytesCnt File::getFileSize()
{
	return myImpl->thisFile->size;
}

char File::truncate()
{
	return KernelFS::deleteRestOfFile(myImpl);
}

File::File()
{
}
