#include "glob_opn_file_table.h"


GlobOpenFileTable::GlobOpenFileTable()
{
	size = START_SIZE;
	table = new GOFTEntry[size];
//	for (int i = 0; i < size; table[i].free = true);
	mutex = CreateSemaphore(NULL, 1, 32, NULL);
}

GlobOpenFileTable::~GlobOpenFileTable()
{
//	delete table;
	CloseHandle(mutex);
}

int GlobOpenFileTable::checkForFile(char * path, PartControlBlock *pcb)
{
	Mutex dummy(mutex);

	return checkForFilePrivate(path, pcb);
}

int GlobOpenFileTable::checkForFilePrivate(char * path, PartControlBlock * pcb)
{
	char **info = PathTokenizer::getNameExtNoDot(path);
	for (int i = 0; i < size; i++)
	{
		if (table[i].free == true) continue;
		if (table[i].pcb != pcb) continue;
		if (strncmp(table[i].name, info[0], FNAMELEN) == 0)
			if (strncmp(table[i].ext, info[1], FEXTLEN) == 0) return i;
	}

	return -1;
}


int GlobOpenFileTable::createRewriteEntry(char * path, PartControlBlock *pcb, PartitionCache * part, Cluster * index, ClusterNo no, int entryNum, unsigned long size, char mode)
{

	char **info = PathTokenizer::getNameExtNoDot(path);
	int en = getFreeEntry();
	//	GOFen = en;		//OVO BI MOGLA DA BUDE POVRATNA VREDNOST

	table[en].CNT = 1;
	table[en].num = no;

	for (int i = 0; i < FNAMELEN; i++)
		table[en].name[i] = info[0][i];
	for (int i = 0; i < FEXTLEN; i++)
		table[en].ext[i] = info[1][i];

	table[en].index = index;
	table[en].part = part;
	table[en].pcb = pcb;
	table[en].entryNum = entryNum;
	table[en].size = size;
	table[en].mode = mode;
	
	table[en].IILevel = nullptr;
	table[en].IInum = 0;
	table[en].ILevLeft = I_LEV_DATA_POINTERS;
	table[en].IILevDataLeft = 0;		//bio je max alo treba ovako da bi ga u prvom upisu inic
	table[en].IILevLeft = II_LEV_POINTERS;			//bolje da ovde ide 0
													//filesOpen++; ovo se radi u pcb posle poziva ove funkcije
	return en;
}

void GlobOpenFileTable::update()
{
	GOFTEntry *newTable = new GOFTEntry[size + INCREMENT];
	
	for (int i = 0; i < size; i++)
		newTable[i] = table[i];
	
	for (int i = size; i < size + INCREMENT; newTable[i++].free = true);

	size += INCREMENT;
	delete table;
	table = newTable;
}

int GlobOpenFileTable::getFreeEntry()
{
	int res = -1;
	int i;
	for (i = 0; i < size;i++)
		if (table[i].free)
		{ 
			res = i; 
			break; 
		}
	if (res == -1) 
	{
		update(); 
		for (; i < size; i++)
			if (table[i].free) 
				res = i; 
	}
	table[res].free = false;
	return res;
}

int GlobOpenFileTable::getFreeEntryPublic()
{
	Mutex dummy = Mutex(mutex);
	int res = -1;
	int i;
	for (i = 0; i < size; i++)
		if (table[i].free)
		{
			res = i;
			break;
		}
	if (res == -1)
	{
		update();
		for (; i < size; i++)
			if (table[i].free)
				res = i;
	}
	table[res].free = false;
	return res;
}

void GlobOpenFileTable::returnEntry(int en)
{
	if(en<size && en>=0)
		table[en].free = true;
}

EntryNum GlobOpenFileTable::getRootEntryNum(int entry)
{
	return table[entry].entryNum;
}

GlobOpenFileTable::GOFTEntry * GlobOpenFileTable::getGOFTablePntr(int entry)
{
	return &table[entry];
}

void GlobOpenFileTable::writePointer(Cluster *index, int no, ClusterNo pointer)
{
	for (int i = 0; i < INDEX_ENRY; i++)
		index->info[no*INDEX_ENRY + i] = pointer >> (i*BYTE_SIZE);
}

ClusterNo GlobOpenFileTable::readPointer(Cluster *index, int no)
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


