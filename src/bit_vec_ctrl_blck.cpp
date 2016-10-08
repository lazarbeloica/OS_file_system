#include "bit_vec_ctrl_blck.h"

void BitVectCtrl::update()
{
	//part->writeCluster(0, bitVector.info);
	for (int i = 0; i < numOfBitVectors; i++)
	{
		bitVector[i] = new Cluster();
		part->writeCluster(i, bitVector[i]->info);
	}
}

BitVectCtrl::BitVectCtrl(PartitionCache *p, int numOfBitVectors)
{
	this->numOfBitVectors = numOfBitVectors;
	part = p;
	bitVector = new Cluster*[numOfBitVectors];

	for (int i = 0; i < numOfBitVectors; i++)
	{
		bitVector[i] = new Cluster();
		part->readCluster(i, bitVector[i]->info);
	}
	//part->readCluster(0, bitVector.info);

	for (FCPointer = 0; FCPointer < F_CLUST_POOL_SIZE; freeCluster[FCPointer++] = 0);	//inicijalizuje freeCluster posle ovoga on ce da pokayuje iza poslednjeg el
	noFreeClusters = false;
	clustersInPartition = part->getNumOfClusters();		//radi kontrole prekoracenja
	updateFCPool();
}

BitVectCtrl::~BitVectCtrl()
{
	update();
	for (int i = 0; i < numOfBitVectors; i++)
	{
		delete bitVector[i];
	}
	delete bitVector;
	delete part;		//BACA IZUZETAK PRI POKUSAJU BRISANJA PARTICIJE
	part = nullptr;
}

void BitVectCtrl::updateFCPool()
{
#ifdef TEST_BIT
	cout << "U updateFCPoolu" << endl;
#endif //  TEST_BIT

	if (FCPointer == 0)
		return;		//ovo znaci da je pool pun	

	int bitVect = 0;
	int Byte = 0; //broj bajta u bit vektoru koji se proverava
	signed char bitMask = 0b10111111;  //1101 1111 pocetna maska, jer u prvom bajtu nikad ne proveravamo prva dva bita 
	bitMask >>= numOfBitVectors;								 //jer oni referenciraju bitVect i rootIndex
	int bitInBlock = numOfBitVectors + 1; //jer je DF 1101 1111

	noFreeClusters = false;

	for (int i = FCPointer - 1; i >= 0 && !noFreeClusters; i--)
	{
		while (Byte < ClusterSize * numOfBitVectors )	// proveraav se samo nulti klaster(bit vektor)
		{
			if (Byte*BYTE_SIZE + bitInBlock >= clustersInPartition)
			{
				noFreeClusters = true;
				break;
			}

			bitVect = Byte / ClusterSize;
			Byte = Byte % ClusterSize;
			if (bitVector[bitVect]->info[Byte] != 0xFF) //nisu svi birti tog bajta 1
			{
				while ((bitVector[bitVect]->info[Byte] | bitMask) == -1 && bitMask != ~0 && bitInBlock < BYTE_SIZE)	//trazimo prvi slobodan bit u tom bajtu  !!!!!PROMENJEN KOD i maskaa == 0xff
				{
					if (bitMask == 0b01111111)
						bitMask = 0b10111111;
					else
						bitMask >>= 1;			//maska se siftuje desno za jedno polje, posto je signed na pocetakse dodaje 1
					bitInBlock++;
				}
				if (bitMask != ~0 && bitInBlock < BYTE_SIZE)	//nismo stigli do kraja, vec smo asli polje
				{
					ClusterNo tmp = Byte*BYTE_SIZE + bitInBlock;	//racuna se br bloka
					if (Byte*BYTE_SIZE + bitInBlock >= clustersInPartition)		//provera da nije doslo do prekoracenja PROMENJEN KOD STAJALO JE tmp
					{
						noFreeClusters = true;
						break;
					}
					
					if (bitMask == 0b01111111)
						bitMask = 0b10111111;
					else
						bitMask >>= 1;		//maska se pomera za polje ako bude jos iteracija
					
					
					bitInBlock++;
					if (bitMask == 0xFF) //dosli smo do kraja polja pa je potrebno rese
					{
						Byte++;
						bitMask = 0x7F; //0111 1111 spremni za proveru sledeceg bajta
						bitInBlock = 0;
					}
					freeCluster[--FCPointer] = tmp;
					break;		//sledaca iteracija
				}
			}
			Byte++;
			bitMask = 0x7F; //0111 1111 spremni za proveru sledeceg bajta
			bitInBlock = 0;	//resetuje se na 0
			if (Byte == ClusterSize * numOfBitVectors - 1)
			{
				noFreeClusters = true;
				break;
			}
		}
	}
}

const ClusterNo BitVectCtrl::getEmptyCluster()		//ne vraca gresku!!!		
{
#ifdef TEST_BIT
	cout << "U getEmptyClusteru" << endl;
#endif //  TEST_BIT

	if (FCPointer == F_CLUST_POOL_SIZE) 
		updateFCPool();	

	if (FCPointer == F_CLUST_POOL_SIZE)
		return 0;		//nema vise mesta

	ClusterNo res = freeCluster[FCPointer];
	freeCluster[FCPointer++] = 0;
	//	if (FCPointr == F_CLUST_POOL_SIZE) updateFCPool();
	return res;
}

