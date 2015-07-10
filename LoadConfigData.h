/**
 * @file     
 * @brief    定义配置文件加载的对象类
             

 * @author   
 * @date     
 */
#ifndef LOADCONFIGDATA_H
#define LOADCONFIGDATA_H

#include "QString"

class LoadConfigData
{

public:

	static LoadConfigData * getInstance();

	static void release();

	/// 初始化配置文件信息
	/// @param fileName[in] 配置文件名称
	/// @rreturn 初始化成功返回 true ;失败返回 false
	bool initConfigData(const QString& fileName);

	/// 获取监听端口号
	int getPort() const;

	/// 获取最大线程数
	int getMaxThreadCount() const;

	int getOneLayerMaxCacheMB() const;

	int getMemStrategy() const;

	/// 获取服务目录 eg: "D:\\thp\\data\\wmts\\",配置服务目录时注意必须带有最后的"\\"
	QString getServerDir() const;

	QString getPreLoadLayerName() const;
	int getBeginLevel() const;
	int getEndLevel() const;

private:
	LoadConfigData();
	~LoadConfigData();
private:
	///< 线程个数
	int			m_nThreadCount;

	///< 端口号
	int			m_nPort;

	///< 数据仓库目录 
	QString		m_strRepositoryDir;

	///< 每个图层使用的缓存上限
	int			m_nMaxOneLayerCacheMb;

	///< 资源策略
	int			m_nMemStrategy;

	///< 单利指针
	static LoadConfigData     *s_pLoadConfigData;

	///< 测试用加载图层数据配置图层名，图层起始等级，结束等级
	QString		m_strPreLoadLayerName;
	int			m_nBeginLv;
	int			m_nEndLv;	
};

#endif