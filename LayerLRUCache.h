#ifndef THP_WMTS_LRUCACHE_H__
#define THP_WMTS_LRUCACHE_H__
#include <map>
#include <list>
#include <QReadWriteLock>
#include "pthread/pthread.h"
#include <Windows.h>

namespace thp
{
class Bundle;

// Bundle 编号
struct TBundleNo
{
	// 考虑内存占用可以用1char
	unsigned int unLv;

	// 在等级内的编号
	// nLv < 8 nBundleID = 0; nLv> 8  nBundleID = [0, 2^(2*nLv-15))
	// 序号对应
	// nLv >= 8  有 2^(nLv-8) row, 有2^(nLv-7) 列
	// 如下表示编号对应的bundle的位置
	// 0   1*(2^(nLv-8))   ...
	// 1   .
	// .   .
	// .   .
	// .   .
	unsigned int unBunldeID;

	bool operator==(const TBundleNo& tNoNew) const;
	bool operator<(const TBundleNo& tNoNew) const;

	TBundleNo();
};

class LayerLRUCache
{
public:

	// 2^32 kb = 2^22MB = 2^12 Gb = 2^2 Tb 
	// 单位 KB 
	LayerLRUCache(unsigned int nCapacity);

	// 单位 KB
	unsigned int getCapacity();
	void setCapacity(unsigned int nCapacity); 

	// 返回使用对象
	thp::Bundle* get(const TBundleNo& key);
	void set(const TBundleNo& key, thp::Bundle* pBundle);

	// 用最近的使用的 bundle 占用内存信息 更新一次lru占用内存情况
	void saveUsedCapacityStatus();
	void updateUsedCapacity();

	bool lockForRead();
	bool lockForWrite();
	void unlock(bool bRead);

private:

	// 用于异常捕捉
	thp::Bundle* get_pri(const TBundleNo& key);
	void set_pri(const TBundleNo& key, thp::Bundle* pBundle);

private:
	// 单位 KB
	unsigned int m_unMaxKBCount ;
	unsigned int m_unUsedKBCount;
	unsigned int m_unLruHeadBundleKBCount;

	//QMutex m_qmutex;
	//QReadWriteLock m_qrwMutex;
	pthread_rwlock_t m_prwMutex;

	pthread_mutex_t m_plock;

	std::list< std::pair<TBundleNo, thp::Bundle*> > m_listCache;
	std::map<TBundleNo, std::list< std::pair<TBundleNo, thp::Bundle*> >::iterator > m_mp;

	LONG m_lockReadTimes;
	LONG m_lockWriteTimes;
};

}
#endif // THP_WMTS_LRU_H__