void BitVectCtrl::releseCluster(const ClusterNo &no)
{
	int byteNum = no / BYTE_SIZE;	//koji je B u bit Vectoru
	int bitInByte = no % BYTE_SIZE;	//koji je taj trazeni bit
	unsigned char mask = 0x01; //0000 0001

	mask <<= BYTE_SIZE - bitInByte - 1; //pomeraj desno za odg br bita

	int bitVect = byteNum / ClusterSize;
	byteNum = byteNum % ClusterSize;

	if ((bitVector[bitVect]->info[byteNum] & mask) == 0)
		return;		//nije ni alociran

	bitVector[bitVect]->info[byteNum] = bitVector[bitVect]->info[byteNum] & (~mask);

	if (FCPointer > 0)	//ima mesta na steku
		freeCluster[--FCPointer] = no;	//ako ima mesta u pool-u odmah ga smestamo tu
	else if (noFreeClusters)
		noFreeClusters = false;		//sad se signalizira da ima slobodnih
									//clastera VAN freeCluster steka
}

char BitVectCtrl::allocateCluster(const ClusterNo &no)
{
	if (no == 0 || no == 1)
		return 0; //greska ili je sve puno
	int byteNum = no / BYTE_SIZE;	//koji je B u bit Vectoru
	int bitInByte = no % BYTE_SIZE;	//koji je taj trazeni bit
	unsigned char mask = 0x01; //0000 0001
	mask <<= BYTE_SIZE - bitInByte - 1; //pomeraj desno za odg br bita

	int bitVect = byteNum / ClusterSize;
	byteNum = byteNum % ClusterSize;

	if ((bitVector[bitVect]->info[byteNum] & mask) != 0)
		return allocateCluster(); //vec je zauzet taj blok
	bitVector[bitVect]->info[byteNum] = bitVector[bitVect]->info[byteNum] | mask; //stavljamo jedinicu na to mesto
														  //	FCPointr = F_CLUST_POOL_SIZE - 1; //pokazuje na poslednji el niza
	return 1;
}

ClusterNo BitVectCtrl::allocateCluster()
{
	const ClusterNo no = getEmptyCluster();
	if (no == 0)
		return 0;

	if (no == 0 || no == 1)
		return 0; //greska ili je sve puno
	int byteNum = no / BYTE_SIZE;	//koji je B u bit Vectoru
	int bitInByte = no % BYTE_SIZE;	//koji je taj trazeni bit
	unsigned char mask = 0x01; //0000 0001
	mask <<= BYTE_SIZE - bitInByte - 1; //pomeraj desno za odg br bita
	
	int bitVect = byteNum / ClusterSize;
	byteNum = byteNum % ClusterSize;

	if ((bitVector[bitVect]->info[byteNum] & mask) != 0)
		return allocateCluster(); //vec je zauzet taj blok
	bitVector[bitVect]->info[byteNum] = bitVector[bitVect]->info[byteNum] | mask; //stavljamo jedinicu na to mesto
																				  //	FCPointr = F_CLUST_POOL_SIZE - 1; //pokazuje na poslednji el niza
	return no;
}

bool BitVectCtrl::isAlocated(const ClusterNo no)
{
	if (no == 0 || no == 1)
		return true;		//ovi su uvek zauzeti
	int byteNum = no / BYTE_SIZE;		//koji je B u bit Vectoru
	int bitInByte = no % BYTE_SIZE;		//koji je taj trazeni bit
	unsigned char mask = 0x01;			//0000 0001
	mask <<= BYTE_SIZE - bitInByte - 1; //pomeraj desno za odg br bita

	int bitVect = byteNum / ClusterSize;
	byteNum = byteNum % ClusterSize;

	if ((bitVector[bitVect]->info[byteNum] & mask) != 0)
		return true;					//zauzet blok
	return false;
}

void BitVectCtrl::formatBV()
{
	for (int i = 0; i < numOfBitVectors; i++)
	{
		int j = 0;
		if (i == 0){
			signed char tmp = 0b10000000;
			tmp >>= numOfBitVectors;
			bitVector[i]->info[0] = tmp;//1100 0000
			j = 1;
		}
		for (; j < ClusterSize; bitVector[i]->info[j++] = 0x0);
		part->writeCluster(i, bitVector[i]->info);
	}

	//bitVector.info[0] = 0xC0;//1100 0000
	//for (int i = 1; i < ClusterSize; bitVector.info[i++] = 0x0);


	//refresh poola niti////////////////PREPRAVKA///////////////////////

	for (FCPointer = 0; FCPointer < F_CLUST_POOL_SIZE; freeCluster[FCPointer++] = 0);	//inicijalizuje freeCluster posle ovoga on ce da pokayuje iza poslednjeg el
	noFreeClusters = false;
	updateFCPool();
//	part->writeCluster(BIT_VECT_POSS, bitVector.info);
}


