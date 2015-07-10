#include "LoadConfigData.h"
#include "QFile"
#include "QSettings"
#include <QTextCodec>

LoadConfigData * LoadConfigData::s_pLoadConfigData = NULL;

LoadConfigData * LoadConfigData::getInstance()
{
	if (s_pLoadConfigData == NULL)
	{
		s_pLoadConfigData = new LoadConfigData;
	}

	return s_pLoadConfigData;
}

void LoadConfigData::release()
{
	if (s_pLoadConfigData != NULL)
	{
		delete s_pLoadConfigData;
	}
	s_pLoadConfigData = NULL;
}


bool LoadConfigData::initConfigData(const QString& fileName)
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

	return true;
}

LoadConfigData::LoadConfigData()
{

}

LoadConfigData::~LoadConfigData()
{

}

int LoadConfigData::getPort() const
{
	return m_nPort;
}

int LoadConfigData::getMaxThreadCount() const
{
	return m_nThreadCount;
}

QString LoadConfigData::getServerDir() const
{
	return m_strRepositoryDir;
}

QString LoadConfigData::getPreLoadLayerName() const
{
	return m_strPreLoadLayerName;
}

int LoadConfigData::getBeginLevel() const
{
	return m_nBeginLv;
}

int LoadConfigData::getEndLevel() const
{
	return m_nEndLv;
}

int LoadConfigData::getOneLayerMaxCacheMB() const
{
	return m_nMaxOneLayerCacheMb;
}

int LoadConfigData::getMemStrategy() const
{
	return m_nMemStrategy;
}
