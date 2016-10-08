
#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <Windows.h>

class Mutex
{
private:
	HANDLE mutex;
public:

	Mutex(HANDLE& mutex)
	{
		this->mutex = mutex;
		WaitForSingleObject(this->mutex, INFINITE);
	}

	~Mutex()
	{
		ReleaseSemaphore(mutex, 1, NULL);
	}

	void wait()
	{
		WaitForSingleObject(this->mutex, INFINITE);
	}

	void signal()
	{
		ReleaseSemaphore(mutex, 1, NULL);
	}


};

#endif // !_MUTEX_H_

