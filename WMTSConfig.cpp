#include "WMTSConfig.h"
#include "QFile"
#include "QSettings"
#include <QTextCodec>

WMTSConfig * WMTSConfig::s_pLoadConfigData = NULL;

WMTSConfig * WMTSConfig::Instance()
{
	if (s_pLoadConfigData == NULL)
	{
		s_pLoadConfigData = new WMTSConfig;
	}

	return s_pLoadConfigData;
}

void WMTSConfig::release()
{
	if (s_pLoadConfigData != NULL)
	{
		delete s_pLoadConfigData;
	}
	s_pLoadConfigData = NULL;
}


bool WMTSConfig::initConfigData(const QString& fileName)
{
	if (!QFile::exists(fileName))
	{
		return false;
	}

	/// 使用ini文件加载方式,
	QSettings serverConfig(fileName, QSettings::IniFormat);

	serverConfig.setIniCodec(QTextCodec::codecForName("UTF-8")); 

	// 加载主要的配置项
	serverConfig.beginGroup("");
	
	m_nThreadCount = serverConfig.value("ThreadCount", 4).toInt();
	m_nPort = serverConfig.value("Port", "9999").toInt();
	m_strRepositoryDir = serverConfig.value("ServerDir", ".\\").toString();

	m_nMaxOneLayerCacheMb = serverConfig.value("MemLimit", "1024").toInt();

	m_nMemStrategy = serverConfig.value("MemStrategy", "0").toUInt();

	m_strPreLoadLayerName = serverConfig.value("LAYER", "").toString();
	m_nBeginLv = serverConfig.value("LvBegin", 0).toInt();
	m_nEndLv = serverConfig.value("LvEnd", 0).toInt();

	m_nFileSysType = serverConfig.value("FileSysType").toInt();
	m_nHdfsClient  = serverConfig.value("hdfsClient").toUInt();
	m_strHdfsServer= serverConfig.value("hdfsServer").toString();
	m_nHdfsNameNodePort = serverConfig.value("hdfsNameNodePort").toInt();

	return true;
}

WMTSConfig::WMTSConfig()
{

}

WMTSConfig::~WMTSConfig()
{

}

int WMTSConfig::getPort() const
{
	return m_nPort;
}

int WMTSConfig::getMaxThreadCount() const
{
	return m_nThreadCount;
}

QString WMTSConfig::getDataDir() const
{
	return m_strRepositoryDir;
}

QString WMTSConfig::getPreLoadLayerName() const
{
	return m_strPreLoadLayerName;
}

int WMTSConfig::getBeginLevel() const
{
	return m_nBeginLv;
}

int WMTSConfig::getEndLevel() const
{
	return m_nEndLv;
}

int WMTSConfig::getOneLayerMaxCacheMB() const
{
	return m_nMaxOneLayerCacheMb;
}

int WMTSConfig::getMemStrategy() const
{
	return m_nMemStrategy;
}

int WMTSConfig::getFileSysType() const
{
	return m_nFileSysType;
}

int WMTSConfig::getHdfsClient() const
{
	return m_nHdfsClient;
}

QString WMTSConfig::getHdfsServer() const
{
	return m_strHdfsServer;
}

int WMTSConfig::getHdfsNameNodeWebPort() const
{
	return m_nHdfsNameNodePort;
}
