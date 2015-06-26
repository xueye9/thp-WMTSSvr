#include "LayerLRUCache.h"
#include "Bundle.h"
#include "glog/logging.h"
#include "glog/raw_logging.h"
#include <set>

#ifdef _DEBUG
#include <ctime>
#include <Windows.h>
#endif

using namespace thp;

LayerLRUCache::LayerLRUCache(unsigned int unCapacity) 
{
	//pthread_rwlock_init(&m_prwMutex, NULL);

	pthread_mutex_init(&m_ptMutex, NULL);

	m_lockReadTimes = 0;
	m_lockWriteTimes = 0;

	bool bLocked = lockForWrite();

	m_unMaxKBCount = unCapacity;
	m_unUsedKBCount = 0;
	m_unLruHeadBundleKBCount = 0;

	unlock(false);
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
	{
		bool bLocked = lockForRead();
		if(!bLocked)
			return NULL;

		std::map<TBundleNo, std::list< std::pair<TBundleNo, Bundle*> >::iterator >::iterator it = m_mp.find(key);

		//没有命中
		if(it == m_mp.end())      
		{
			unlock(false);
			lockForWrite();
			return NULL;
		}

		// 最近一次使用
		std::list< std::pair<TBundleNo, Bundle*> >::iterator listIt = it->second;
		if( listIt == m_listCache.begin() )
		{
			Bundle* pBundleRes = m_listCache.begin()->second;
			pBundleRes->lockForRead();

			unlock(false);

			return pBundleRes;
		}

		unlock(true);
	}

	bool bLocked = lockForWrite();
	if( !bLocked )
		return NULL;

	std::map<TBundleNo, std::list< std::pair<TBundleNo, Bundle*> >::iterator >::iterator it = m_mp.find(key);
	std::list< std::pair<TBundleNo, Bundle*> >::iterator listIt = it->second;

	// 命中节点	
	std::pair<TBundleNo, Bundle*> pairNode;
	pairNode.first  = key;
	pairNode.second = listIt->second;

	// 先删除命中的节点
	m_listCache.erase( listIt );                

	// 将命中的节点放到链表头部
	m_listCache.push_front(pairNode);

	m_mp[key] = m_listCache.begin();
	
	Bundle* pBundleRes = m_listCache.begin()->second;

	unlock(false);

	return pBundleRes;
}

