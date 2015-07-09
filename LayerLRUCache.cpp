#include "StdAfx.h"
#include "LayerLRUCache.h"
#include "Bundle.h"
//#include "glog/logging.h"
//#include "glog/raw_logging.h"
#include <set>
#include "dclist.h"
#include  <CLogThreadMgr.h>
#ifdef _DEBUG
#include <ctime>
#include <Windows.h>
#endif

using namespace thp;
LayerLRUCache::LayerLRUCache(unsigned int unCapacity) 
{
	pthread_rwlock_init(&m_prwMutex, NULL);

#ifdef _DEBUG
	m_lockReadTimes = 0;
	m_lockWriteTimes = 0;
#endif

	bool bLocked = lockForWrite();

	m_unMaxKBCount = unCapacity;
	m_unUsedKBCount = 0;
	m_unLruHeadBundleKBCount = 0;

	//m_pBundles = NULL;
   _initLogWriter();
	unlock(false);
}

bool LayerLRUCache::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("LayerLRUCache.log");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("LayerLRUCache", "LayerLRUCache.log", "", "DebugLog"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}

thp::LayerLRUCache::~LayerLRUCache()
{
	// 释放hash表资源
	lockForWrite();

	m_pLogWriter->debugLog("LRU 正在释放资源...");
	//LOG(INFO) << "LRU 正在释放资源...";

	unlock(false);

	pthread_rwlock_destroy(&m_prwMutex);
	//delete m_pList;

	m_pLogWriter->debugLog("LRU 完成释放资源...");
	//LOG(INFO) << "LRU 完成释放资源";
}


void LayerLRUCache::setCapacity(unsigned int unCapacity)
{
	bool bLocked = lockForWrite();
	if( !bLocked )
		return ;

	m_unMaxKBCount = unCapacity;

	unlock(false);
}

unsigned int LayerLRUCache::getCapacity()
{
	return m_unMaxKBCount;
}

bool thp::LayerLRUCache::lockForRead()
{
#ifndef _DEBUG
	if( 0 == pthread_rwlock_tryrdlock(&m_prwMutex) )
		return true;
	else
		return false;
#else
	m_pLogWriter->debugLog("locking for read");
	//RAW_DLOG(INFO, "locking for read, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
	if( 0 == pthread_rwlock_tryrdlock(&m_prwMutex) )
	{
		InterlockedIncrement(&m_lockReadTimes);
		m_pLogWriter->debugLog("locking for read");
		//RAW_DLOG(INFO, "lock read success, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		return true;
	}
	else
	{
		m_pLogWriter->infoLog("lock read fail");
		return false;
	}
#endif
}

bool thp::LayerLRUCache::lockForWrite()
{
#ifndef _DEBUG
	if(0 == pthread_rwlock_wrlock(&m_prwMutex) )
		return true;
	else
		return false;
#else
	//LOG(INFO) << "locking for write,wt"<< m_lockWriteTimes << ", rd" << m_lockReadTimes;

	//RAW_DLOG(INFO, "locking for write,wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
	//m_pLogWriter->debugLog("lock read fail");

	//if( 0 == pthread_mutex_lock(&m_ptMutex) )
	if(0 == pthread_rwlock_wrlock(&m_prwMutex) )
	{
		InterlockedIncrement(&m_lockWriteTimes);
		//RAW_DLOG(INFO, "lock write success,wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		return true;
	}
	else
	{
		//RAW_DLOG(INFO, "lock write fail");
		return false;
	}
#endif
}

void thp::LayerLRUCache::unlock(bool bRead)
{
#ifndef _DEBUG
	if(1 ==	pthread_rwlock_unlock(&m_prwMutex) )
	{
		m_pLogWriter->errorLog("资源解锁失败,请重启服务");
		//RAW_LOG(ERROR, "资源解锁失败,请重启服务");
	}
#else
	//RAW_DLOG(INFO, "unlocking, rd %d , wt %d", m_lockReadTimes, m_lockWriteTimes);
	//if( 0 == pthread_mutex_unlock(&m_ptMutex) )
	if( 0 == pthread_rwlock_unlock(&m_prwMutex) )
	{
		if( bRead )
		{
			InterlockedDecrement(&m_lockReadTimes);
			if(m_lockReadTimes < 0)
				m_pLogWriter->errorLog("解读锁错误");
				//LOG(ERROR) << "解读锁错误";

			//RAW_DLOG(INFO, "unlock read success, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		}
		else
		{
			InterlockedDecrement(&m_lockWriteTimes);
			if(m_lockReadTimes < 0)
				m_pLogWriter->errorLog("解读锁错误");
				//LOG(ERROR) << "解读锁错误";

			//RAW_DLOG(INFO, "unlock write success, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		}
	}
	else
	{
		if( bRead )
		{
			m_pLogWriter->errorLog("unlock read fail");
			//RAW_DLOG(INFO, "unlock read fail");
		}
		else
		{
			m_pLogWriter->errorLog("unlock write fail");
			//DLOG(INFO, "unlock write fail");
			//RAW_DLOG(INFO, "unlock write fail");
		}
	}
#endif
}

bool thp::LayerLRUCache::isFull()
{
	return m_unUsedKBCount >= m_unMaxKBCount;
}

int thp::LayerLRUCache::add(std::tr1::shared_ptr<Bundle> sp)
{
	lockForWrite();

	if(m_unUsedKBCount < m_unMaxKBCount)
	{
		m_listHotColdResources.push_front(sp);
		
		unlock(false);
		return 0;
	}
	
	while(m_unUsedKBCount > m_unMaxKBCount)
	{
		sPtr sp = m_listHotColdResources.back();
		while( sp->getTemperature() > 2)
		{
			sp->setTemperature(0);
			m_listHotColdResources.move_head_forward();
			continue;
		}

		if( m_unUsedKBCount < sp->getMaxKB() )
		{
			m_pLogWriter->errorLog("LRU错误");
			//LOG(ERROR) << "LRU错误";
			break;
		}

		m_unUsedKBCount -= sp->getMaxKB();
	
		m_listHotColdResources.pop_back();
	}

	m_unUsedKBCount += sp->getMaxKB();

	unlock(false);
	return 0;
}



