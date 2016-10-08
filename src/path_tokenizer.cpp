#include "path_tokenizer.h"

char PathTokenizer::getPartition(char *path)
{
	if (path == nullptr) return 0;
	char part = path[0];
	if (part<'A' || part>'Z') return 0;  //lose je ime rosledjeno
	return part;
}

char* PathTokenizer::getFileName(char *path)
{
	if (path == nullptr) return 0;
	if (path[0]<'A' || path[0]> 'Z' || path[1]!=':' || path[2] != '\\') 
		return nullptr;
	
	int cnt = 3;
//	while (path[cnt++] != '\0');

	while (path[cnt] != '.' && path[cnt] != '\0') cnt++;

//	cnt--;		//jer sada ukazuje ili na '.' ili na '/0'

	if (cnt > FNAMELEN + 3) return nullptr;		//sigurnosni kod: ime fajla ima vise od 8 karaktera

//	char *res = new char[cnt - 2];	//oduzme se pocetno stanje i poslednje inkrementiranje koje je visak
	
	char *res = new char[FNAMELEN + 1];			//ime +\0

	int i;
	for (i = 0; i < cnt - 3; i++)	//IZMENA: BILO JE -2	//ne kopira se \0
		res[i] = path[i + 3];
	while (i < FNAMELEN) res[i++] = ' ';
	res[i] = '\0';

	return res;
}

char * PathTokenizer::getFileExt(char *path)
{
	if (path == nullptr) return 0;
	if (path[0]<'A' || path[0]> 'Z' || path[1] != ':' || path[2] != '\\') return nullptr;

	int cnt = 3;
	while (path[cnt] != '.')
	{
		if (path[cnt]=='\0') return nullptr;		//putanja nema ekstenizju
		cnt++;
	}
	if (cnt > FNAMELEN + 3) return nullptr;		//sigurnosni kod: ime fajla ima vise od 8 karaktera

	int i = ++cnt;	//od ovog mesta se radi o ekstenziji

	while (path[cnt] != '\0') cnt++;

	if (cnt - i > FEXTLEN) return nullptr;		//ext ima vise od 3 karaktera

	char *res = new char[FEXTLEN + 1];	//oduzme se pocetno stanje i poslednje inkrementiranje koje je visak

	int j;
	for (j = 0; j < cnt - i ; j++)
		res[j] = path[j + i];

	while (j < FEXTLEN) res[j++] = ' ';
	res[FEXTLEN] = '\0';
	

	return res;
}

char * PathTokenizer::getNameAndExt(char *path)
{
	if (path == nullptr || path[0]<'A' || path[0]> 'Z' || path[1] != ':' || path[2] != '\\') return nullptr;

	int cnt = 3;
	char *res = new char[FNAMELEN + FEXTLEN + 2];

	int i = 0;
	while (i < FNAMELEN && path[cnt] != '.')
		res[i++] = path[cnt++];
	while (i < FNAMELEN) res[i++] = ' ';
	res[i = FNAMELEN] = '.';		//granica izmedju imena i eksteniyizije
	while (path[cnt] != '.') cnt++;
	i++, cnt++;

	while (i < FNAMELEN + FEXTLEN + 1 && path[cnt] != '\0')
		res[i++] = path[cnt++];
	while (i < FNAMELEN+FEXTLEN+1) res[i++] = ' ';
	res[FNAMELEN + FEXTLEN + 1] = '\0';

	return res;
}

char ** PathTokenizer::getPartNameExtNoDot(char *path)
{
	char **res = new char*[3];

	char *part = new char[2];
	part[1] = PathTokenizer::getPartition(path);
	part[2] = '\0';
	res[0] = part;
	res[1] = PathTokenizer::fillNameToFit(PathTokenizer::getFileName(path));
	res[2] = PathTokenizer::fillExtToFit(PathTokenizer::getFileExt(path));
	return res;
}

char ** PathTokenizer::getNameExtNoDot(char *path)
{
	char **res = new char*[2];

	char *part = new char[2];
	
	res[0] = PathTokenizer::fillNameToFit(PathTokenizer::getFileName(path));
	res[1] = PathTokenizer::fillExtToFit(PathTokenizer::getFileExt(path));
	return res;
}

char* PathTokenizer::fillNameToFit(char *name)
{
	if (name == nullptr) return nullptr;
	char *res = new char[FNAMELEN + 1];

	int i = 0;
	while (i < FNAMELEN && name[i] != '\0')
		res[i++] = name[i];
	while (i < FNAMELEN) res[i++] = ' ';
	res[FNAMELEN] = '\0';

	return res;
}

char* PathTokenizer::fillExtToFit(char *ext)
{
	if (ext == nullptr) return nullptr;
	char *res = new char[FEXTLEN + 1];

	int i = 0;
	while (i < FEXTLEN&& ext[i] != '\0')
		res[i++] = ext[i];
	while (i < FEXTLEN) res[i++] = ' ';
	res[FEXTLEN] = '\0';

	return res;
}

