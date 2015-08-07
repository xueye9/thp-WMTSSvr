#include "WinBundleReader.h"
#include "../ParamDef.h"
#include <stdio.h>
#include <assert.h>

using namespace thp;


thp::WinBundleReader::WinBundleReader() : BundleReader()
{

}

thp::WinBundleReader::~WinBundleReader()
{

}

bool WinBundleReader::getTileFromFile(int nTileInBundleIndex, unsigned char*& pByteTile, unsigned int& nSize)
{
	unsigned int* pOffSet = (unsigned int*)(m_szBundlxContents + nTileInBundleIndex * BUNDLX_NODE_SIZE);

	FILE* fpBundle = fopen(m_szBundleFile, "rb");
	if( !fpBundle )
	{
		//LOG(ERROR) << "打开文件出错";
		//m_pLogWriter->errorLog("打开文件出错");
		return false;
	}

	int nRes = fseek((FILE*)fpBundle, *pOffSet, SEEK_SET);
	if(0 != nRes)
	{	
		//m_pLogWriter->warnLog("IO文件出错");
		//LOG(ERROR) << "IO文件出错" << m_szBundleFile;
		sprintf(m_szLastErr, "seek bundle offset failed");
		fclose(fpBundle);
		return false;
	}

	unsigned int unReadCount = fread(&nSize, 4, 1, fpBundle);
	if( 1 != unReadCount)
	{
		//m_pLogWriter->warnLog("IO文件出错");
		//LOG(ERROR) << "IO文件出错" << m_szBundleFile;
		sprintf(m_szLastErr, "read bundle offset failed");
		fclose(fpBundle);
		return false;
	}

	if(0 == nSize)
	{
		fclose(fpBundle);
		return false;
	}

	if( nSize > getMaxByte() )
	{
		fclose(fpBundle);
		return false;
	}

	nRes = fseek((FILE*)fpBundle, *pOffSet + 4, SEEK_SET);
	if(0 != nRes)
	{	
		sprintf(m_szLastErr, "seek bundle offset failed");
		return false;
	}

	// 读取内容
	pByteTile = new unsigned char[nSize];
	if(!pByteTile)
	{
		sprintf(m_szLastErr, "memory Error");
		delete[] pByteTile;
		fclose(fpBundle);
		return false;
	}

	unReadCount = fread(pByteTile, 1, nSize, fpBundle);
	if( nSize != unReadCount)
	{
		if ( feof(fpBundle) )
			printf("Error reading test.bin: unexpected end of file\n");
		else if (ferror(fpBundle)) 
		{
			perror("Error reading test.bin");
		}

		fclose(fpBundle);
		sprintf(m_szLastErr, "I/O Error");

		delete[] pByteTile;
		return false;
	}

	fclose(fpBundle);
	return true;
}

bool thp::WinBundleReader::open(const char* szFile)
{
	size_t nLen = strlen(szFile);
	if(nLen >= BUNDLE_MAX_PATH || nLen < 20)
	{
		sprintf(m_szLastErr, "file path is too long!the limits length is %d", BUNDLE_MAX_PATH);
		return false;
	}// bundle 文件名长度为 17 +'\0' 

	memcpy(m_szBundleFile, szFile, nLen);
	m_szBundleFile[nLen] = '\0';

	char szBundlxFile[BUNDLE_MAX_PATH];
	memcpy(szBundlxFile, m_szBundleFile, THP_MAX_PATH);
	szBundlxFile[nLen-1] = 'x';
	if( !_loadBundlx(szBundlxFile) )
		return false;

	// 得到Bundle文件名
	std::string sBundle(szFile);
	size_t nPos0 = sBundle.rfind('\\');
	if(nPos0 == std::string::npos)
		nPos0 = sBundle.rfind('/');

	size_t nPos1 = sBundle.rfind('.');
	if(nPos1 == std::string::npos)
		return false;

	std::string sBundleName = sBundle.substr(nPos0, (nPos1 - nPos0));

	// 计算起始行列号
	nPos0 = sBundleName.find('R');
	nPos1 = sBundleName.find('C');

	std::string sBeginRow = sBundleName.substr(nPos0+1, nPos1 - nPos0 -1);
	std::string sBeginCol = sBundleName.substr(nPos1+1, sBundleName.size() - nPos1);

	sscanf(sBeginRow.c_str(), "%x", &m_nBundleBeginRow);
	sscanf(sBeginCol.c_str(), "%x", &m_nBundleBeginCol);

	return true;
}

