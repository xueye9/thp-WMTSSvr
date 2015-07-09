#ifndef THP_WMTS_LRUCACHE_H__
#define THP_WMTS_LRUCACHE_H__
#include <Windows.h>
#include <list>

#include "pthread/pthread.h"
#include "ParamDef.h"
#include "Bundle.h"
#include "uthash/uthash.h"
#include "dclist.h"

class CLogWriter;

namespace thp
{
class Bundle;
class BundleFactory;
class CircularList;

struct TBundleHashTableNode
{
	struct thp::TBundleID key;
	thp::Bundle* val;

	UT_hash_handle hh;
};

class LayerLRUCache
{
public:
	enum GetType 
	{
		G_SUCCESS = 0,
		G_FAIL,
	};

	typedef std::tr1::shared_ptr<thp::Bundle> sPtr;

	// 2^32 kb = 2^22MB = 2^12 Gb = 2^2 Tb 
	// 单位 KB 
	LayerLRUCache(unsigned int nCapacity);
	~LayerLRUCache();

	// 单位 KB
	unsigned int getCapacity();
	void setCapacity(unsigned int nCapacity); 

	// 会有一次解锁
	// 调用者对lru负责加解锁，对象直接放到头部，可能有出栈操作
	int add( std::tr1::shared_ptr<Bundle> sp );

	bool lockForRead();
	bool lockForWrite();
	void unlock(bool bRead);

	bool isFull();

	bool _initLogWriter();
private:

	// 用于异常捕捉
	thp::Bundle* get_pri(const TBundleIDex& key);

private:
	
	// 必须使用互斥锁 使用读写锁会有问题
	// 如下五个个成员变量的读写锁
	pthread_rwlock_t m_prwMutex;

	//pthread_mutex_t m_ptMutex;

	// 单位 KB
	unsigned int m_unMaxKBCount ;
	unsigned int m_unUsedKBCount;
	unsigned int m_unLruHeadBundleKBCount;

	// 资源hash表
	//TBundleHashTableNode* m_pBundles;

	// 资源热度循环链表 
	// CircularList* m_pList;

	//std::map< TBundleID, sPtr > m_Map;

	dclist<sPtr> m_listHotColdResources;

	CLogWriter *					m_pLogWriter;
#ifdef _DEBUG
	LONG m_lockReadTimes;
	LONG m_lockWriteTimes;
#endif
};

}
#endif // THP_WMTS_LRU_H__
