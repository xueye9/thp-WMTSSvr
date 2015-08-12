#include "StdAfx.h"
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <CLogThreadMgr.h>
#include "WMTSLevel.h"
#include "LayerLRUCache.h"
#include "Tile.h"
#include "WMTSLayer.h"
#include "Bundle.h"
#include "bdi/bdiapi.h"

#pragma warning(once:4996)

using namespace thp;

thp::TBundleRecord::TBundleRecord()
{
}

thp::TBundleRecord::~TBundleRecord()
{
}

std::tr1::shared_ptr<Bundle> thp::TBundleRecord::loadBundle(WMTSLevel* pLv)
{
	try
	{
		{
			QReadLocker locker(&rwLocker);
			if( !wpBundle.expired() )
			{
				std::tr1::shared_ptr<Bundle> sp = wpBundle.lock();
				return sp;
			}
		}

		QWriteLocker locker(&rwLocker);

		std::tr1::shared_ptr<Bundle> sp = pLv->getBundle( *this );

		wpBundle = sp;

		return sp;
	}
	catch(...)
	{
		//LOG(ERROR) << "分配资源错误";
		std::cerr << "分配资源错误";
		std::tr1::shared_ptr<Bundle> sp;
		return sp;
	}
}

thp::WMTSLayer::WMTSLayer()
{
	memset(m_pLvl, 0, THP_MAX_LEVEL);
	memset(m_szPath, 0, THP_MAX_PATH);

	m_pLyrLRU = new LayerLRUCache(THP_WMTS_DEFAULT_LRU_CACHE);

	m_pBundleRecords = NULL;
	m_eGTS = GTS_IO;

#ifdef _THP_TJ
	m_nCount = 0;
	m_nENum = 0;
	m_nMNum = 0;
#endif// _THP_TJ

	// 哈希表中资源超过1000万(含)时到整理时间会整理资源
	m_nMaintainLine = 100000000;
	m_nSpan = 200;

	_initLogWriter();
}

thp::WMTSLayer::~WMTSLayer()
{
	delete m_pLyrLRU;
	m_pLyrLRU = NULL;

	int nCount = HASH_COUNT(m_pBundleRecords);

	std::cout << "delete hash..." << std::endl;
	std::cout << "bundle count:" << nCount << std::endl;

	TBundleRecord* p = NULL, *pTemp = NULL;
	HASH_ITER(hh, m_pBundleRecords, p, pTemp) 
	{
		HASH_DEL(m_pBundleRecords, p);

		// 释放bundle资源
		delete p;
	}
	std::cout << "hash deleted" << std::endl;

	std::cout << "delete level..." << std::endl;
	for (int i = 0; i < THP_MAX_LEVEL; ++i)
	{
		WMTSLevel* pLv = m_pLvl[i];
		delete pLv;
		pLv = NULL;
	}
	std::cout << "level deleted" << std::endl;
}

bool WMTSLayer::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("WMTSLayer.csv");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("WMTSLayer", "WMTSLayer.csv", "", "DebugLog"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}



void thp::WMTSLayer::setCacheMbSize(unsigned int nMemByMB)
{
	// 换算成kb
	m_pLyrLRU->setCapacity( nMemByMB<<10 );
}

thp::WMTSLevel* thp::WMTSLayer::getLevel(int nLvl)
{
	if(nLvl<0 || nLvl >= THP_MAX_LEVEL)
		return 0;

	return m_pLvl[nLvl];
}

int thp::WMTSLayer::loadData(int nLvl)
{
	if( nLvl > THP_MAX_LEVEL)
	{
		// log
		return 0;
	}// 超出定义最大范围
	
	int nCount = _clacBundleCount(nLvl);
	int i = 0;
	TBundleIDex tId;
	
	return 0;
}

void WMTSLayer::_calcBundleNo(int nLvl, int nRow, int nCol, TBundleIDex& tNo)
{
	tNo.tID.unLv = (unsigned int)nLvl;

	if(nLvl < 8)
	{
		tNo.tID.unBundleIDinLv = 0;
		tNo.nBundleRow = 0;
		tNo.nBundleCol = 0;
		return ;
	}

	// 该等级下Bundle的行数
	int nBundleRowNum = 1 << (nLvl - 8);
	tNo.nBundleRow = nRow >> 7;
	
	// 瓦片所在Bundle所在的列索引
	tNo.nBundleCol = nCol >> 7;
	
	tNo.tID.unBundleIDinLv = (unsigned int)( (tNo.nBundleCol * nBundleRowNum) + tNo.nBundleRow );
}

