#include "part_cnt_blck.h"

char PartControlBlock::format()
{
	Mutex dummy(mutex);

	if (wantToUnmount)	//neko hoce da skine particiju, pa ne moze da se formatira
		return 0;

	wantToFormat++;

	if (filesOpen > 0)		//wait
	{
		if (wantToFormat == 1)		//prvi koji zeli da formatira
		{
			dummy.signal();
			formatBinSem->waitB();
			dummy.wait();
			
			formatBV();
			formatRI();
			wantToFormat--;
		}
		else 
		{
			dummy.signal();
			formatBinSem->waitB();
			dummy.wait();		//ostali koji zele da formatiraju
			wantToFormat--;
		}
	}
	else			//nema otvorenih fajlova, pa moze odmah da se formatira
	{
		formatBV();
		formatRI();
		wantToFormat = 0;
	}

	return 1;
}

void PartControlBlock::prepareUnmount()
{
	Mutex dummy(mutex);

	wantToUnmount++;

	if (filesOpen > 0)		//wait
	{
		if (wantToUnmount == 1)		//prvi koji zeli da skine particiju
		{
			dummy.signal();
			unmountBinSem->waitB();
			dummy.wait();
			//unmount
			wantToUnmount--;
		}
		else
		{
			dummy.signal();
			unmountBinSem->waitB();
			dummy.wait();		//ostali koji zele da je skinu
			wantToUnmount--;
		}
	}
	wantToUnmount--;
	
}

bool PartControlBlock::checkPremision()
{
	Mutex dummy(mutex);
	if (wantToUnmount)
		return false;	//neko hoce da skine particiju, pa nije dozvoljeno otvarnjae fajlova
	
	if (wantToFormat)
		return false;
		//ovo je bilo bolje resenje, al nije po zahtevu zadatka
	//{	//neko zeli da formatira particiju, pa treba sacekati sa otvaranjem fajla
	//	dummy.signal();
	//	formatBinSem.waitB();
	//	dummy.wait();
	//}
		
	return true;		//nike nece da foramtatira ili demontira particiju, pa je OK
}

PartControlBlock::PartControlBlock(PartitionCache *part): BitVectCtrl(part, part->getNumOfClusters() % (ClusterSize * BYTE_SIZE) == 0 ?
	part->getNumOfClusters() / (ClusterSize * BYTE_SIZE) : part->getNumOfClusters() / (ClusterSize * BYTE_SIZE) + 1), RootVectCtrl(part,
		part->getNumOfClusters() % (ClusterSize * BYTE_SIZE) == 0 ?
		part->getNumOfClusters() / (ClusterSize * BYTE_SIZE) : part->getNumOfClusters() / (ClusterSize * BYTE_SIZE) + 1)
{
	filesOpen = 0;
	wantToFormat = 0;
	wantToUnmount = 0;

	formatBinSem = new BinarySemaphore();
	unmountBinSem = new BinarySemaphore();

	//formatSem = CreateSemaphore(NULL, 0, 32, NULL);
//	formatMutex = CreateSemaphore(NULL, 1, 32, NULL);

	//deleteSem = CreateSemaphore(NULL, 0, 32, NULL);
//	deleteMutex = CreateSemaphore(NULL, 1, 32, NULL);
	waitingToDelete = 0;


	mutex = CreateSemaphore(NULL, 1, 32, NULL);
	this->part = part;
}

void PartControlBlock::writePointer(Cluster * index, int no, ClusterNo pointer)
{
	for (int i = 0; i < INDEX_ENRY; i++)
		index->info[no*INDEX_ENRY + i] = pointer >> (i*BYTE_SIZE);
}

ClusterNo PartControlBlock::readPointer(Cluster * index, int no)
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

PartControlBlock::~PartControlBlock()
{
	//CloseHandle(deleteMutex);
	//CloseHandle(deleteSem);
	//CloseHandle(formatSem);
	//CloseHandle(formatMutex);
	CloseHandle(mutex);

	//delete part;	
	part = nullptr;
	delete formatBinSem;
	delete unmountBinSem;
}