bool thp::WinBundleReader::_loadBundlx(const char* szFile)
{
	FILE* fpBundlx = fopen(szFile, "rb");
	if ( !fpBundlx )
	{
		sprintf(m_szLastErr, "load bundlx file[%s] error", szFile);
		// log error 打开索引文件失败
		return false;// 
	}

	int nTemp = 0;
	// 文件前16个字节无用
	nTemp = fseek(fpBundlx, 16L,SEEK_SET);
	if(0 != nTemp)
	{
		sprintf(m_szLastErr, "load bundlx file[%s] error", szFile);
		// log error 打开索引文件失败
		fclose(fpBundlx);
		return false;// 
	}

	size_t nCount = fread(m_szBundlxContents, 1, BUNDLX_CONTENT_SIZE, fpBundlx);
	if(BUNDLX_CONTENT_SIZE != nCount)
	{
		sprintf(m_szLastErr, "load bundlx file[%s] error", szFile);
		// log error 打开索引文件失败
		fclose(fpBundlx);
		return false;// 
	}

	fclose(fpBundlx);
	return true;
}

unsigned int thp::WinBundleReader::getMaxByte()
{
	FILE* fpBundle = fopen(m_szBundleFile, "rb");
	if( !fpBundle )
	{
		//m_pLogWriter->warnLog("IO 文件错误");
		//LOG(ERROR) << "IO 文件错误";
		return 0;
	}

	// 两次 I/O 读取整个bundle文件到内存 

	// 得到bundle文件的大小
	unsigned int nRes = fseek (fpBundle, 0, SEEK_END);   
	if ( 0 != nRes )
	{	
		sprintf(m_szLastErr, "seek bundle offset failed");
		fclose(fpBundle);
		return -1;
	}
	unsigned int nSize = ftell (fpBundle); 

	fclose(fpBundle);
	return nSize;
}

int thp::WinBundleReader::readAll(char*& pBundle)
{
	// 计算bundle文件大小  可以研究一下bundle文件内容
	FILE* fpBundle = fopen(m_szBundleFile, "rb");
	if( !fpBundle )
	{
		//m_pLogWriter->warnLog("IO 文件出错");
		//LOG(ERROR) << "IO 文件出错" << m_szBundleFile;
		return -1;
	}

	// 两次 I/O 读取整个bundle文件到内存 

	// 得到bundle文件的大小
	int nRes = fseek (fpBundle, 0, SEEK_END);   
	if ( 0 != nRes )
	{	
		sprintf(m_szLastErr, "seek bundle offset failed");
		fclose(fpBundle);
		return -1;
	}
	int nSize = ftell (fpBundle); 

	nRes = fseek(fpBundle, 0, SEEK_SET);
	if ( 0 != nRes )
	{	
		sprintf(m_szLastErr, "seek bundle offset failed");
		fclose(fpBundle);
		return false;
	}

	pBundle = new char[nSize]; 
	if(!pBundle)
	{
		// log
		fclose(fpBundle);
		sprintf(m_szLastErr, "memory full");
		return false;
	}

	unsigned int unReadCount = fread(pBundle, sizeof(char), nSize, fpBundle);

	// 检查I/O
	if ( unReadCount != nSize )
	{
		// log
		sprintf(m_szLastErr, "seek bundle offset failed");
		fclose(fpBundle);
		return 0;
	}

	fclose(fpBundle);
	return nSize;
}


