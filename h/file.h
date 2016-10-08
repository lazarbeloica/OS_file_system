#ifndef _FILE_H_
#define _FILE_H_


//#include "data_structs.h"
#include "kernel_file.h"
#include "data_structs.h"

//class KernelFile;
class File {
public:
	~File(); //zatvaranje fajla
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
private:
	friend class FS;
	friend class KernelFS;
	File(); 
	KernelFile *myImpl;
};


#endif // !_FILE_H_


