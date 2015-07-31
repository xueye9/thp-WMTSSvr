/**
 * @file     
 * @brief    定义配置文件加载的对象类
             

 * @author   
 * @date     
 */
#ifndef LOADCONFIGDATA_H
#define LOADCONFIGDATA_H

#include "QString"

class WMTSConfig
{

public:

	static WMTSConfig * Instance();

	static void release();

	/// 初始化配置文件信息
	/// @param fileName[in] 配置文件名称
	/// @rreturn 初始化成功返回 true ;失败返回 false
	bool initConfigData(const QString& fileName);

	/// 获取监听端口号
	int getPort() const;

	/// 获取最大线程数
	int getMaxThreadCount() const;

	// 一个图层使用的内存上限
	int getOneLayerMaxCacheMB() const;

	int getMemStrategy() const;

	// 文件系统类型 0:windows文件系统 1:类unix文件系统 2: HDFS
	// eg 0: #ServerDir=D:\\thp\\data\\wmts\\
	// eg 1: #ServerDir=/thp/data/wmts/
	// eg 2: #ServerDir=/thp/data/wmts/
	int getFileSysType() const;

	//0:webhdfs,其他可扩展的链接方式：libhdfs，libhdfs3
	int getHdfsClient() const;

	QString getHdfsServer() const;

	int getHdfsNameNodeWebPort() const;

	/// 获取服务目录 eg: "D:\\thp\\data\\wmts\\",配置服务目录时注意必须带有最后的"\\"
	QString getDataDir() const;

	QString getPreLoadLayerName() const;
	int getBeginLevel() const;
	int getEndLevel() const;

private:
	WMTSConfig();
	~WMTSConfig();
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
	static WMTSConfig     *s_pLoadConfigData;

	///< 测试用加载图层数据配置图层名，图层起始等级，结束等级
	QString		m_strPreLoadLayerName;
	int			m_nBeginLv;
	int			m_nEndLv;	

	// 文件系统
	int			m_nFileSysType;

	// 连接hdfs平台
	int			m_nHdfsClient;

	// hdfs 地址
	QString		m_strHdfsServer;

	// hdfs NameNode端口号
	int			m_nHdfsNameNodePort;
};

#endif