void PartControlBlock::incOpenFiles()
{
	Mutex dummy(mutex);
	filesOpen++;
}

bool PartControlBlock::decOpenFiles()
{
	Mutex dummy(mutex);

	if (--filesOpen == 0)
	{
		formatBinSem->signalB();
		unmountBinSem->signalB();
	}
	
	if (filesOpen < 0)
	{
		filesOpen = 0;
		return false;
	}
	
	return true;
}

char PartControlBlock::deleteFile(Cluster * index, int old)
{
	if (!index || old == 0 || old == 1) 
		return 0;

	Cluster *IILevel = nullptr;
	ClusterNo ILev = 0;
	ClusterNo IILev = 0;

	for (int i = 0; i < I_LEV_DATA_POINTERS; i++)
	{
		ILev = readPointer(index, i);
		if (ILev != 0 && isAlocated(ILev)) 
			releseCluster(ILev);
		else
			break;
	}

	for (int j = I_LEV_DATA_POINTERS; j < I_LEV_DATA_POINTERS + II_LEV_POINTERS; j++)
	{
		ILev = readPointer(index, j);
		if (ILev != 0 && isAlocated(ILev))
		{
			IILevel = new Cluster();
			part->readCluster(ILev, IILevel->info);

			for (int i = 0; i < II_LEV_DATA_POINTERS; i++)
			{
				IILev = readPointer(IILevel, i);
				if (IILev != 0 && isAlocated(IILev)) 
					releseCluster(IILev);	//oslobadja svaki iz drugo nivoa
				else break;
			}
			releseCluster(ILev);	//oslobadja taj
		}
	}
	releseCluster(old);

	return 1;
}

char PartControlBlock::deleteRestOfFile(Cluster * index, int start)
{
	if (!index)
		return 0;

	Cluster *IILevel = nullptr;
	ClusterNo ILev = 0;
	ClusterNo IILev = 0;

	int startPoint = start / ClusterSize < I_LEV_DATA_POINTERS ? (start % ClusterSize == 0 ? (start / ClusterSize) : (start / ClusterSize + 1)) : I_LEV_DATA_POINTERS;

	for (int i = startPoint; i < I_LEV_DATA_POINTERS; i++)
	{
		ILev = readPointer(index, i);
		if (ILev != 0 && isAlocated(ILev))
			releseCluster(ILev);
		else
			break;
	}

	for (int j = startPoint < I_LEV_DATA_POINTERS ? I_LEV_DATA_POINTERS : 
		(start / ClusterSize - I_LEV_DATA_POINTERS) / II_LEV_POINTERS + I_LEV_DATA_POINTERS; j < I_LEV_DATA_POINTERS + II_LEV_POINTERS; j++)
	{
		ILev = readPointer(index, j);
		if (ILev != 0 && isAlocated(ILev))
		{
			IILevel = new Cluster();
			part->readCluster(ILev, IILevel->info);

			//sledeca instrukcija ce samo pri prvoj proveri biti u mogucnosti da da rezultat raylicit od 0
			startPoint = start / ClusterSize < I_LEV_DATA_POINTERS ? 0 : (start % ClusterSize == 0 ?
				(start / ClusterSize - I_LEV_DATA_POINTERS) % II_LEV_POINTERS : (start / ClusterSize - I_LEV_DATA_POINTERS) % II_LEV_POINTERS + 1);

			for (int i = startPoint; i < II_LEV_DATA_POINTERS; i++)
			{
				IILev = readPointer(IILevel, i);
				if (IILev != 0 && isAlocated(IILev))
					releseCluster(IILev);	//oslobadja svaki iz drugo nivoa
				else 
					break;
			}
			if(!((start / ClusterSize - I_LEV_DATA_POINTERS) / II_LEV_POINTERS + I_LEV_DATA_POINTERS == j))
				releseCluster(ILev);	//oslobadja taj
			start = 0;
		}else 
			break;
	}


	return 1;
}



