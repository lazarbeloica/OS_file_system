#include "root_vec_ctrl.h"

void RootVectCtrl::update()
{
	part->writeCluster(ROOT_IND_POSS, rootIndex.info);
}

RootVectCtrl::RootVectCtrl(PartitionCache *p, int ROOT_IND_POSS)
{
	part = p;
	this->ROOT_IND_POSS = ROOT_IND_POSS;
	rootIndex = Cluster();

	part->readCluster(ROOT_IND_POSS, rootIndex.info);
	
}

RootVectCtrl::~RootVectCtrl()
{
	update();
//	delete part;		//ovo ce da prise pcb
	part = nullptr;
}

void RootVectCtrl::writeDirEntry(EntryNum &no, Entry *e)
{
	for (int i = 0; i < FNAMELEN; i++)
		rootIndex.info[no*ROOT_ENTRY_OFFSET + i] = e->name[i];

	for (int i = 0; i < FEXTLEN; i++)
		rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + i] = e->ext[i];

	rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE] = e->reserved;

	for (int i = 0; i < ROOT_FILE_INDEX_CLUSTER_SIZE; i++)
		rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + i] = e->indexCluster >> (i*BYTE_SIZE);


	for (int i = 0; i < ROOT_FILE_SIZE_SIZE; i++)
		rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE + i] = e->size >> (i*BYTE_SIZE);

}

Entry * RootVectCtrl::readDirEntry(EntryNum no)		
{
	Entry *res = new Entry();

	for (int i = 0; i < FNAMELEN; i++)
		res->name[i] = rootIndex.info[no*ROOT_ENTRY_OFFSET + i];

	for (int i = 0; i < FEXTLEN; i++)
		res->ext[i] = rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + i];

	res->reserved = rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN];

	unsigned char tmp = 0;
	res->indexCluster = 0;
	for (int i = 0; i < ROOT_FILE_INDEX_CLUSTER_SIZE; i++)
	{
		res->indexCluster <<= BYTE_SIZE;
		tmp = rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE - i - 1];
		res->indexCluster |= tmp;
	}

	res->size = 0;
	for (int i = 0; i < ROOT_FILE_SIZE_SIZE; i++)
	{
		res->size <<= BYTE_SIZE;
		tmp = rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE + ROOT_FILE_SIZE_SIZE - i - 1];
		res->size |= tmp;
	}

	return res;
}

void RootVectCtrl::clearDirEntry(EntryNum no)
{
	for (int i = no*ROOT_ENTRY_OFFSET; i < FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE + ROOT_FILE_SIZE_SIZE; rootIndex.info[i++] = 0);
}

void RootVectCtrl::clearDirEntryName(EntryNum no)
{
	for (int i = no*ROOT_ENTRY_OFFSET; i < FNAMELEN; rootIndex.info[i++] = 0);
}

void RootVectCtrl::clearDirEntryExt(EntryNum no)
{
	for (int i = no*ROOT_ENTRY_OFFSET + FNAMELEN; i < FNAMELEN + FEXTLEN; rootIndex.info[i++] = 0);
}

void RootVectCtrl::copyEnry(Entry &from, Entry &to)
{
	for (int i = 0; i < FNAMELEN; to.name[i++] = from.name[i]);

	for (int i = 0; i < FNAMELEN; to.ext[i++] = from.ext[i]);

	to.reserved = from.reserved;
	to.indexCluster = from.indexCluster;
	to.size = from.size;
}

EntryNum RootVectCtrl::checkForName(char *name, char *ext)
{
	char *checkName = new char[FNAMELEN + 1];
	char *checkExt = new char[FEXTLEN + 1];

	for (int no = 0; no < ENTRYCNT; no++)
	{
		for (int i = 0; i < FNAMELEN; i++)		//cita se ime
			checkName[i] = rootIndex.info[no*ROOT_ENTRY_OFFSET + i];
		checkName[FNAMELEN] = '\0';

		//	string x = tmp;
		if (strcmp(checkName, name) == 0)
		{
			for (int i = 0; i < FEXTLEN; i++)		//cita se ext
				checkExt[i] = rootIndex.info[no*ROOT_ENTRY_OFFSET + FNAMELEN + i];
			checkExt[FEXTLEN] = '\0';

			if (strcmp(checkExt, ext) == 0) return no;
		}
	}

	return -1;
}

void RootVectCtrl::updateFileSize(int entryNum, unsigned long size)
{
	for (int i = 0; i < ROOT_FILE_SIZE_SIZE; i++)
		rootIndex.info[entryNum*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + ROOT_FILE_INDEX_CLUSTER_SIZE + i] = size >> (i*BYTE_SIZE);

}

void RootVectCtrl::setIndex(int entryNum, EntryNum index)
{
	for (int i = 0; i < ROOT_FILE_INDEX_CLUSTER_SIZE; i++)
		rootIndex.info[entryNum*ROOT_ENTRY_OFFSET + FNAMELEN + FEXTLEN + ROOT_FILE_RESERVED_SIZE + i] = index >> (i*BYTE_SIZE);
}

void RootVectCtrl::formatRI()
{
	for (int i = 0; i < ClusterSize; rootIndex.info[i++] = 0x00);
	part->writeCluster(ROOT_IND_POSS, rootIndex.info);
}

EntryNum RootVectCtrl::getFreeEntry()
{
	for (int i = 0; i < ENTRYCNT; i++)
		if (rootIndex.info[ROOT_ENTRY_OFFSET*i] == 0) return i;

	return -1;  //kod da je puno
}


EntryNum RootVectCtrl::doesExist(char *path)
{
	char *name = PathTokenizer::getFileName(path);
	char *ext = PathTokenizer::getFileExt(path);

	if (name == nullptr || ext == nullptr) return 0;

	return checkForName(name, ext);
}

char RootVectCtrl::readRootDir(EntryNum no, Directory & d)
{
	int entrysRead = 0;
	Entry *tmp;
	for (int i = no; i < ENTRYCNT; i++)
	{
		tmp = readDirEntry(i);
		if (tmp->indexCluster == 0 && tmp->size == 0 && tmp->name[0] == 0 && tmp->ext[0] == 0) 
			continue;
		copyEnry(*tmp, d[entrysRead]);
		entrysRead++;
	}
	return entrysRead;
}
