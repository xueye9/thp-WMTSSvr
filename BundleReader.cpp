#include "StdAfx.h"
#include "BundleReader.h"
#include "./ParamDef.h"
#include <assert.h>

#pragma warning(once:4996)

using namespace thp;

// Bundle 最大的 行=列 瓦片数
const unsigned int nBUNLE_MAX_EDGE = 128;
const unsigned int nFILE_NAME_LENGTH = 18;

BundleReader::BundleReader()
{
	memset(m_szBundleFile, 0, BUNDLE_MAX_PATH);
	memset(m_szBundlxContents, 0, BUNDLX_CONTENT_SIZE);

	m_nIdxPos = 0;
	m_nBundleBeginRow = 0;
	m_nBundleBeginCol = 0;

#ifdef _DEBUG
	m_nValdTileCount = 0;
#endif// _DEBUG

	//_initLogWriter();
}

BundleReader::BundleReader(const char* szPath)
{
	memset(m_szBundleFile, 0, BUNDLE_MAX_PATH);
	memset(m_szLastErr, 0, BUNDLE_MAX_PATH);
	memset(m_szBundlxContents, 0, BUNDLX_CONTENT_SIZE);

	m_nIdxPos = 0;
	m_nBundleBeginRow = 0;
	m_nBundleBeginCol = 0;

#ifdef _DEBUG
	m_nValdTileCount = 0;
#endif// _DEBUG
}

BundleReader::~BundleReader()
{
	close();
}

bool thp::BundleReader::openNotLoadBundlx(const char* szFile)
{
	size_t nLen = strlen(szFile);
	if(nLen >= BUNDLE_MAX_PATH || nLen < 20)
	{
		sprintf(m_szLastErr, "file path is too long!the limits length is %d", BUNDLE_MAX_PATH);
		return false;
	}// bundle 文件名长度为 17 +'\0' 

	memcpy(m_szBundleFile, szFile, nLen);
	m_szBundleFile[nLen] = '\0';

	size_t nFrom = nLen - 16;	//
	char szNum[5];
	memcpy(szNum, szFile+nFrom, 4/* * sizeof(char) */ );
	szNum[4] = '\0';
	int nRow = 0;
	sscanf(szNum, "%x", &nRow);

	memcpy(szNum, szFile + nFrom + 5, 4 /* * sizeof(char) */);
	int nCol = 0;
	sscanf(szNum, "%x", &nCol);

	m_nBundleBeginRow = nRow;
	m_nBundleBeginCol = nCol;

	return true;
}

void thp::BundleReader::close()
{
	m_nIdxPos = 0;
	m_nBundleBeginRow = 0;
	m_nBundleBeginCol = 0;
	
	memset(m_szBundlxContents, 0, BUNDLX_CONTENT_SIZE);
#ifdef _DEBUG
	m_nValdTileCount = 0;
#endif// _DEBUG
}

int thp::BundleReader::_nextTile(unsigned char*& pOut, unsigned int& nSize)
{
	if(m_nIdxPos == BUNDLE_MAX_TILE_NUM)
		return -1;

	FILE* fpBundle = fopen(m_szBundleFile, "rb");
	if( !fpBundle )
	{
		//LOG(ERROR) << "打开文件出错";
		//m_pLogWriter->errorLog("打开文件出错");
		return false;
	}

	unsigned int* pOffset = NULL;
	int nRes = 0;
	unsigned int nTileSize =0;
	size_t nReadCount = 0;
	bool isEOF = true;

	while(m_nIdxPos < BUNDLE_MAX_TILE_NUM) 
	{
		// bundle 文件内容过大的话 这个位置可能要处理偏移值的生成
		pOffset = (unsigned int*)(m_szBundlxContents + m_nIdxPos * BUNDLX_NODE_SIZE);
		nRes = fseek((FILE*)fpBundle, *pOffset, SEEK_SET);
		if(0 != nRes)
		{	
			sprintf(m_szLastErr, "seek bundle offset failed");
			return -1;
		}

		nReadCount = fread(&nTileSize, 4, 1, fpBundle);
		if( 1 != nReadCount)
		{
			sprintf(m_szLastErr, "read bundle offset failed");
			return -1;
		}

		if(0 == nTileSize)
		{
			++m_nIdxPos;
			continue;// tile 不存在
		}

		pOut = new unsigned char[nTileSize];
		// 检测内存分配
		if(0 == pOut)
		{
			sprintf(m_szLastErr, "memory not enough");
			++m_nIdxPos;
			return -2;
		}

		nReadCount = fread(pOut, 1, nTileSize, fpBundle);
		if( nReadCount != nTileSize)
		{
			delete[] pOut;
			pOut = NULL;
			sprintf(m_szLastErr, "read bundle offset failed");
			++m_nIdxPos;
			return -1;
		}

		nSize = nTileSize;
		++m_nIdxPos;
		isEOF = false;

#ifdef _DEBUG
		++m_nValdTileCount;
#endif// _DEBUG

		break;
	}

	if(isEOF)
		return -1;

	return m_nIdxPos - 1;
}

