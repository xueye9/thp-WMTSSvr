#include "../StdAfx.h"
#include "WinBundle.h"
#include <memory>
#include <assert.h>
#include  <CLogThreadMgr.h>
#include <Windows.h>
#include "../Tile.h"
#include "WinBundleReader.h"
#include "../LayerLRUCache.h"

using namespace thp;

thp::WinBundle::WinBundle(const TBundleIDex& tNoEx) : Bundle(tNoEx)
{

}

thp::WinBundle::~WinBundle()
{

}

int thp::WinBundle::cache()
{
	thp::WinBundleReader reader;

	reader.open(m_szFilePath);

	if( !reader.copyBundlx(m_pBlx) )
	{
		m_pLogWriter->errorLog("复制索引错误");
		return 0;
	}

	int nSize = reader.readAll(m_pBle);
	if( nSize <= 0 )
	{
		m_pLogWriter->warnLog("复制数据错误");
		delete[] m_pBlx;
		m_pBlx = NULL;
		return 0;
	}

	m_bCached = true;

	return nSize;
}

bool thp::WinBundle::open(const char* szFile)
{
	size_t nLen = strlen(szFile);
	if(nLen >= BUNDLE_MAX_PATH || nLen < 20)
	{
		sprintf(m_szLastErr, "file path is too long!the limits length is %d", BUNDLE_MAX_PATH);
		return false;
	}// bundle 文件名长度为 17 +'\0' 

	memcpy(m_szFilePath, szFile, nLen);
	m_szFilePath[nLen] = '\0';

	thp::WinBundleReader reader;

	reader.open(m_szFilePath);
	m_unMaxByteSize = reader.getMaxByte();

	return true;
}

unsigned int WinBundle::getTile(int nRow, int nCol, QByteArray& arTile,int &nDetail)
{
	// 参数检查
	int nBundleRowIdx = nRow - m_nBeginRow;
	int nBundleColIdx = nCol - m_nBeginCol;

	if( (nBundleRowIdx < 0 || nBundleRowIdx >=128) || (nBundleColIdx<0 || nBundleColIdx >= 128) )
	{
		m_pLogWriter->warnLog("参数错误");
		return 0;
	}

	int nBundleId = (nBundleColIdx << 7) + nBundleRowIdx;

	if ( m_bCached )
	{
		return _getTileFromCache(nBundleId, arTile);
	}// bundle 全缓存模式 
	else
	{
		WinBundleReader reader;

		reader.open(m_szFilePath);

		unsigned char* pTile = NULL;
		unsigned int nSize = 0;

		if( reader.getTileFromFile(nBundleId, pTile, nSize) )
		{
			arTile.append((char*)pTile, nSize);
			delete pTile;
			pTile = NULL;
		}

		return nSize;
	}// 零散缓存模式--直接I/O
}
