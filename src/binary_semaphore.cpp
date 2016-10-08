#include "binary_semaphore.h"

BinarySemaphore::BinarySemaphore()
{
	mutexBin = CreateSemaphore(NULL, 1, 32, NULL);
	sem = CreateSemaphore(NULL, 0, 32, NULL);
	counter = 0;
}

BinarySemaphore::~BinarySemaphore()
{
	CloseHandle(mutexBin);
	CloseHandle(sem);
}

void BinarySemaphore::signalB()
{
	///WaitForSingleObject(mutexBin, INFINITE);
	Mutex dummy(mutexBin);

	if (counter)
		while (counter--)
			ReleaseSemaphore(sem, 1, NULL);

}

void BinarySemaphore::waitB()
{
	//WaitForSingleObject(mutexBin, INFINITE);
	Mutex dummy(mutexBin);
	counter++;
	
	dummy.signal();

	WaitForSingleObject(sem, INFINITE);
	
	dummy.wait();
}
