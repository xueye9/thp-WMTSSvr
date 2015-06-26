#ifndef THP_WMTS_LRUCACHE_H__
#define THP_WMTS_LRUCACHE_H__
#include <map>
#include "pthread/pthread.h"
#include <Windows.h>
#include <set>
#include <list>
#include "ParamDef.h"

namespace thp
{
class Bundle;
class BundleFactory;
class CircularList;

class LayerLRUCache
{
public:

	// 2^32 kb = 2^22MB = 2^12 Gb = 2^2 Tb 
	// 单位 KB 
	LayerLRUCache(unsigned int nCapacity);
	~LayerLRUCache();

	// 单位 KB
	unsigned int getCapacity();
	void setCapacity(unsigned int nCapacity); 

	// 返回使用对象
	// 返回NULL时对自身加写锁
	thp::Bundle* get(const TBundleNo& key);

	// 会有一次解锁
	// 调用者对lru负责加解锁，对象直接放到头部，可能有出栈操作
	thp::Bundle* addAndGet(const TBundleNo& key, BundleFactory* pFactory);

	bool lockForRead();
	bool lockForWrite();
	void unlock(bool bRead);

private:

	// 用于异常捕捉
	thp::Bundle* get_pri(const TBundleNo& key);

private:
	
	// 必须使用互斥锁 使用读写锁会有问题
	// 如下五个个成员变量的读写锁
	pthread_rwlock_t m_prwMutex;

	//pthread_mutex_t m_ptMutex;

	// 单位 KB
	unsigned int m_unMaxKBCount ;
	unsigned int m_unUsedKBCount;
	unsigned int m_unLruHeadBundleKBCount;
	// std::list< std::pair<TBundleNo, thp::Bundle*> > m_listCache;
	// 使用hash_map替换效率可以更高 
	typedef std::map<TBundleNo, thp::Bundle*> TbnoBdlMap;
	typedef TbnoBdlMap::iterator	TbnoBdlMapIt;

	// bundle 资源表
	TbnoBdlMap m_mapKeyValue;

	// 资源热度循环链表 
	CircularList* m_pList;

	LONG m_lockReadTimes;
	LONG m_lockWriteTimes;
};

}
#endif // THP_WMTS_LRU_H__
