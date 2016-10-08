#include "mount_part_table.h"


MountedPartitionTable::~MountedPartitionTable()
{
	CloseHandle(mutex);
}

MountedPartitionTable::MountedPartitionTable()
{
	mutex = CreateSemaphore(NULL, 1, 32, NULL);
	free[0] = 0;	//kraj steka
	for (int i = 1; i <= NUM_OF_PARTS; i++)
		free[i] = ('Z'+1 - i);
	sp = NUM_OF_PARTS;	//ukayuje na poslednji zauzeti ulaz
	for (int i = 0; i < NUM_OF_PARTS; i++)
		mounted[i] = nullptr;		//inicijalizuje se tabela montirtanih particija
}

char MountedPartitionTable::getFreeMPoint()
{
#ifdef TEST1
	cout << "Usao u MTP getFreePoint" << endl;
#endif // TEST1
	if (free[sp] == 0) return 0;
	return free[sp--];
}

void MountedPartitionTable::returnMountPoint(char c)
{
#ifdef TEST1
	cout << "Usao u MTP returnFreePoint" << endl;
#endif // TEST1
	
	free[++sp] = c;
}

PartControlBlock* MountedPartitionTable::getPCBPointer(char c)
{
#ifdef TEST1
	cout << "Usao u MTP getPCBpointer" << endl;
#endif // TEST1
	Mutex mut = Mutex(mutex);
	return mounted[c - 'A'];
}

char MountedPartitionTable::mountPCBpointer(PartControlBlock *p)
{
#ifdef TEST1
	cout << "Usao u MTP rmountPCB" << endl;
#endif // TEST1
	Mutex mut = Mutex(mutex);

	char mp = getFreeMPoint();
	if (mp == 0) 
		return 0;		//nema vise mesta
	mounted[mp - 'A'] = p;

#ifdef TEST1
	cout << "MP "<<mp << endl;
#endif // TEST1
	return mp;
}

int MountedPartitionTable::unmountPCBPointer(char c)
{
#ifdef TEST1
	cout << "Usao u MTP unmountPCB" << endl;
#endif // TEST1
	Mutex mut = Mutex(mutex);

	if (c<'A' || c>'Z') 
		return 0;
	if (mounted[c - 'A'] == nullptr) 
		return 0;		//particija ne postoji
	PartControlBlock *p = mounted[c - 'A'];
	mut.signal();
	p->prepareUnmount();
	mut.wait();
	mounted[c - 'A'] = nullptr;
	returnMountPoint(c);
	delete p;
	return 1;
}