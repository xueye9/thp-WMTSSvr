#include "../StdAfx.h"
#include "HdfsBundle.h"
#include "../BundleParamDef.h"
#include "HdfsBundleReader.h"
#include <QString>
#include  <CLogThreadMgr.h>

thp::HdfsBundle::HdfsBundle(const TBundleIDex& tNoEx) : Bundle(tNoEx)
{

}

thp::HdfsBundle::~HdfsBundle()
{

}

bool thp::HdfsBundle::open(const char* szFile)
{
	size_t nLen = strlen(szFile);
	if(nLen >= BUNDLE_MAX_PATH || nLen < 20)
	{
		sprintf(m_szLastErr, "file path is too long!the limits length is %d", BUNDLE_MAX_PATH);
		return false;
	}// bundle 文件名长度为 17 +'\0' 

	memcpy(m_szFilePath, szFile, nLen);
	m_szFilePath[nLen] = '\0';

	thp::HdfsBundleReader reader;

	reader.open(m_szFilePath);
	m_unMaxByteSize = reader.getMaxByte();

	return true;
}

int thp::HdfsBundle::cache()
{
	int nSize = 0;
	thp::HdfsBundleReader reader;

	reader.open(m_szFilePath);

	if( !reader.copyBundlx(m_pBlx) )
	{
		m_pLogWriter->errorLog("复制索引错误");
		return 0;
	}

	nSize = reader.readAll(m_pBle);
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

unsigned int thp::HdfsBundle::getTile(int nRow, int nCol, QByteArray& arTile,int &nDetail)
{
	// 参数检查
	int nBundleRowIdx = nRow - m_nBeginRow;
	int nBundleColIdx = nCol - m_nBeginCol;

	if( (nBundleRowIdx < 0 || nBundleRowIdx >=128) || (nBundleColIdx<0 || nBundleColIdx >= 128) )
	{
		QString qsLog = QString("hdfs,参数错误,%0,%1,%2").arg( GB(m_szFilePath) ).arg(nRow).arg(nCol);
		m_pLogWriter->errorLog(qsLog);
		return 0;
	}

	int nBundleId = (nBundleColIdx << 7) + nBundleRowIdx;

	if ( m_bCached )
	{
		return _getTileFromCache(nBundleId, arTile);
	}// bundle 全缓存模式 
	else
	{
		HdfsBundleReader reader;

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