bool thp::BundleReader::getTileFromFile(int nTileInBundleIndex, unsigned char*& pByteTile, unsigned int& nSize)
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

bool thp::BundleReader::copyBundlx(char*& pBlx, int nSize /*= BUNDLX_CONTENT_SIZE*/)
{
	pBlx = new char[nSize];
	if(!pBlx)
	{
		// log
		sprintf(m_szLastErr, "memory full");
		return false;
	}

	memcpy(pBlx, m_szBundlxContents, nSize);

	return true;
}

bool thp::BundleReader::loadBundlx(char* pBlx, int nSize /*= BUNDLX_CONTENT_SIZE*/)
{
	assert(pBlx);
	memcpy(m_szBundlxContents, pBlx, nSize);

	return true;
}

bool BundleReader::_calcWorldRowCol(int nIdxPos, int& nRow, int& nCol)
{
	int nInBundleCol = m_nIdxPos/BUNDLE_EDGE;
	int nInBundleRow = m_nIdxPos%BUNDLE_EDGE;

	nRow = m_nBundleBeginRow + nInBundleRow;
	nCol = m_nBundleBeginCol + nInBundleCol;

	return true;
}

BundleReader::FetchType BundleReader::nextTile(int& nRow, int& nCol, unsigned char*& pOut, int& nSize/*, std::ofstream* pOsInfo*/)
{
	FILE* fpBundle = fopen(m_szBundleFile, "rb");
	if( !fpBundle )
	{
		fclose(fpBundle);
	}
	if(m_nIdxPos == BUNDLE_MAX_TILE_NUM)
	{
		fclose(fpBundle);
		return FetchType_EOF;
	}

	unsigned int* pOffset = NULL;
	int nRes = 0;
	unsigned int nTileSize =0;
	size_t nReadCount = 0;
	bool isEOF = true;

	while(m_nIdxPos < BUNDLE_MAX_TILE_NUM) 
	{
		// bundle 文件内容过大的话 这个位置可能要处理偏移值的生成
		pOffset = (unsigned int*)(m_szBundlxContents + m_nIdxPos * BUNDLX_NODE_SIZE);
		nRes = fseek(fpBundle, *pOffset, SEEK_SET);

		if(0 != nRes)
		{	
			sprintf(m_szLastErr, "seek bundle offset failed");
			fclose(fpBundle);
			return FetchType_Fail;
		}

		nReadCount = fread(&nTileSize, 4, 1, fpBundle);

		if( 1 != nReadCount)
		{
			fclose(fpBundle);
			return FetchType_Fail;
		}

		if(0 == nTileSize)
		{
			++m_nIdxPos;
			continue;// tile 不存在
		}

		pOut = (unsigned char*)malloc(nTileSize);
		nReadCount = fread(pOut, 1, nTileSize, fpBundle);
		if( nReadCount != nTileSize)
		{
			free(pOut);
			pOut = NULL;

			sprintf(m_szLastErr, "read bundle offset failed");
			fclose(fpBundle);
			return FetchType_Fail;
		}
		
		nSize = nTileSize;
		_calcWorldRowCol(m_nIdxPos, nRow, nCol);
		++m_nIdxPos;
		isEOF = false;

#ifdef _DEBUG
		++m_nValdTileCount;
#endif// _DEBUG
		
		break;
	}

	if(isEOF)
	{
		fclose(fpBundle);
		return FetchType_EOF;
	}

	fclose(fpBundle);
	return FetchType_Success;
}