int GlobOpenFileTable::createEntry(char * path, PartControlBlock *pcb, char mode)
{
	//getAcess();
	Mutex dummy(mutex);		//konstruktor poziva wait, a destruktor signal

	EntryNum no = pcb->getFreeEntry();
	if (no == -1) return ~0;	//nema vise slobodnoh ulaza

	Entry *tmp = new Entry();
	char **info = PathTokenizer::getNameExtNoDot(path);

	ClusterNo indexCluster = pcb->allocateCluster();
	if (indexCluster == 0) return ~0; //nema slobodnih klastera

	//POPUNJAVANJE DIRENTRY-A
	for (int i = 0; i < FNAMELEN; i++)		//popunjavanje imena
		tmp->name[i] = info[0][i];

	for (int i = 0; i < FEXTLEN; i++)		//popnjavanje ekstenizije
		tmp->ext[i] = info[1][i];

	tmp->reserved = RESERVED_BIT;
	tmp->size = 0;							//trenutno je prazno

	tmp->indexCluster = indexCluster;

	pcb->writeDirEntry(no, tmp);

	//POPUNJAVANJE GOF TABELE
	int en = getFreeEntry();

	dummy.signal();
	if (mode == 'w' || mode == 'a')		//SINHRONIZACIJA
		AcquireSRWLockExclusive(table[en].sync);
	else
		AcquireSRWLockShared(table[en].sync);
	dummy.wait();

	for (int i = 0; i < FNAMELEN; i++)
		table[en].name[i] = info[0][i];
	for (int i = 0; i < FEXTLEN; i++)
		table[en].ext[i] = info[1][i];

	table[en].CNT = 1;
	table[en].num = indexCluster;

	table[en].index = new Cluster();		//otvara se novi klaster
	table[en].part = pcb->part;
	table[en].pcb = pcb;		
	table[en].entryNum = no;
	table[en].size = 0;						//tek se otvorio fajl
	table[en].mode = mode;


	table[en].IILevel = nullptr;
	table[en].IInum = 0;
	table[en].ILevLeft = I_LEV_DATA_POINTERS;
	table[en].IILevDataLeft = 0;		//bio je max alo treba ovako da bi ga u prvom upisu inic
	table[en].IILevLeft = II_LEV_POINTERS;			//bolje da ovde ide 0

	//regulisanje brojaca u pcb-u
//	pcb->setIndex( no, indexCluster);	//vec je upisano
	pcb->incOpenFiles();
	delete tmp;
	return en;
	//relese();
}

