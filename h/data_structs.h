#ifndef _DATA_STRUCTS_H_
#define _DATA_STRUCTS_H_


#define BIT_VECT_POSS 0
//#define ROOT_IND_POSS 1
//pozicije bitVektora i rootIndexa

#define BYTE_SIZE 8  
//jedan bajt je 8 bita

#define F_CLUST_POOL_SIZE 10
#define ROOT_ENTRY_OFFSET 20
//pomeraj je 20 bajtova

#define ROOT_FILE_RESERVED_SIZE 1
#define ROOT_FILE_INDEX_CLUSTER_SIZE 4
#define ROOT_FILE_SIZE_SIZE 4
//svaki ulaz je 20B, a bajt je 8b
#define RESERVED_BIT 0


#define INDEX_ENRY 4
//velicina ulaza je 4B
#define I_LEV_DATA_POINTERS 256
#define II_LEV_POINTERS 256
//512/2
#define II_LEV_DATA_POINTERS 512


#define NUM_OF_PARTS 26


#define START_SIZE 5
#define INCREMENT 5


typedef unsigned long BytesCnt;
typedef unsigned long EntryNum;

const unsigned long ENTRYCNT = 64;
const unsigned int FNAMELEN = 8;
const unsigned int FEXTLEN = 3;

struct Entry {
	char name[FNAMELEN];
	char ext[FEXTLEN];
	char reserved;
	unsigned long indexCluster;
	unsigned long size;
};

typedef Entry Directory[ENTRYCNT];

typedef unsigned long ClusterNo;
const unsigned long ClusterSize = 2048;





struct Cluster
{
	char info[2048];
	int writeInByte(unsigned int no, char ch)
	{
		if (no > ClusterSize - 1) return 0;
		info[no] = ch;
	}

	char readFromByte(unsigned int no)
	{
		//if (no > ClusterSize-1) return 0;
		return info[no];
	}

	void writeFirstKByte(char *buffer)
	{
		for (int i = 0; i < 1024; i++)
		{
			info[i] = 0x00;
			info[i] |= buffer[i];
		}
	}
	void writeSecondKByte(char *buffer)
	{
		for (int i = 1024; i < 2048; i++)
		{
			info[i] = 0x00;
			info[i] |= buffer[i - 1024];
		}
	}



	char* readFirstKByte()
	{
		char *res = new char[1024 + 1];
		for (int i = 0; i < 1024; i++)
			res[i] = info[i];
		return res;
	}

	char* readSecondKByte()
	{
		char *res = new char[1024 + 1];
		for (int i = 1024; i < 2048; i++)
			res[i - 1024] = info[i];
		return res;
	}


	Cluster()
	{
		for (int i = 0; i < 2048; i++)
			info[i] = 0;
	}

};


struct FileIndexI
{
	ClusterNo index;
	int nextData;
	int DataLeft;

};
struct FileIndexII
{

};





#endif // !_DATA_STRUCTS_H_