int thp::WMTSLayer::_clacBundleCount(int nLvl)
{
	if(nLvl < 8)
		return 1;

	// 该等级下Bundle的行数
	return 1 << (2*nLvl - 15);
}

void thp::WMTSLayer::setMemStrategy(MemStrategy eGTS)
{
	m_eGTS = eGTS;
}

thp::WMTSLayer::MemStrategy thp::WMTSLayer::getMemStrategy() const
{
	return m_eGTS;
}

bool thp::WMTSLayer::setPath(const char* szPath)
{
	unsigned int unLen = (unsigned int)strlen(szPath);
	if(0 == unLen || unLen > THP_MAX_PATH)
		return false;

	memcpy(m_szPath, szPath, unLen);

	return true;
}

void thp::WMTSLayer::showStatus()
{
	QMutexLocker locker(&m_pRecordsMutex);

	TBundleRecord* p = NULL, *pTemp = NULL;

	std::cout << "-----------hash status---------" << std::endl;
	int nCount = HASH_COUNT(m_pBundleRecords);
	std::cout << "hash count :" << nCount << std::endl;
	std::cout << "--in memory--" << std::endl;
	int nCountInMem = 0;
	HASH_ITER(hh, m_pBundleRecords, p, pTemp) 
	{
		if( !p->wpBundle.expired() )
		{
			std::tr1::shared_ptr<Bundle> spBundle = p->wpBundle.lock();

			if( spBundle->isCached() )
			{
				std::cout << spBundle->getPath() << std::endl;
				++nCountInMem;
			}
		}
	}
	std::cout << "memory count :" << nCountInMem << std::endl;

	std::cout << "-----------hash END-------------------" << std::endl;

	std::cout << "-----------LRU status-------------------" << std::endl;

	m_pLyrLRU->showStatus();

	std::cout << "-----------LRU END-------------------" << std::endl;
}

void thp::WMTSLayer::maintain(bool bForce)
{
	QMutexLocker locker(&m_pRecordsMutex);

	int nRecordCount = HASH_COUNT(m_pBundleRecords);
	if(nRecordCount < m_nMaintainLine)
		return ;

	TBundleRecord* p = NULL, *pTemp = NULL;

	std::string sLayerName = getName();
	std::cout << "正在维护图层["<< sLayerName << "]..." << std::endl;

	bool bMaintainedData = false;

	if(bForce)
	{
		// 记录要删除的记录
		HASH_ITER(hh, m_pBundleRecords, p, pTemp) 
		{
			// 删除不再内存中的所有记录
			if( p->wpBundle.expired() )
			{
				HASH_DEL(m_pBundleRecords, p);

				// 释放bundle资源
				delete p;
				bMaintainedData = true;
			}// 已经不再内存中直接删除
			else
			{
				std::tr1::shared_ptr<Bundle> spBundle = p->wpBundle.lock();

				if( !spBundle->isCached() )
				{
					HASH_DEL(m_pBundleRecords, p);

					// 释放bundle资源
					delete p;
					bMaintainedData = true;
				}
			}// 清楚不在缓存记录中的 
		}
	}
	else
	{
		clock_t tBegin = clock();
		clock_t tMsBase = CLOCKS_PER_SEC / 1000;

		// 记录要删除的记录
		HASH_ITER(hh, m_pBundleRecords, p, pTemp) 
		{
			// 删除不再内存中的所有记录
			if( p->wpBundle.expired() )
			{
				HASH_DEL(m_pBundleRecords, p);

				// 释放bundle资源
				delete p;
				bMaintainedData = true;
			}// 已经不再内存中直接删除
			else
			{
				std::tr1::shared_ptr<Bundle> spBundle = p->wpBundle.lock();

				if( !spBundle->isCached() )
				{
					HASH_DEL(m_pBundleRecords, p);

					// 释放bundle资源
					delete p;
					bMaintainedData = true;
				}
			}// 清楚不在缓存记录中的 
			
			if ( (clock() - tBegin)/tMsBase > m_nSpan )
				break;
		}
	}

	if(bMaintainedData)
	{
		std::cout << "图层["<< sLayerName << "]维护完成" << std::endl;
	}
}

std::string thp::WMTSLayer::getName() const
{
	std::string sPath( m_szPath );

	if(sPath.empty())
		return "";

	size_t nSize = sPath.size();
	size_t nPos0 = sPath.rfind('\\');
	if(std::string::npos == nPos0)
		nPos0 = sPath.rfind('/');

	std::string sName = sPath.substr(nPos0+1, (nSize - nPos0));

	return sName;
}