int GlobOpenFileTable::reopenEntry(char * path, PartControlBlock *pcb, int entryNum, char mode)
{//getAcess();
	Mutex dummy(mutex);

	//provera da li je fajl vec otvoren
	int en = checkForFilePrivate(path, pcb);
	if (en == -1)		//fajl nije vec otvoren
	{
		en = getFreeEntry();
		dummy.signal();
		if (mode == 'w' || mode == 'a')		//SINHRONIZACIJA
		{
			AcquireSRWLockExclusive(table[en].sync);
			dummy.wait();
		}
		else
		{
			AcquireSRWLockShared(table[en].sync);
			dummy.wait();
		}	
	}
	else   // ent!=-1 //fajl nije otvoren
	{
		dummy.signal();
		if (mode == 'w')		//OVO SE NE KORISTI NIKAD, BOLJE RESENJE, ALI STRUKTURA NE DOZVOLJAVA
		{	//treba obrisati stari fajl i napraviti novi
			AcquireSRWLockExclusive(table[en].sync);
			dummy.wait();
			if (table[en].free)
				table[en].free = false;
			else {	//seafty code
			
			}
				//en = getFreeEntry();
			//KernelFS::deleteFile(index, no, pcb);		//brise stari fajl
			//deleteOldFile
		//	int res = createRewriteEntry(path, pcb, part, index, no, entryNum, size, mode);
			
			//return res;
		}/////////////////////////////////////////
		else if (mode == 'a')
		{
			AcquireSRWLockExclusive(table[en].sync);	//MARKER X
			dummy.wait();
			if (table[en].free)
				table[en].free = false;
				//en = getFreeEntry();
		}
		else
		{
			
			AcquireSRWLockShared(table[en].sync);
			dummy.wait();

			if (table[en].CNT > 0) {	//	PREMESTENO ZBOG SINHRONIZACIJE
				++table[en].CNT;
				return en;			//samo se uveca broj citalaca i varca se nazad
			}
									//inace upravo je doslo do promene moda 
			if(table[en].free)
				table[en].free = false;
				//en = getFreeEntry();
		}
		
	}

	char **info = PathTokenizer::getNameExtNoDot(path);

	Cluster *index = new Cluster();
	Entry *dirEntry = pcb->readDirEntry(entryNum);
	ClusterNo indexNum = dirEntry->indexCluster;
	pcb->part->readCluster(indexNum, index->info);

	for (int i = 0; i < FNAMELEN; i++)
		table[en].name[i] = info[0][i];
	for (int i = 0; i < FEXTLEN; i++)
		table[en].ext[i] = info[1][i];

	table[en].CNT = 1;
	table[en].num = indexNum;
	table[en].index = index;
	table[en].part = pcb->part;
	table[en].pcb = pcb;
	table[en].entryNum = entryNum;
	table[en].size = dirEntry->size;
	table[en].mode = mode;

	int i;

	//prebrojava se broj data klastera I novoa
	table[en].ILevLeft = I_LEV_DATA_POINTERS;
	for (i = 0; i < I_LEV_DATA_POINTERS; i++)
	{
		//long int check = 0;
		//for (int j = 0; j < INDEX_ENRY; j++)
		//	check |= index->info[j + i * INDEX_ENRY];	//proverava da li su svi ulazi 0
		//if (check == 0) break;
		//else table[en].ILevLeft--;
		int check = readPointer(index, i);
		if (check != 0 && pcb->isAlocated(check))
			table[en].ILevLeft--;
		else
			break;
	}

	//prebrojava se broj klastera pokazivaca na II nivo
	table[en].IILevLeft = II_LEV_POINTERS;
	for (i = I_LEV_DATA_POINTERS; i < I_LEV_DATA_POINTERS + II_LEV_POINTERS; i++)
	{
		//long int check = 0;
		//for (int j = 0; j < INDEX_ENRY; j++)
		//	check |= index->info[j + i * INDEX_ENRY];	//proverava da li su svi ulazi 0
		//if (check == 0)
		//	break;
		//else table[en].IILevLeft--;
		int check = readPointer(index, i);
		if (check != 0 && pcb->isAlocated(check))
			table[en].IILevLeft--;
		else
			break;
	}

	//prebrojava se broj data klastera II nivoa
	if (table[en].IILevLeft != II_LEV_POINTERS)
	{
		table[en].IInum = readPointer(index, II_LEV_POINTERS - table[en].IILevLeft + I_LEV_DATA_POINTERS - 1);	//ucitava broj klastera II nivoa

		table[en].IILevel = new Cluster();
		table[en].part->readCluster(table[en].IInum, table[en].IILevel->info);	//ucitava se II nivo za podatke

		//prebrojava se koliko je ostalo mesta za II nivo
		table[en].IILevDataLeft = II_LEV_DATA_POINTERS;
		for (i = 0; i < II_LEV_DATA_POINTERS; i++)
		{
			//int check = 0;
			//for (int j = 0; j < INDEX_ENRY; j++)
			//	check |= table[en].IILevel->info[j + i * INDEX_ENRY];		//provera da li su svi ulazi 0
			//if (check == 0) break;
			//else table[en].IILevDataLeft--;
			int check = readPointer(table[en].IILevel, i);
			if (check != 0 && pcb->isAlocated(check))
				table[en].IILevDataLeft--;
			else
				break;
		}
	}
	else
	{
		table[en].IILevel = nullptr;					//postavlja se inicijalno stanje
		table[en].IILevLeft = II_LEV_POINTERS;
	}

	pcb->incOpenFiles();
	return en;
}

