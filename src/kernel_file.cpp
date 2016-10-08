#include "kernel_file.h"
#include <iostream>
using namespace std;


KernelFile::KernelFile()
{
}

KernelFile::~KernelFile()
{
	KernelFS::openFiles.closeEntry(this->GOFEntry);
}


int KernelFile::writeData(BytesCnt num, char *buffer)
{
	for (int i = 0; i < num; i++) {
		if (!writeNewDataByte(buffer[i]))
		{
			cursor -= i;	//ponistava se upis
			KernelFS::deleteRestOfFile(this);
			return 0;
		}
		
		if (i == 51195)
			int x = i;
	}
	return 1;
}

BytesCnt KernelFile::readData(BytesCnt cur, char *buffer)
{
	for (int i = 0; i < cur; i++)
	{
		if (cursor >= thisFile->size)
		{
			cursor = thisFile->size;
			return i;
		}
		buffer[i] = readDataByte();
	}
	return cur;		//procitao je sve
}

void KernelFile::writePointer(Cluster *index, int no, ClusterNo pointer)
{
	for (int i = 0; i < INDEX_ENRY; i++)
		index->info[no*INDEX_ENRY + i] = pointer >> (i*BYTE_SIZE);
}

ClusterNo KernelFile::readPointer(Cluster *index, int no)
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

char KernelFile::writeNewDataByte(char &buffer)
{
	PartControlBlock *pcb = thisFile->pcb;
	int whatByte = cursor % ClusterSize;
	int whatLocation = cursor / ClusterSize;
	Cluster *whatIndex;

	if (cursor == thisFile->size) {		//ako nije ispunjen ovaj uslov, nije kraj fajla
		if (!whatByte)		//neophodna je alokacija nove lokacije
		{
			Cluster *whatIndex;


			if (!(whatLocation < I_LEV_DATA_POINTERS))	//lokacija se nalazi medju indeksima II nivoa
			{
				if (thisFile->IILevDataLeft == 0 || thisFile->IILevel == nullptr)	//nema vise prostora u II nivou
				{									//neophidna je alokacija

					if (thisFile->IILevel != nullptr)	//ne radi se o prvom upisu u II nivo
					{									//cuva se stari IILevel pokazivac
						pcb->part->writeCluster(this->thisFile->IInum, thisFile->IILevel->info);		//cuva se pokazivac II nivoa
						delete thisFile->IILevel;
					}

					if (thisFile->IILevLeft == 0)
						return 0; //nema vise mesta za pokazivace II nivoa

					ClusterNo tmp;
					if (!(tmp = pcb->allocateCluster()))
						return 0;	//greska pri alokaciji klastera na disku

					thisFile->IInum = tmp;			//pokazivac na klaster II ivoa
					writePointer(thisFile->index, II_LEV_POINTERS - thisFile->IILevLeft + I_LEV_DATA_POINTERS, tmp);	//kodovanje pokazivaca, arg no pokazuje na sledece slobodno mesto u II 256 lokacija
					thisFile->IILevel = new Cluster();		//alokacija novog klastera za podatke u OM
					thisFile->IILevLeft--;
					thisFile->IILevDataLeft = II_LEV_DATA_POINTERS;			//raspolozivo je svih 512 lokacija jer je tak alocirano
				}

				whatIndex = thisFile->IILevel;		//upisuje se u indeks II nivoa
				whatLocation = (whatLocation - I_LEV_DATA_POINTERS) % II_LEV_DATA_POINTERS;
				thisFile->IILevDataLeft--;
			}

			else
			{
				whatIndex = thisFile->index;
				thisFile->ILevLeft--;
			}

			//SAD IDE ALOKACIJA LOKACIJE I UPIS

			ClusterNo pointer;//	//rezervacija novog klastera za LOKACIJU
			if (!(pointer = pcb->allocateCluster()))
				return 0;	//greska

			writePointer(whatIndex, whatLocation, pointer);		//upisan novi klaster na odgovarajucu lokaciju
			//moze ovde da se formatira novoalocirani klaster, ali samo usporava a nije neophodno

		}
	}

	//ostaje jos upis u klaster

	if (cursor == thisFile->size) {								//radi se o upisu na kraj fajla, tj. prosirivanju fajla
		if (cursor / ClusterSize < I_LEV_DATA_POINTERS)
		{
			whatLocation = I_LEV_DATA_POINTERS - thisFile->ILevLeft - 1;
			whatIndex = thisFile->index;
		}
		else
		{
			whatLocation = II_LEV_DATA_POINTERS - thisFile->IILevDataLeft - 1;
			whatIndex = thisFile->IILevel;
		}
	}

	else {						//radi se o prepisivanju preko fajla
		if (cursor / ClusterSize < I_LEV_DATA_POINTERS)
		{
			whatLocation = cursor / ClusterSize;
			whatIndex = thisFile->index;
		}
		else {
			int tmp = readPointer(thisFile->index, (cursor / ClusterSize - I_LEV_DATA_POINTERS) / II_LEV_DATA_POINTERS + I_LEV_DATA_POINTERS);
			if(tmp == thisFile->IInum)		//IILevel je vec ucitan u memoriju
			{
				whatLocation = (cursor / ClusterSize - I_LEV_DATA_POINTERS) % II_LEV_DATA_POINTERS;
				whatIndex = thisFile->IILevel;
			}
			else {		//indeks drugog nivoa nije ucitan u memoriju
				whatLocation = (cursor / ClusterSize - I_LEV_DATA_POINTERS) % II_LEV_DATA_POINTERS;
				Cluster *tmpIILev = new Cluster();
				pcb->part->readCluster(tmp, tmpIILev->info);
				whatIndex = tmpIILev;

				Cluster *writeTo = new Cluster();
				ClusterNo ent = readPointer(whatIndex, whatLocation);

				pcb->part->readCluster(ent, writeTo->info);
				writeTo->writeInByte(whatByte, buffer);
				pcb->part->writeCluster(ent, writeTo->info);

				delete tmpIILev;
				delete writeTo;
				return 1;
			}
		}
	}

	Cluster *writeTo = new Cluster();
	ClusterNo ent = readPointer(whatIndex, whatLocation);

	pcb->part->readCluster(ent, writeTo->info);
	writeTo->writeInByte(whatByte, buffer);
	pcb->part->writeCluster(ent, writeTo->info);
	
	if (cursor == thisFile->size)	//ako je dopisano na kraj fajla, neophodno za slucaj kada je mod rada 'a'
		thisFile->size++;		
	cursor++;
	
	delete writeTo;
	return 1;
}

