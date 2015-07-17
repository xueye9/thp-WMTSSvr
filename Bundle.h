#ifndef BUNDLE_H__
#define BUNDLE_H__

#include <memory>

#include <QByteArray>

#ifdef _DEBUG
#include <Windows.h>
#endif

#include "ParamDef.h"

class CLogWriter;

namespace thp
{
	class Tile;
	class BundleReader;
	class LayerLRUCache;

/*!
 * \class Bundle
 *
 * \brief 以该对象为单位调入内存，使用lru算法管理内存
 * \detail 一束瓦片, 128 * 128 个行列的瓦片位一束
 * \author Baldwin
 * \date 六月 2015
 */
class Bundle
{
public:
	Bundle(const TBundleIDex& tNoEx);
	~Bundle();

	// 设置文件地址 eg：.\\L09\\R0080C0180.bundle
	bool open(const char* szFile); 

	// 清楚bundle使用的资源
	void close();

	// 整片缓存到内存
	// 缓存自己到内存
	int cache();
	bool isCached();

	// 获取最大占用内存
	unsigned int getMaxKB();

	/**
	* @brief 	 getTile
	* @details	 Bundle::getTile
	* @param[in] int nRow Tile 全局行号
	* @param[in] int nCol tile 全局列号
	* @return 	获取到非空tile返回true，否则返回false
	* @todo 	
	*/
	unsigned int getTile(int nRow, int nCol, QByteArray& arTile,int &nDetail);

	// 加热资源
	void heating();
	void setTemperature(int nDegree);
	int getTemperature() const;

	// 获取Bundle的编号
	const TBundleID& getID() const;

	const char* getPath() const;

#ifdef _DEBUG
	void check();
#endif

private:
	void _getMaxByte();
	bool _initLogWriter();
	unsigned int _getTileFromCache(int nTileIndexInBundle, QByteArray& arTile);

//public:
private:
	// 起始行号 只会128的整数倍
	int					m_nBeginRow;

	// 起始列号 只会128的整数倍
	int					m_nBeginCol;

	// 获取最大的占用内存
	unsigned int		m_unMaxByteSize;

	// 表示Bundle是否被整个缓存
	bool				m_bCached;

	// bundle文件地址
	char				m_szFilePath[THP_MAX_PATH];

	// 最后发生错误的信息
	char				m_szLastErr[256];

	// 自身读写锁
	//pthread_rwlock_t	m_prwMutex;

	// 编号
	TBundleIDex			m_tID;

	// 日志类
	CLogWriter*			m_pLogWriter;
	
	// 使用热度, 0位最低
	int m_nHeatDegree;

	// bundle资源索引
	char*				m_pBlx;

	// bundle资源内容
	char*				m_pBle;

#ifdef _DEBUG
	LONG m_lockReadTimes;
	LONG m_lockWriteTimes;
#endif
};//

}// namespace thp


#endif // BUNDLE_H__