int GlobOpenFileTable::reopenGOFEntryWithDelete(char * path, PartControlBlock *pcb, int entryNum, char mode)
{
	int en = reopenEntry(path, pcb, entryNum, mode);

	if (table[en].IILevel != nullptr) {
		delete table[en].IILevel;	//moze samo da se obrise bez azuriranja jer nista nije ni moglo da se upise
	}

	pcb->deleteFile(table[en].index , table[en].num);	
	delete table[en].index;
	table[en].index = nullptr;

	EntryNum x = table[en].entryNum;
	pcb->clearDirEntry(x);

	Entry *tmp = new Entry();
	char **info = PathTokenizer::getNameExtNoDot(path);

	for (int i = 0; i < FNAMELEN; i++)
		tmp->name[i] = info[0][i];

	for (int i = 0; i < FEXTLEN; i++)
		tmp->ext[i] = info[1][i];

	tmp->reserved = RESERVED_BIT;
	tmp->size = 0;	//trenutno je prazno
	ClusterNo cluster = pcb->allocateCluster();
	if (cluster == 0)
		return ~0;
	tmp->indexCluster = cluster;

	pcb->writeDirEntry(x, tmp);		//upisuhe se novi ulaz u root

	Cluster *index = new Cluster();

	for (int i = 0; i < FNAMELEN; i++)
		table[en].name[i] = tmp->name[i];
	for (int i = 0; i < FEXTLEN; i++)
		table[en].ext[i] = tmp->ext[i];

	table[en].CNT = 1;
	table[en].num = tmp->indexCluster;

	table[en].index = index;
	table[en].part = pcb->part;
	table[en].entryNum = x;
	table[en].size = tmp->size;
	table[en].mode = mode;

	table[en].IILevel = nullptr;
	table[en].IInum = 0;
	table[en].ILevLeft = I_LEV_DATA_POINTERS;
	table[en].IILevDataLeft = 0;
	table[en].IILevLeft = II_LEV_POINTERS;

	delete tmp;

	return en;
}

char GlobOpenFileTable::deleteFile(char * path, PartControlBlock * pcb, int entryNum)
{
	Mutex dummy(mutex);

	//brise ime i ekstenziju iz root vektora da niko vise ne bi mogao da ih otvori
	pcb->clearDirEntryName(entryNum);
	pcb->clearDirEntryExt(entryNum);
	char mode = 'w';
	int en = checkForFilePrivate(path, pcb);
	if (en == -1)		//fajl nije vec otvoren
	{
		en = getFreeEntry();
		dummy.signal();
		
		AcquireSRWLockExclusive(table[en].sync);
		
		dummy.wait();
	}
	else   // ent!=-1 //fajl nije otvoren
	{
		
		dummy.signal();
		AcquireSRWLockExclusive(table[en].sync);
		dummy.wait();

		if (table[en].free)
			table[en].free = false;

	}

	char **info = PathTokenizer::getNameExtNoDot(path);

	Cluster *index = new Cluster();
	Entry *dirEntry = pcb->readDirEntry(entryNum);
	ClusterNo indexNum = dirEntry->indexCluster;
	pcb->part->readCluster(indexNum, index->info);

	for (int i = 0; i < FNAMELEN; i++)
		table[en].name[i] = info[0][i];
	for (int i = 0; i < FEXTLEN; i++)
		table[en].ext[i] = info[1][i];

	table[en].CNT = 1;
	table[en].num = indexNum;
	table[en].index = index;
	table[en].part = pcb->part;
	table[en].pcb = pcb;
	table[en].entryNum = entryNum;
	table[en].size = dirEntry->size;
	table[en].mode = mode;

	int i;

	//////////////////////////////ODAVDE NIJE POTREBNO POSTO SE SVAKAKO BRISE/////////////

	//prebrojava se broj data klastera I novoa
	table[en].ILevLeft = I_LEV_DATA_POINTERS;
	for (i = 0; i < I_LEV_DATA_POINTERS; i++)
	{
		long int check = 0;
		for (int j = 0; j < INDEX_ENRY; j++)
			check |= index->info[j + i * INDEX_ENRY];	//proverava da li su svi ulazi 0
		if (check == 0) break;
		else table[en].ILevLeft--;
	}

	//prebrojava se broj klastera pokazivaca na II nivo
	table[en].IILevLeft = II_LEV_POINTERS;
	for (i = I_LEV_DATA_POINTERS; i < I_LEV_DATA_POINTERS + II_LEV_POINTERS; i++)
	{
		long int check = 0;
		for (int j = 0; j < INDEX_ENRY; j++)
			check |= index->info[j + i * INDEX_ENRY];	//proverava da li su svi ulazi 0
		if (check == 0) break;
		else table[en].IILevLeft--;
	}

	//prebrojava se broj data klastera II nivoa
	if (table[en].IILevLeft != II_LEV_POINTERS)
	{
		table[en].IInum = readPointer(index, II_LEV_POINTERS - table[en].IILevLeft + I_LEV_DATA_POINTERS - 1);	//ucitava broj klastera II nivoa

		table[en].IILevel = new Cluster();
		table[en].part->readCluster(table[en].IInum, table[en].IILevel->info);	//ucitava se II nivo za podatke

																				//prebrojava se koliko je ostalo mesta za II nivo
		table[en].IILevDataLeft = II_LEV_DATA_POINTERS;
		for (i = 0; i < II_LEV_DATA_POINTERS; i++)
		{
			int check = 0;
			for (int j = 0; j < INDEX_ENRY; j++)
				check |= table[en].IILevel->info[j + i * INDEX_ENRY];		//provera da li su svi ulazi 0
			if (check == 0) break;
			else table[en].IILevDataLeft--;
		}
	}
	else
	{
		table[en].IILevel = nullptr;					//postavlja se inicijalno stanje
		table[en].IILevLeft = II_LEV_DATA_POINTERS;
	}

	if (table[en].IILevel != nullptr) {
		delete table[en].IILevel;	//moze samo da se obrise bez azuriranja jer nista nije ni moglo da se upise
	}
	///////////////////////////////DOVDE/////////////////////////////////////////////////////////
	pcb->deleteFile(table[en].index, table[en].num);
	pcb->clearDirEntry(table[en].entryNum);
	delete table[en].index;

	ReleaseSRWLockExclusive(table[en].sync);
	returnEntry(en);

	return 1;
}

