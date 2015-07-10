#include "bdiapi.h"
#include <io.h>
#include <iostream>

using namespace std;

// -1 不是有效的LayerFolder
//IN:文件所在的路径,如：f:\example
//out:
int searchLayerFolder (string sLayerFolderPath, std::map<int,TLevelBundleExistStatus*>& pBlEstIdx) 
{
	struct _finddata64i32_t fileInfo;//保存文件信息的结构体
	long handle;//句柄
	int done;//查找nextfile是否成功
	string fileName = sLayerFolderPath+"\\*.*"; //要搜索的文件名

	//查找第一个文件，返回句柄
	handle=_findfirst64i32(fileName.c_str(), &fileInfo);
	if(handle==-1)
		return -1;

	do
	{
		//如果是文件夹".",或者".."，则进行判断下一个文件
		if( (strcmp(fileInfo.name,".")==0) || (strcmp(fileInfo.name,"..") == 0))
			continue;

		//如果是文件夹，则进入下一层文件夹搜索
		if((fileInfo.attrib&_A_SUBDIR)==_A_SUBDIR)
		{
			// cout<<"是文件夹"<<endl;
			// cin.get();
			string filePathSub=sLayerFolderPath+"\\"+fileInfo.name;

			if( 3 != strlen(fileInfo.name) )
			{
				std::cout << "文件夹["<< filePathSub <<"]"<< "跳过"<< std::endl;
				continue;
			}

			if( 'L' != fileInfo.name[0] )
			{
				std::cout << "文件夹["<< filePathSub <<"]"<< "跳过"<< std::endl;
				continue;
			}

			char szNum[4];
			memcpy(szNum, fileInfo.name+1, 4/* * sizeof(char) */ );
			szNum[3] = '\0';
			int nLv = 0;
			if( -1 == sscanf(szNum, "%d", &nLv) )
			{
				string fileNameTure = sLayerFolderPath + "\\" +fileInfo.name;
				std::cout << "文件["<< fileNameTure <<"] "<< "跳过"<< std::endl;
				continue;
			}

			TLevelBundleExistStatus* pNode = new TLevelBundleExistStatus;
			pNode->nSize = calcBunldeExistStatusOccupyByte(nLv);
			pNode->pbyteIndex = new unsigned char[pNode->nSize];
			memset(pNode->pbyteIndex, 0, pNode->nSize);

			//递归调用
			searchLevelFolder(filePathSub, nLv, pNode->pbyteIndex);

			// 记录索引
			pBlEstIdx.insert( std::make_pair(nLv, pNode) );
		}
		//把搜集到的信息连接到文件
		else
		{
			// 跳过文件
			string fileNameTure=sLayerFolderPath+"\\"+fileInfo.name;
			std::cout << "文件["<< fileNameTure <<"]"<< "跳过"<< std::endl;
		}

	}while( 0 == (done=_findnext64i32(handle,&fileInfo)) );

	_findclose(handle);
	if( pBlEstIdx.empty() )
		return -1;

	return 0;
}

// 获取表示bundle存在的表示需要占用的字节数
int calcBunldeExistStatusOccupyByte(int nLvl)
{
	if( nLvl < 10)
		return 1;

	//int nLeftMove = 2*nLvl - 15 - 3;
	int nLeftMove = 2*nLvl - 18;
	return (1 << nLeftMove);
}