void thp::LayerLRUCache::set(const TBundleNo& key, thp::Bundle* pBundle)
{
	__try
	{
		set_pri(key, pBundle);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
}

void LayerLRUCache::set_pri(const TBundleNo& key, Bundle* pBundle)
{
	std::set<Bundle*> setRelease;

	bool bLocked = lockForWrite();
	if( !bLocked )
		return ;

	std::map<TBundleNo, std::list< std::pair<TBundleNo, Bundle*> >::iterator >::iterator it = m_mp.find(key);

	if(it == m_mp.end()) 
	{
		m_unUsedKBCount += pBundle->getMemKB();

		// 栈满了
		while(m_unUsedKBCount > m_unMaxKBCount)
		{
			if( 0 == m_listCache.size() )
				break;

			const std::pair<TBundleNo, Bundle*>& pairNode = m_listCache.back();

			// 内部保证没有出现负值的情况
			m_mp.erase( pairNode.first );

			m_unUsedKBCount -= pairNode.second->getMemKB();

			Bundle* pBl = m_listCache.back().second;

			setRelease.insert(pBl);

			// 出栈
			m_listCache.pop_back();
		}

		std::pair<TBundleNo, Bundle*> pairNewNode(key, pBundle);
		m_listCache.push_front( std::make_pair(key, pBundle) );
		m_mp[key] = m_listCache.begin();
	}// 没有命中
	else
	{
		std::list< std::pair<TBundleNo, Bundle*> >::iterator listIt = m_mp[key];

		// 维护信息, 释放命中节点的资源 
		m_unUsedKBCount -= listIt->second->getMemKB();

		setRelease.insert(listIt->second);

		// 删除命中的节点
		m_listCache.erase(listIt);

		// 维护信息
		m_unUsedKBCount += pBundle->getMemKB();

		// 添加新节点
		std::pair<TBundleNo, Bundle*> pairNode;
		pairNode.first = key;
		pairNode.second = pBundle;

		//将命中的节点放到链表头部
		m_listCache.push_front(pairNode);   
		m_mp[key] = m_listCache.begin();
	}//命中

	unlock(false);

	for (std::set<Bundle*>::iterator setit = setRelease.begin(); setit != setRelease.end(); ++setit)
	{
		Bundle* pDl = *setit;
		pDl->lockForWrite();
		delete pDl;
		pDl = NULL;
		// ? 析构了的对象的锁还要不要释放
	}
}

void thp::LayerLRUCache::push(const TBundleNo& key, thp::Bundle* pBundle)
{
	std::set<Bundle*> setRelease;

	std::map<TBundleNo, std::list< std::pair<TBundleNo, Bundle*> >::iterator >::iterator it = m_mp.find(key);

	m_unUsedKBCount += pBundle->getMemKB();

	// 栈满了
	while(m_unUsedKBCount > m_unMaxKBCount)
	{
		if( 0 == m_listCache.size() )
			break;

		const std::pair<TBundleNo, Bundle*>& pairNode = m_listCache.back();

		// 内部保证没有出现负值的情况
		m_mp.erase( pairNode.first );

		m_unUsedKBCount -= pairNode.second->getMemKB();

		Bundle* pBl = m_listCache.back().second;

		setRelease.insert(pBl);

		// 出栈
		m_listCache.pop_back();
	}

	std::pair<TBundleNo, Bundle*> pairNewNode(key, pBundle);
	m_listCache.push_front( std::make_pair(key, pBundle) );
	m_mp[key] = m_listCache.begin();

	unlock(false);

	for (std::set<Bundle*>::iterator setit = setRelease.begin(); setit != setRelease.end(); ++setit)
	{
		Bundle* pDl = *setit;
		pDl->lockForWrite();
		delete pDl;
		pDl = NULL;
		// ? 析构了的对象的锁还要不要释放
	}
}

#include <Windows.h>

bool thp::LayerLRUCache::lockForRead()
{
	RAW_LOG(INFO, "locking for read, wt %d, rd %d", m_lockWriteTimes, m_lockReadTimes);
	if( 0 == pthread_mutex_lock(&m_ptMutex) )
	//if( 0 == pthread_rwlock_tryrdlock(&m_prwMutex) )
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

	if( 0 == pthread_mutex_lock(&m_ptMutex) )
	//if(0 == pthread_rwlock_wrlock(&m_prwMutex) )
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
	if( 0 == pthread_mutex_unlock(&m_ptMutex) )
	//if( 0 == pthread_rwlock_unlock(&m_prwMutex) )
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

thp::LayerLRUCache::~LayerLRUCache()
{
	//pthread_rwlock_destroy(&m_prwMutex);
	pthread_mutex_destroy(&m_ptMutex);
}

void LayerLRUCache::updateUsedCapacity()
{
	std::set<Bundle*> setRelease;

	Bundle* pBdl = m_listCache.begin()->second;
	m_unUsedKBCount = pBdl->getMemKB() - m_unLruHeadBundleKBCount;

	// 栈满了
	while(m_unUsedKBCount > m_unMaxKBCount)
	{
		const std::pair<TBundleNo, Bundle*>& pairNode = m_listCache.back();

		// 内部保证没有出现负值的情况
		m_mp.erase( pairNode.first );
		
		// 记录日志
		//DLOG(INFO) << "LRU释放内存 bundleid:" << pairNode.first.unLv << "-" << pairNode.first.unBunldeID;

		// 释放资源
		//delete m_listCache.back().second;
		setRelease.insert( m_listCache.back().second );

		// 忘记
		m_listCache.pop_back();

		m_unUsedKBCount -= pairNode.second->getMemKB();
	}

	for (std::set<Bundle*>::iterator setit = setRelease.begin(); setit != setRelease.end(); ++setit)
	{
		Bundle* pDl = *setit;
		pDl->lockForWrite();
		delete pDl;
		pDl = NULL;
		// ? 析构了的对象的锁还要不要释放
	}
}

void LayerLRUCache::saveUsedCapacityStatus()
{
	lockForWrite();
	Bundle* pBdl = m_listCache.begin()->second;
	m_unLruHeadBundleKBCount = pBdl->getMemKB();
	unlock(false);
}


