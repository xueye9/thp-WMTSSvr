#include "StdAfx.h"

#include <memory>
#include <assert.h>

#include  <CLogThreadMgr.h>

#include <Windows.h>

#include "Bundle.h"
#include "Tile.h"
#include "BundleReader.h"
#include "LayerLRUCache.h"

using namespace thp;

thp::Bundle::Bundle(const TBundleIDex& tNoEx)
{
	//pthread_rwlock_init(&m_prwMutex, NULL);

	m_tID = tNoEx;
	m_nBeginRow = 128 * tNoEx.nBundleRow;
	m_nBeginCol = 128 * tNoEx.nBundleCol;
	m_bCached = false;
	m_nHeatDegree = 0;
	m_pBle = NULL;
	m_pBlx = NULL;
	m_unMaxByteSize = 0;

#ifdef _DEBUG
	m_lockReadTimes = 0;
	m_lockWriteTimes = 0;
#endif

	_initLogWriter();

	//pthread_rwlock_unlock(&m_prwMutex);
}

bool Bundle::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("Bundle.log");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("Bundle", "Bundle.log", "", "DebugLog"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}

thp::Bundle::~Bundle()
{
	//pthread_rwlock_destroy(&m_prwMutex);

	close();
}

void thp::Bundle::close()
{
	// 防止西沟函数抛出异常
	try
	{
		if(m_pBlx)
		{
			delete[] m_pBlx;
			m_pBlx = NULL;
		}

		if(m_pBle)
		{
			delete[] m_pBle;
			m_pBle = NULL;
		}
	}
	catch(...)
	{

	}
}

unsigned int thp::Bundle::getTile(int nRow, int nCol, QByteArray& arTile,int &nDetail)
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
		BundleReader reader;

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

unsigned int thp::Bundle::_getTileFromCache(int nTileIndexInBundle, QByteArray& arTile)
{
	unsigned int* pOffSet = (unsigned int*)(m_pBlx + nTileIndexInBundle * BUNDLX_NODE_SIZE);
	if( *pOffSet >= m_unMaxByteSize)
	{
		m_pLogWriter->warnLog("数据错误");
		//LOG(WARNING) << "数据错误";
		return 0;
	}

	int nSize = 0;
	//取四个字节读成unsigned int
	//unsigned int unSize = 0;
	//memcpy(&unSize, (m_byteBundle + *pOffSet), sizeof(unsigned int) );
	memcpy(&nSize, (m_pBle + *pOffSet), sizeof(unsigned int) );
	if(nSize >= m_unMaxByteSize)
	{
		m_pLogWriter->warnLog("数据错误");
		return 0;
	}

	if( 0 == nSize )
		return 0;

	arTile.append((m_pBle + *pOffSet + 4), nSize);

	return nSize;
}

unsigned int thp::Bundle::getMaxKB()
{
	return m_unMaxByteSize >> 10;	
}

int thp::Bundle::cache()
{
	thp::BundleReader reader;

	reader.open(m_szFilePath);

	if( !reader.copyBundlx(m_pBlx) )
	{
		m_pLogWriter->errorLog("复制索引错误");
		return 0;
	}

	int nSize = reader.ReadAll(m_pBle);
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

bool thp::Bundle::isCached()
{
	return m_bCached;
}

bool thp::Bundle::open(const char* szFile)
{
	size_t nLen = strlen(szFile);
	if(nLen >= BUNDLE_MAX_PATH || nLen < 20)
	{
		sprintf(m_szLastErr, "file path is too long!the limits length is %d", BUNDLE_MAX_PATH);
		return false;
	}// bundle 文件名长度为 17 +'\0' 

	memcpy(m_szFilePath, szFile, nLen);
	m_szFilePath[nLen] = '\0';

	thp::BundleReader reader;

	reader.open(m_szFilePath);
	m_unMaxByteSize = reader.getMaxByte();

	return true;
}

#ifdef _DEBUG

void thp::Bundle::check()
{
	int n = m_nBeginRow%128;
	int n2 = m_nBeginCol%128;
	if(0 != n || 0 != n2)
	{
		//LOG(ERROR) << "数据错误";
		std::cout << "数据错误";
	}

	char szLv[3];
	memset(szLv, 0, 3);

	memcpy(szLv, m_szFilePath+23, 2);

	int nl = atoi(szLv);
}

#endif

void thp::Bundle::heating()
{
	InterlockedIncrement( (LONG*)(&m_nHeatDegree) );
}

int thp::Bundle::getTemperature() const
{
	int nDegree = m_nHeatDegree;
	return nDegree;
}

void thp::Bundle::setTemperature(int nDegree)
{
	m_nHeatDegree = nDegree;
}

const thp::TBundleID& thp::Bundle::getID() const
{
	return m_tID.tID;
}

const char* thp::Bundle::getPath() const
{
	return m_szFilePath;
}

