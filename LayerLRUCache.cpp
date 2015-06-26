#include "LayerLRUCache.h"
#include "Bundle.h"
#include "glog/logging.h"
#include "glog/raw_logging.h"
#include <set>
#include "CircularList.h"
#include "BundleFactory.h"

#ifdef _DEBUG
#include <ctime>
#include <Windows.h>
#endif

using namespace thp;

LayerLRUCache::LayerLRUCache(unsigned int unCapacity) 
{
	pthread_rwlock_init(&m_prwMutex, NULL);

	//pthread_mutex_init(&m_ptMutex, NULL);

	m_lockReadTimes = 0;
	m_lockWriteTimes = 0;

	bool bLocked = lockForWrite();

	m_unMaxKBCount = unCapacity;
	m_unUsedKBCount = 0;
	m_unLruHeadBundleKBCount = 0;

	m_pList = new CircularList;

	unlock(false);
}

thp::LayerLRUCache::~LayerLRUCache()
{
	pthread_rwlock_destroy(&m_prwMutex);
	delete m_pList;
	//pthread_mutex_destroy(&m_ptMutex);
}

void LayerLRUCache::setCapacity(unsigned int unCapacity)
{
	bool bLocked = lockForWrite();
	if(bLocked)
		return ;

	m_unMaxKBCount = unCapacity;

	unlock(false);
}

unsigned int LayerLRUCache::getCapacity()
{
	bool bLocked = lockForWrite();
	if(bLocked)
		return 0;

	int nRes = m_unMaxKBCount;

	unlock(false);

	return nRes;
}

Bundle* LayerLRUCache::get(const TBundleNo& key)
{
	__try
	{
		return get_pri(key);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return NULL;
	}
}

Bundle* LayerLRUCache::get_pri(const TBundleNo& key)
{
	lockForRead();

	// 现在的时间复杂度 log2n
	TbnoBdlMapIt it = m_mapKeyValue.find(key);
	if( it == m_mapKeyValue.end() )
	{
		unlock(true);
		return NULL;
	}

	Bundle* pRes = it->second;

	pRes->lockForRead();
	pRes->heating();

	unlock(true);

	return pRes;
}

thp::Bundle* thp::LayerLRUCache::addAndGet(const TBundleNo& key, BundleFactory* pFactory)
{
	std::set<Bundle*> setRelease;
	lockForWrite();
	
	// 其他线程可能已经写过
	TbnoBdlMapIt it = m_mapKeyValue.find(key);
	if( it != m_mapKeyValue.end() )
	{
		Bundle* pBundle = it->second;
		
		pBundle->heating();

		pBundle->lockForRead();

		unlock(true);

		return pBundle;
	}

	Bundle* pNewBundle = pFactory->createBundle(key);

	// 这种实现方式最后入栈的bundle过大可能会造成使用的内存使用大于lru的设定值
	if( m_unUsedKBCount < m_unMaxKBCount )
	{
		pNewBundle->cache();

		m_unUsedKBCount += pNewBundle->getMemKB();
		m_mapKeyValue[key] = pNewBundle;
		m_pList->push_front(pNewBundle);
	}// 栈未满 将资源直接插入热段头
	else
	{
		// 资源出栈
		Bundle* pBundle = (Bundle*)m_pList->tailData();
		while( m_unUsedKBCount > m_unMaxKBCount )
		{
			while ( pBundle->getHeatDegree() > 2 )
			{
				pBundle->lockForWrite();
				pBundle->setHeatDegree(0);
				pBundle->unlock(false);

				m_pList->moveHeadForward();

				pBundle = (Bundle*)m_pList->tailData();
			}// 使用两个以上认为是热的

			pBundle = (Bundle*)m_pList->pop_back();
			m_unUsedKBCount -= pBundle->getMemKB();

			setRelease.insert( pBundle );
		}// 新资源未加入时保证内存未用满
	
		// 缓存资源 
		pNewBundle->cache();

		// 添加资源到冷端
		m_unUsedKBCount += pNewBundle->getMemKB();
		m_mapKeyValue[key] = pNewBundle;

		// map的资源总数 和循环链表中的资源总数相等的
		int nCount = m_mapKeyValue.size();

		int nColdBegin = 0;
		
		// 最多有10个冷资源
		if( nCount > 20 )
			nColdBegin = 10;
		else
			nColdBegin = nCount >> 1; // <=> nCount/2

		CircularList::TclNode* pColdHead = m_pList->tail();
		for (int i=0;i<nColdBegin;++i)
			pColdHead = pColdHead->pPrev;

		m_pList->insert(pColdHead, pNewBundle);
	}// 栈满

	pNewBundle->heating();
	pNewBundle->lockForRead();

	unlock(false);

	for (std::set<Bundle*>::iterator setit = setRelease.begin(); setit != setRelease.end(); ++setit)
	{
		Bundle* pDl = *setit;
		pDl->lockForWrite();
		delete pDl;
		pDl = NULL;
		// ? 析构了的对象的锁还要不要释放
	}

	return pNewBundle;
}

#include <Windows.h>

bool thp::LayerLRUCache::lockForRead()
{
	RAW_LOG(INFO, "locking for read, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
	//if( 0 == pthread_mutex_lock(&m_ptMutex) )
	if( 0 == pthread_rwlock_tryrdlock(&m_prwMutex) )
	{
		InterlockedIncrement(&m_lockReadTimes);
		RAW_LOG(INFO, "lock read success, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		return true;
	}
	else
	{
		RAW_LOG(INFO, "lock read fail");
		return false;
	}
}

bool thp::LayerLRUCache::lockForWrite()
{
	RAW_LOG(INFO, "locking for write,wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);

	//if( 0 == pthread_mutex_lock(&m_ptMutex) )
	if(0 == pthread_rwlock_wrlock(&m_prwMutex) )
	{
		InterlockedIncrement(&m_lockWriteTimes);
		RAW_LOG(INFO, "lock write success,wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		return true;
	}
	else
	{
		RAW_LOG(INFO, "lock write fail");
		return false;
	}
}

void thp::LayerLRUCache::unlock(bool bRead)
{
	RAW_LOG(INFO, "unlocking, rd %d , wt %d", m_lockReadTimes, m_lockWriteTimes);
	//if( 0 == pthread_mutex_unlock(&m_ptMutex) )
	if( 0 == pthread_rwlock_unlock(&m_prwMutex) )
	{
		if( bRead )
		{
			InterlockedDecrement(&m_lockReadTimes);
			RAW_LOG(INFO, "unlock read success, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		}
		else
		{
			InterlockedDecrement(&m_lockWriteTimes);
			RAW_LOG(INFO, "unlock write success, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
		}
	}
	else
	{
		if( bRead )
			RAW_LOG(INFO, "unlock read fail");
		else
			RAW_LOG(INFO, "unlock write fail");
	}
}