int GlobOpenFileTable::redoEntry(char * path, PartControlBlock *pcb, int entryNum, char mode)
{
	Mutex dummy(mutex);

	//provera da li je fajl vec otvoren
	int en = getFreeEntry();
	
	char **info = PathTokenizer::getNameExtNoDot(path);

	Cluster *index = new Cluster();
	Entry *dirEntry = pcb->readDirEntry(entryNum);
	ClusterNo indexNum = dirEntry->indexCluster;
	pcb->part->readCluster(indexNum, index->info);

	for (int i = 0; i < FNAMELEN; i++)
		table[en].name[i] = info[0][i];
	for (int i = 0; i < FEXTLEN; i++)
		table[en].ext[i] = info[1][i];

	table[en].CNT = 1;
	table[en].num = indexNum;
	table[en].index = index;
	table[en].part = pcb->part;
	table[en].pcb = pcb;
	table[en].entryNum = entryNum;
	table[en].size = dirEntry->size;
	table[en].mode = mode;

	int i;

	//prebrojava se broj data klastera I novoa
	table[en].ILevLeft = I_LEV_DATA_POINTERS;
	for (i = 0; i < I_LEV_DATA_POINTERS; i++)
	{
		//long int check = 0;
		//for (int j = 0; j < INDEX_ENRY; j++)
		//	check |= index->info[j + i * INDEX_ENRY];	//proverava da li su svi ulazi 0
		//if (check == 0) break;
		//else table[en].ILevLeft--;
		int check = readPointer(index, i);
		if (check != 0 && pcb->isAlocated(check))
			table[en].ILevLeft;
		else
			break;
	}

	//prebrojava se broj klastera pokazivaca na II nivo
	table[en].IILevLeft = II_LEV_POINTERS;
	for (i = I_LEV_DATA_POINTERS; i < I_LEV_DATA_POINTERS + II_LEV_POINTERS; i++)
	{
		//long int check = 0;
		//for (int j = 0; j < INDEX_ENRY; j++)
		//	check |= index->info[j + i * INDEX_ENRY];	//proverava da li su svi ulazi 0
		//if (check == 0) break;
		//else table[en].IILevLeft--;
		int check = readPointer(index, i);
		if (check != 0 && pcb->isAlocated(check))
			table[en].IILevLeft--;
		else
			break;
	}

	//prebrojava se broj data klastera II nivoa
	if (table[en].IILevLeft != II_LEV_POINTERS)
	{
		table[en].IInum = readPointer(index, II_LEV_POINTERS - table[en].IILevLeft + I_LEV_DATA_POINTERS - 1);	//ucitava broj klastera II nivoa

		table[en].IILevel = new Cluster();
		table[en].part->readCluster(table[en].IInum, table[en].IILevel->info);	//ucitava se II nivo za podatke

																				//prebrojava se koliko je ostalo mesta za II nivo
		table[en].IILevDataLeft = II_LEV_DATA_POINTERS;
		for (i = 0; i < II_LEV_DATA_POINTERS; i++)
		{
			//int check = 0;
			//for (int j = 0; j < INDEX_ENRY; j++)
			//	check |= table[en].IILevel->info[j + i * INDEX_ENRY];		//provera da li su svi ulazi 0
			//if (check == 0) break;
			//else table[en].IILevDataLeft--;
			int check = readPointer(table[en].IILevel, i);
			if (check != 0 && pcb->isAlocated(check))
				table[en].IILevDataLeft--;
			else
				break;
		}
	}
	else
	{
		table[en].IILevel = nullptr;					//postavlja se inicijalno stanje
		table[en].IILevLeft = II_LEV_DATA_POINTERS;
	}

	return en;
}


