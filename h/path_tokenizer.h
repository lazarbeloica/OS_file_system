#ifndef _PATH_TOK_H_
#define _PATH_TOK_H_

#include <string>
#include "data_structs.h"

class PathTokenizer
{
public:
	static char getPartition(char*);
	static char* getFileName(char*);
	static char* getFileExt(char*);
	static char* getNameAndExt(char*);
	static char** getPartNameExtNoDot(char*);
	static char** getNameExtNoDot(char*);

	static char* fillNameToFit(char *);
	static char* fillExtToFit(char*);


};

#endif // !_PATH_TOK_H_

