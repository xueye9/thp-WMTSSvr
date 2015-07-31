/*!
 * \file WMTSLayer.h
 * \author Xuebingbing
 * \brief 一个WMTS图层抽象
 *
 * TODO: 1.负责外部获取Tile; 2.负责创建bundle但不负责释放
 * \date 六月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */

#ifndef THP_WMTS_H__
#define THP_WMTS_H__

#include <memory>
#include <QByteArray>
#include "ParamDef.h"
#include "bdi/bdiapi.h"
#include "uthash/uthash.h"
#include <QMutex>
#include <QReadWriteLock>
#include "Bundle.h"

#ifdef _THP_TJ
#include <Windows.h>
#endif// _THP_TJ

class CLogWriter;
namespace thp
{
	class Tile;
	class WMTSLevel;
	class LayerLRUCache;

	// Bundle资源hash表记录
	struct TBundleRecord : public TBundleIDex
	{
		// 资源若引用
		std::tr1::weak_ptr<Bundle> wpBundle;

		// 资源读写索
		QReadWriteLock rwLocker;

		// hash表句柄
		UT_hash_handle hh;

		// 加载资源
		std::tr1::shared_ptr<Bundle> loadBundle(WMTSLevel* pLv);

		TBundleRecord();
		~TBundleRecord();
	};

	class WMTSLayer 
	{
	public:
		/**< 获取tile策略 */
		enum MemStrategy 
		{
			/**< 直接IO不进行缓存 */
			GTS_IO = 0,

			/**< 先IO后后放入缓存 */
			GTS_MEM,
		};

	public:
		WMTSLayer();
		virtual ~WMTSLayer();

		bool setPath(const char* szPath);

		// 0 - success 1 - no dir
		// bdi 文件位置
		virtual int init(const char* szBdiPath) = 0;

		// 获取响应的等级
		WMTSLevel* getLevel(int nLvl);

		/**
		* @brief 	 getTile 获取瓦片
		* @details	 thp::WMTSLayer::getTile
		* @param[in] int nLvl   0...21
		* @param[in] int nRow	瓦片行号
		* @param[in] int nCol	瓦片列号
		* @param[in] int & nDetail	获取的详细信息 待定
		* @return 	 const Tile*
		* @todo 	
		*/
		virtual unsigned int getTile(int nLvl, int nRow, int nCol, QByteArray& arTile, int& nDetail) = 0;

		// 设置缓存上限
		void setCacheMbSize(unsigned int nMemByMB);

		// 内存管理策略
		void setMemStrategy(MemStrategy eGTS);
		MemStrategy getMemStrategy() const;

		// 装在数据,测试使用
		int loadData(int nLvl);

	protected:
		// 计算指定等级的bundle编号
		void _calcBundleNo(int nLvl, int nRow, int nCol, TBundleIDex& tNo);

		// 计算等级下bundle的最大总数
		int _clacBundleCount(int nLvl);

		bool _initLogWriter();

	protected:
		int						nID;

		// 定义图层范围以方便信息的刷新 非必需 实现一优化服务开启速度
		double					m_dLeft;
		double					m_dBottom;
		double					m_dRight;
		double					m_dTop;

		// 缓存管理器
		LayerLRUCache*			m_pLyrLRU;

		// Bundle资源列表
		TBundleRecord*			m_pBundleRecords;

		// Bundle资源列表互斥索,只有add资源的时候会加所
		QMutex						m_pRecordsMutex;

		// Bundle装在策略 
		MemStrategy			m_eGTS;

		// 日志读写器
		CLogWriter *			m_pLogWriter;

		char m_szPath[THP_MAX_PATH];

		WMTSLevel* m_pLvl[THP_MAX_LEVEL];

#ifdef _THP_TJ
		// 总访问次数
		LONG			m_nCount;

		// 有效访问次数
		LONG			m_nENum;

		// 从内存获取的次数
		LONG			m_nMNum;
#endif// _THP_TJ
	};
}


#endif // THP_WMTS_H__