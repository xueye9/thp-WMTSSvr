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
	//pthread_rwlock_init(&rwLocker, NULL);
}

thp::TBundleRecord::~TBundleRecord()
{
	//pthread_rwlock_destroy(&rwLocker);
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

	_initLogWriter();
	//pthread_mutex_init(&m_pRecordsMutex, NULL);
}

bool WMTSLayer::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("WMTSLayer.log");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("WMTSLayer", "WMTSLayer.log", "", "DebugLog"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}

thp::WMTSLayer::~WMTSLayer()
{
	delete m_pLyrLRU;

	TBundleRecord* p = NULL, *pTemp = NULL;
	HASH_ITER(hh, m_pBundleRecords, p, pTemp) 
	{
		HASH_DEL(m_pBundleRecords, p);

		// 释放bundle资源
		delete p;
	}
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

unsigned int thp::WMTSLayer::getTile(int nLvl, int nRow, int nCol, QByteArray& arTile, int& nDetail)
{
	if( nLvl > THP_MAX_LEVEL)
	{
		// log
		return 0;
	}// 超出定义最大范围

	if( nRow < 0 || nRow > (1 << (nLvl-1)) )
	{
		// log
		return 0;
	}// 超出范围定义的瓦片行数

	if( nCol < 0 || nCol > (1 << nLvl) )
	{
		// log
		return 0;
	}// 超出瓦片列数
	
	// 没有对应的等级
	WMTSLevel* pLv = m_pLvl[nLvl];
	if( NULL == pLv )
		return 0;

	TBundleIDex tbnNo;
	_calcBundleNo(nLvl, nRow, nCol, tbnNo);

	// 测试缓存
	std::tr1::shared_ptr<Bundle> spBundle; 

	// 查找资源
	TBundleRecord* pRecord = NULL;
	HASH_FIND(hh, m_pBundleRecords, &(tbnNo.tID), sizeof(TBundleID), pRecord);

	// 获取操作成功但是没有对应的数据则尝试创建
	if( NULL == pRecord )
	{
		// bundle 不存在
		if( !pLv->exist(tbnNo) )
			return 0;

		{
			//pthread_mutex_lock(&m_pRecordsMutex);
			QMutexLocker locker(&m_pRecordsMutex);

			HASH_FIND(hh, m_pBundleRecords, &(tbnNo.tID), sizeof(TBundleID), pRecord);
			if(NULL == pRecord)
			{
				pRecord = new TBundleRecord;

				pRecord->tID = tbnNo.tID;
				pRecord->nBundleRow = tbnNo.nBundleRow;
				pRecord->nBundleCol = tbnNo.nBundleCol;

				HASH_ADD(hh, m_pBundleRecords, tID, sizeof(TBundleID), pRecord);
			}

			//pthread_mutex_unlock(&m_pRecordsMutex);
		}
	}// 加载bundle

	spBundle = pRecord->loadBundle(pLv);

	// 资源无法创建返回，没有对应的切片文件,服务没有对应的bundle资源
	if( NULL == spBundle.get() )
	{
		QString sErrInfo = QString("none tile resource, %0,%1,%2,%3");
		m_pLogWriter->errorLog( GB("IO资源错误") );
		return 0;
	}

	// 保证有 8 个以上的bundle可以缓存
	if( (GTS_MEM == m_eGTS) && (spBundle->getMaxKB() < (m_pLyrLRU->getCapacity() >> 3)) )
	{
		if( !spBundle->isCached() )
		{
			spBundle->cache();
		}

		m_pLyrLRU->add(spBundle);
	}

	if(spBundle->getID().unLv < 0 || spBundle->getID().unLv > 21)
	{
		m_pLogWriter->errorLog( GB("IO资源错误") );
		return 0;
	}

	return spBundle->getTile(nRow, nCol, arTile, nDetail);
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

void thp::WMTSLayer::setGetTileStrategy(GetTileStrategy eGTS)
{
	m_eGTS = eGTS;
}

thp::WMTSLayer::GetTileStrategy thp::WMTSLayer::getGetTileStrategy() const
{
	return m_eGTS;
}