// unsigned char* pBundleExistIdx 有2^(2*lv-23)个byte, lv< 12 有1个字节
// -1 不合法的level 文件夹
int searchLevelFolder (string sLayerLevelPath,
					   int nLvl,
					   unsigned char* pBundleExistIdx)
{
	struct _finddata64i32_t fileInfo;
	static int counter=0; 
	long handle;
	int done;

	//要搜索的文件名
	string fileName = sLayerLevelPath + "\\*"+ FILE_POSTFIX; 

	//查找第一个文件，返回句柄
	handle=_findfirst64i32(fileName.c_str(), &fileInfo);

	if(handle==-1)
		return -1;

	do
	{
		//如果是文件夹".",或者".."，则进行判断下一个文件
		if( (strcmp(fileInfo.name,".")==0) || (strcmp(fileInfo.name,"..") == 0))
			continue;

		if((fileInfo.attrib&_A_SUBDIR)==_A_SUBDIR)
		{
			// 跳过文件夹
			string fileNameTure = sLayerLevelPath + "\\" +fileInfo.name;
			std::cout << "文件夹["<< fileNameTure <<"] "<< "跳过"<< std::endl;
			continue;
		}// 

		fileInfo.name;
		//size_t nLen = strlen( fileInfo.name );
		//if( FILE_NAME_LEN != nLen)
		//{
		//	string fileNameTure = sLayerLevelPath + "\\" +fileInfo.name;
		//	std::cout << "文件["<< fileNameTure <<"] "<< "跳过"<< std::endl;
		//	continue;
		//}// 文件名长度不正确

		char* ptr = strchr(fileInfo.name, 'R');
		int pos0 = ptr - fileInfo.name;  
		ptr = strchr(fileInfo.name, 'C');
		int pos1 = ptr - fileInfo.name;
		
		ptr = strchr(fileInfo.name, '.');
		int pos2 = ptr - fileInfo.name;

		int nNum = pos1 - pos0 - 1;
		char* szNum = new char[nNum+1];
		memcpy(szNum, fileInfo.name + pos0 + 1, nNum * sizeof(char) );
		szNum[nNum] = '\0';
		unsigned int nRow = 0;
		if( -1 == sscanf(szNum, "%x", &nRow) )
		{
			string fileNameTure = sLayerLevelPath + "\\" +fileInfo.name;
			std::cout << "文件["<< fileNameTure <<"] "<< "跳过"<< std::endl;
			delete[] szNum;
			szNum = NULL;
			continue;
		}
		delete[] szNum;
		szNum = NULL;

		unsigned int nCol = 0;
		nNum = pos2 - pos1 - 1;
		szNum = new char[nNum+1];
		memcpy(szNum, fileInfo.name + pos1 + 1, nNum * sizeof(char) );
		szNum[nNum] = '\0';
		if( -1 == sscanf(szNum, "%x", &nCol) )
		{
			string fileNameTure = sLayerLevelPath + "\\" +fileInfo.name;
			std::cout << "文件["<< fileNameTure <<"] "<< "跳过"<< std::endl;
			delete[] szNum;
			szNum = NULL;
			continue;
		}
		delete[] szNum;
		szNum = NULL;

		// 计算bundle编号
		unsigned int nBundleIndex = calcBundleNo(nLvl, nRow, nCol);

		// 得到字节的偏移
		unsigned int nByteOffset = nBundleIndex >> 3;  // <==> nBundleIndex / 3
		unsigned char* pOf = pBundleExistIdx + nByteOffset;

		// 标记位置 0-7
		unsigned int nTagIdx = nBundleIndex - (nByteOffset << 3);
		tag(pOf, nTagIdx);

	}while( 0 == (done=_findnext64i32(handle,&fileInfo)) );

	_findclose(handle);
	return 0;
}

void tag(unsigned char* pOf, int nTagIdx)
{
	char cTag = 0x00;
	switch (nTagIdx)
	{
		// 10000000
	case 0:
		cTag = 0x80;
		break;
	case 1:
		cTag = 0x40;
		break;

		//00100000
	case 2:
		cTag = 0x20;
		break;

		//00010000
	case 3:
		cTag = 0x10;
		break;

		//00001000
	case 4:
		cTag = 0x08;
		break;

		//00000100
	case 5:
		cTag = 0x04;
		break;	

		//00000010
	case 6:
		cTag = 0x02;
		break;

		//00000001
	case 7:
		cTag = 0x01;
		break;

	default:
		break;
	}

	*pOf |= cTag;
}


int calcBundleNo(int nLvl, int nRow, int nCol)
{
	if(nLvl < 8)
		return 0;

	// 该等级下Bundle的行数
	int nBundleRowNum = 1 << (nLvl - 8);
	unsigned int nBundleRowIndex = nRow >> 7;

	// 瓦片所在Bundle所在的列索引
	unsigned int nBundleColIndex = nCol >> 7;

	int nBundleIndex = (nBundleColIndex * nBundleRowNum) + nBundleRowIndex;

	return nBundleIndex;
}

