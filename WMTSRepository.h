/*!
 * \file WMTS.h
 * \author Xuebingbing
 * \brief wmts数据仓库
 *
 * TODO: long description
 * \date 六月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */

#ifndef THP_WMTSREPOSITORY_H__
#define THP_WMTSREPOSITORY_H__
#include <string>
#include "ParamDef.h"
#include <memory>
#include <QMutex>
#include <QByteArray>
#include "uthash/uthash.h"

class CLogWriter;
namespace thp
{
	class Tile;
	class WMTSLayer;

	// 哈希表结构定义
	struct TLayerHashTableNode
	{
		// 图层名
		char szName[THP_WMTS_MAX_LAYERLEN];

		// 图层对象指针
		thp::WMTSLayer* pLayer;

		// hash 表句柄
		UT_hash_handle hh;
	};

	/*!
	 * \class WMTS
	 *
	 * \brief WMTS瓦片数据仓库
	 *
	 * \author Baldwin
	 * \date 六月 2015
	 */
	class WMTSRepository
	{
	public:
		WMTSRepository();
		~WMTSRepository();

		// eg: .\Layers\_alllayers\L18\  必需尾'\'
		/**
		* @brief 	 setPath 设置服务目录
		* @details	 eg: .\Layers\_alllayers\L18\, 目录末尾必须有'\'
		* @param[in] const char * szPath 服务目录，图层组所在的目录
		* @return 	 bool 设置成功放回 true
		* @todo 	图层组下有用 小工具生成的 [图层名].bdi 文件
		没有bdi文件视为没有图层
		*/
		bool setPath(const char* szPath);
		
		/**
		* @brief 	 init 初始化服务
		* @details	 thp::WMTS::init
		* @param[in] int nMode
			0x0001--搜索服务目录bdi文件方式初始化
			0x0002--使用配置文件方式初始化,未实现
		* @return 	 bool 初始化成功返回true
		* @todo 	
		*/
		bool init(int nMode);

		/**
		* @brief 	 getTile	获取指定信息的瓦片
		* @details	 thp::WMTS::getTile
		* @param[in] std::string & strLayer	瓦片所在图层
		* @param[in] int nLvl	瓦片等级
		* @param[in] int nRow	瓦片行号
		* @param[in] int nCol	瓦片列号
		* @param[in] Tile * pTile  获取到的瓦片，应该用指针指针包装
			现在的这种使用方式在lru过小，同时服务一次请求的tile过多时可能造成野指针
		* @return 	 int 对返回值可以做详细的信息定制目前值如下
			0	成功
			1	失败
		* @todo 	弃用
		*/
		// 返回字节数
		unsigned int getTile(const std::string& strLayer, int nLvl, int nRow, int nCol, QByteArray& arTile, int& nDetail);
		
		/**
		* @brief 	 setMaxOccupyMemory     设置缓存上限，
		* @details	 thp::WMTS::setMaxOccupyMemory
		* @param[in] unsigned int nMemByMB
		* @return 	 void
		* @todo 	单位MB
				1）单独图层设置
				2）图层间使用内存根据使用频率只能分配
				3）每个图层都使用这个值作为缓存上限
		*/
		void setCacheMbSize(unsigned int nMemByMB);
		int getCacheMbSize() const;

		// 图层资源调度策略
		void setMemStrategy(int nMemStrategy);
		int getMemStrategy() const;

		// 装载指定图层指定等级的bundle 
		// 0-全部装载 1-部分装载 2-没有bundle被装载
		// 指定图层点的lru满了就不再装在了 
		int loadData(const std::string& strLayer, int nLvl);

	private:
		int _initByDir();
		bool _initByConfig();
		bool _initLayer(const char* szLayer, const char* szBdiPath);
		void getTile_trans(const std::string& strLayer, int nLvl, int nRow, int nCol, int& nDetail, std::tr1::shared_ptr<Tile>& spRes);
		void getTile_pri(const std::string& strLayer, int nLvl, int nRow, int nCol, int& nDetail, std::tr1::shared_ptr<Tile>& spRes);
		bool _initLogWriter();

	private:
		// 最大占用内存 单位 MB
		int						m_unMaxOccupyMemMb;

		// 服务数据仓库目录
		char					m_szPath[THP_MAX_PATH];

		// 图层 hash 表
		TLayerHashTableNode*	m_layers;

		// 图层资源调度策略
		int						m_nMemStrategy;

		// 日志类
		CLogWriter*				m_pLogWriter;
	};

}

#endif // THP_WMTSREPOSITORY_H__