void GlobOpenFileTable::closeEntry(int entry)
{
	Mutex dummy(mutex);
	if (table[entry].free)
		return;

	long size = table[entry].size;
	PartControlBlock *pcb = table[entry].pcb;

	if (--table[entry].CNT > 0  && table[entry].mode == 'r') //sta ako ovaj bas treba da ga zatvori
	{	
		ReleaseSRWLockShared(table[entry].sync);
		return;		//citanje nije mogo da promeni velicinu
	}

	table[entry].part->writeCluster(table[entry].num, table[entry].index->info);
	delete table[entry].index;

	if (table[entry].IILevel != nullptr)
	{
		table[entry].part->writeCluster(table[entry].IInum, table[entry].IILevel->info);
		delete table[entry].IILevel;
	}
	
	table[entry].part = nullptr;
	table[entry].pcb = nullptr;

	///pcb->closeGFOEntry(entry, table[entry].entryNum);
	if (table[entry].mode != 'r')
		pcb->updateFileSize(table[entry].entryNum, size);		//citanje nije mogo da promeni velicinu

	///dummy.signal();
	pcb->decOpenFiles();
	///dummy.wait();
	
	if (table[entry].mode == 'r')		//oslobadjanje resursa
	{
		ReleaseSRWLockShared(table[entry].sync);
	}
	else
	{
		ReleaseSRWLockExclusive(table[entry].sync);
	}
	///dummy.signal();
	
	returnEntry(entry);
	
}



void GlobOpenFileTable::GOFTEntry::operator=(GOFTEntry &en)
{
	this->free = en.free;
	this->index = en.index;
	for (int i = 0; i < FNAMELEN; i++)
		this->name[i] = en.name[i];
	for (int i = 0; i < FEXTLEN; i++)
		this->ext[i] = en.ext[i];
	this->part = en.part;
	this->pcb = en.pcb;
	this->entryNum = en.entryNum;
	this->size = en.size;
	this->mode = en.mode;


	this->CNT = en.CNT;
	this->IILevel = en.IILevel;
	this->IInum = en.IInum;
	this->ILevLeft = en.ILevLeft;
	this->IILevDataLeft = en.IILevDataLeft;
	this->IILevLeft = en.IILevLeft;
}

GlobOpenFileTable::GOFTEntry::GOFTEntry()
{
	sync = new SRWLOCK;
	InitializeSRWLock(sync);
	free = true;
}

GlobOpenFileTable::GOFTEntry::~GOFTEntry()
{
}


Entry* readDirEntry(EntryNum no, char *info)		//PAZNJA !!! Ne vrsi se provera da li je ulaz validan ili nije
{
	Entry *res = new Entry();

	for (int i = 0; i < FNAMELEN; i++)
		res->name[i] = info[no*ROOT_ENTRY_OFFSET + i];

	for (int i = 0; i < FEXTLEN; i++)
		res->ext[i] = info[no*ROOT_ENTRY_OFFSET + FNAMELEN + i];

	res->reserved = info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN];

	unsigned char tmp = 0;
	res->indexCluster = 0;
	for (int i = 0; i < ROOT_FILE_INDEX_CLUSTER_SIZE; i++)
	{
		res->indexCluster <<= BYTE_SIZE;
		tmp = info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE - i - 1];
		res->indexCluster |= tmp;
	}

	res->size = 0;
	for (int i = 0; i < ROOT_FILE_SIZE_SIZE; i++)
	{
		res->size <<= BYTE_SIZE;
		tmp = info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE + ROOT_FILE_SIZE_SIZE - i - 1];
		res->size |= tmp;
	}

	return res;
}