bool writeLayerBdlExistIdx(const std::map<int, TLevelBundleExistStatus*>& idxMap, const std::string& sPath)
{
	int i = 0;
	std::map<int, TLevelBundleExistStatus*>::const_iterator it = idxMap.begin();
	FILE* fpBdlIdx = fopen(sPath.c_str(), "wb");

	unsigned char szBom[16];
	memset(szBom, 0, 16);
	if( 16 != fwrite(szBom, 1, 16, fpBdlIdx) )
		return false;

	int nExist = 0x0000;
	for (i=0; i < MAX_LEVLE; ++i)
	{
		it = idxMap.find(i);
		int nSize = calcBunldeExistStatusOccupyByte(i);

		if( it == idxMap.end() )
		{
			nExist = 0x0000;
			fwrite((void*)&i, sizeof(int), 1, fpBdlIdx);

			nSize = 0;
			fwrite((void*)&nSize, sizeof(int), 1, fpBdlIdx);

			//unsigned char* pIdx = new unsigned char[nSize];
			//memset(pIdx, 0, nSize);

			//fwrite((void*)pIdx, sizeof(char), nSize, fpBdlIdx);
			continue;
		}

		nSize = it->second->nSize;
		fwrite((void*)&i, sizeof(int), 1, fpBdlIdx);
		fwrite((void*)&nSize, sizeof(int), 1, fpBdlIdx);
		fwrite(it->second->pbyteIndex, sizeof(unsigned char), nSize, fpBdlIdx);
	}

	fclose(fpBdlIdx);
	return true;
}

bool readLayerDbi(const std::string& sPath, std::map<int, TLevelBundleExistStatus*>& idxMap)
{
	int i = 0;
	FILE* fpBdlIdx = fopen(sPath.c_str(), "rb");
	if( !fpBdlIdx )
	{
		return false;
	}

	unsigned char szBom[16];
	memset(szBom, 0, 16);
	if( 16 != fread(szBom, 1, 16, fpBdlIdx) )
		return false;

	bool bSuccess = true;
	int nByteCount = 0;
	int nLv = 0;
	for (i=0; i < MAX_LEVLE; ++i)
	{
		if( 1 != fread(&nLv, sizeof(int), 1, fpBdlIdx) )
		{
			bSuccess = false;
			break;
		}

		if( 1 != fread(&nByteCount, sizeof(int), 1, fpBdlIdx) )
		{
			bSuccess = false;
			break;
		}

		if(0 == nByteCount)
		{
			idxMap.insert( std::make_pair(i, (TLevelBundleExistStatus*)NULL) );
			continue;
		}

		TLevelBundleExistStatus* pNode = new TLevelBundleExistStatus;
		//pNode->nSize = calcBunldeExistStatusOccupyByte(i);
		pNode->nSize = nByteCount;

		//fread((void*)&(pNode->nSize), sizeof(int), 1, fpBdlIdx);
		pNode->pbyteIndex = new unsigned char[nByteCount];
		fread( (void*)(pNode->pbyteIndex), sizeof(unsigned char), nByteCount, fpBdlIdx);
		idxMap.insert( std::make_pair(i, pNode) );
	}

	fclose(fpBdlIdx);
	return bSuccess;
}

bool isExist(unsigned char* pByteDbi, unsigned int unBundleIndex)
{
	// 得到字节的偏移
	unsigned int nByteOffset = unBundleIndex >> 3;  // <==> nBundleIndex / 3
	unsigned char* pOf = pByteDbi + nByteOffset;

	// 标记位置 0-7
	unsigned int nTagIdx = unBundleIndex - (nByteOffset << 3);

	return taged(pOf, nTagIdx);
}

bool taged(unsigned char* pOf, int nTagIdx)
{
	unsigned char cTag = 0x00;
	switch (nTagIdx)
	{
		// 10000000
	case 0:
		cTag = 0x80;
		break;
	case 1:
		cTag = 0x40;
		break;

		//00100000
	case 2:
		cTag = 0x20;
		break;

		//00010000
	case 3:
		cTag = 0x10;
		break;

		//00001000
	case 4:
		cTag = 0x08;
		break;

		//00000100
	case 5:
		cTag = 0x04;
		break;	

		//00000010
	case 6:
		cTag = 0x02;
		break;

		//00000001
	case 7:
		cTag = 0x01;
		break;

	default:
		return false;
	}

	bool bTaged = ( (cTag & (*pOf)) == cTag );
	return bTaged;
}