char KernelFile::readDataByte()
{
	PartControlBlock *pcb = thisFile->pcb;
	int whatByte = cursor % ClusterSize;
	int whatLocation = cursor / ClusterSize;
	Cluster *readFrom = new Cluster();

	if (whatLocation < I_LEV_DATA_POINTERS)		//trazeni bajt je u I nivou 
	{
		pcb->part->readCluster(readPointer( thisFile->index, whatLocation), readFrom->info);
	}
	else          //trazeni bajt je u II nivou
 	{
		Cluster *whatIndex = new Cluster();

		whatLocation = (whatLocation - I_LEV_DATA_POINTERS) / II_LEV_DATA_POINTERS + I_LEV_DATA_POINTERS;		
		pcb->part->readCluster(readPointer(thisFile->index, whatLocation), whatIndex->info);	//pokazivac na index II nivoa
		
		whatLocation = (cursor / ClusterSize - I_LEV_DATA_POINTERS) % II_LEV_DATA_POINTERS;		//cita klaster iz odgovarajuceg klastera II nivoa
		pcb->part->readCluster(readPointer(whatIndex, whatLocation), readFrom->info);
		
		delete whatIndex;
	}

	char res = readFrom->readFromByte(whatByte);
	delete readFrom;

	cursor++;

	return res;		
}

char KernelFile::seek(BytesCnt newPos)
{
	if (newPos > thisFile->size)
		return 0;
	cursor = newPos;
	return 1;
}

BytesCnt KernelFile::filePos()
{
	return cursor;
}

BytesCnt KernelFile::eof()
{
	if(myMode == 'r' || myMode == 'a')
	{	if(cursor == thisFile->size )	//za citanje
			return 2; //?
	}
	else
	{
		if (cursor == thisFile->size)/// - 1)	//za citanje
			return 2; //?
	}
	return 0;
}

char KernelFile::truncate()
{
	return 0;
}


