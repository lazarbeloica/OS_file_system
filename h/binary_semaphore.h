#ifndef _BIN_SEM_H_
#define _BIN_SEM_H_

#include "mutex.h"
#include <Windows.h>

class BinarySemaphore
{
private:
	HANDLE mutexBin;
	HANDLE sem;
	int counter;
public:
	BinarySemaphore();
	~BinarySemaphore();

	void signalB();
	void waitB();


};

#endif // !_BIN_SEM_H_
