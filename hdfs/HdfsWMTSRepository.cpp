#include "../StdAfx.h"
#include "HdfsWMTSRepository.h"
#include <io.h>
#include <sstream>
#include "../Tile.h"
//#include "HdfsWMTSLevel.h"
#include <iostream>
#include <algorithm>
#include "../WMTSConfig.h"
#include <CLogThreadMgr.h>
#include "../WMTSFactory.h"
#include "../WMTSLayer.h"
#include "json/json.h"
#include "curl/curl.h"
#include "HdfsUrl.h"

using namespace thp;

HdfsWMTSRepository::HdfsWMTSRepository()
{

}

HdfsWMTSRepository::~HdfsWMTSRepository()
{
	curl_global_cleanup();
}

bool thp::HdfsWMTSRepository::init(int nMode)
{
	// 初始化curl
	curl_global_init(CURL_GLOBAL_ALL);

	// 按文件结构初始化
	bool bSuccess = false;
	int nLayerCount = 0;
	int nFileSysType = WMTSConfig::Instance()->getFileSysType();
	if( 0x0001 == (nMode&0x0001) )
	{
		nLayerCount = _initByDirWithWebhdfs();
	}
	else
		nLayerCount = 0;

	if(nLayerCount > 0)
		return true;

	return bSuccess;
}

int HdfsWMTSRepository::_initByDirWithWebhdfs()
{
	// TODO: 搜索hdfs
	QString qsHdfsServer = WMTSConfig::Instance()->getHdfsServer();
	if( qsHdfsServer.isEmpty() )
	{
		QString qsError = QString("hdfs,初始化失败,服务地址缺失").arg(qsHdfsServer);
		m_pLogWriter->debugLog(qsError);
	}
	std::string sHdfsServer = (const char*)qsHdfsServer.toLocal8Bit();
	int nHdfsNNPort = WMTSConfig::Instance()->getHdfsNameNodeWebPort();

	// 要搜索的目录 图层组目录
	QString qsDataDir = WMTSConfig::Instance()->getDataDir(); 
	if( qsDataDir.isEmpty() )
	{
		QString qsError = QString("hdfs,初始化失败,数据目录缺失").arg(qsDataDir);
		m_pLogWriter->debugLog(qsError);
	}
	std::string sDataDir = (const char*)qsDataDir.toLocal8Bit();
	int nLayerCount = 0;

	// 格式化 webhdfs url
	std::stringstream ss;
	ss << "http://" << sHdfsServer << ":" << nHdfsNNPort
		<< "/webhdfs/v1";
		//<< sDataDir;

	std::string sBaseUrl = ss.str();

	// 格式化搜索 webhdfs url
	std::string sLsUrl = sBaseUrl + sDataDir + "?op=LISTSTATUS";

	// 搜索目录
	CURL* curlHandle = curl_easy_init();

	// 设置获取详细的联系信息 当前为打开状态可以关闭 掉 
	CURLcode res;

	QByteArray arDirectoryStatus;

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 设置数据写入对象
	res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arDirectoryStatus);

	// 设置请求url
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sLsUrl.c_str() );

	res = curl_easy_perform(curlHandle);
	if( CURLE_OK != res )
	{
		QString sLog = QString("%0,%1,%2").arg("webhdfs").arg("op=LISTSTATUS").arg(GB("请求失败"));
		m_pLogWriter->debugLog(sLog);
		curl_easy_cleanup(curlHandle);

		return nLayerCount;
	}

	QString qs = arDirectoryStatus;

	curl_easy_cleanup(curlHandle);

	std::string sJson = (const char*)( qs.toLocal8Bit() );

	// 解析json串 
	Json::Reader reader;  
	Json::Value root;  
	if ( reader.parse(sJson, root) )  // reader将Json字符串解析到root，root将包含Json里所有子元素  
	{  
		Json::Value vlFileStatuses = root["FileStatuses"];
		Json::Value vlFileStatus = vlFileStatuses["FileStatus"];
		Json::Value::UInt nFileCount = vlFileStatus.size();
		for (Json::Value::UInt i = 0; i < nFileCount; ++i)
		{
			Json::Value vlTemp = vlFileStatus[i];
			std::string sType = vlTemp["type"].asString();

			// 跳过不是文件的目录对象
			if( 0 != sType.compare("FILE") )
				continue;

			std::string spathSuffix = vlTemp["pathSuffix"].asString();
			size_t nDotPos = spathSuffix.rfind('.');
			if( std::string::npos == spathSuffix.rfind('.') )
				continue;

			// 过滤不是索引的文件
			std::string sFilesuffix = spathSuffix.substr(nDotPos);
			if( 0 != sFilesuffix.compare(".bdi") )
				continue;

			std::string sBdiPath = sDataDir + spathSuffix;
			//std::string sBdiPath = sDataDir + sFilesuffix;

			std::string sLayerName  = spathSuffix;
			sLayerName.erase(spathSuffix.size() - 4);

			if( _initLayerWithWebhdfs(sLayerName.c_str(), sBaseUrl.c_str(), sBdiPath.c_str()) )
				++nLayerCount;
		}
	}  

	return nLayerCount;
}

bool HdfsWMTSRepository::_initLayerWithWebhdfs(const char* szLayer, const char* szBaseUrl, const char* szBdiPath)
{
	size_t nLen = strlen(szLayer);
	if(0 == nLen || nLen > THP_WMTS_MAX_LAYERLEN)
	{
		QString qsError = QString("hdfs,初始化失败,图层名过长(路径长度限制%0)").arg(THP_WMTS_MAX_LAYERLEN);
		m_pLogWriter->warnLog(qsError);
		return false;
	}

	QString qsPath = WMTSConfig::Instance()->getDataDir();
	std::string sPath = (const char*)qsPath.toLocal8Bit();

	struct TLayerHashTableNode* pLayerNode = NULL;
	HASH_FIND_STR(m_layers, szLayer, pLayerNode);
	if( NULL != pLayerNode)
	{
		QString qsError = QString("hdfs,初始化失败,图层(%s)已存在").arg( GB(szLayer) );
		m_pLogWriter->warnLog(qsError);
		return false;
	}

	char szPath[THP_MAX_PATH];
	memset(szPath, 0, THP_MAX_PATH);
	sprintf(szPath, "%s%s/", sPath.c_str(), szLayer);

	WMTSLayer* pNewLayer = WMTSFactory::Instance()->createLayer();
	if( !pNewLayer->setPath(szPath) )
	{
		QString qsError = QString("hdfs,初始化失败,图层(%0)设置参数失败").arg( GB(szLayer) );
		m_pLogWriter->warnLog(qsError);
		delete pNewLayer;
		return false;
	}

	// 图层缓存上限
	pNewLayer->setCacheMbSize( WMTSConfig::Instance()->getOneLayerMaxCacheMB() );

	// 内存管理策略
	pNewLayer->setMemStrategy( (WMTSLayer::MemStrategy)WMTSConfig::Instance()->getMemStrategy() );

	std::cout << "初始化图层[" << szLayer << "]" << std::endl;

	std::string sBdiUrl = szBaseUrl;
	sBdiUrl += szBdiPath;
	if( 0 != pNewLayer->init(sBdiUrl.c_str()) )
	{
		std::cout << "初始化图层[" << szLayer << "]失败";
		QString qsError = QString("hdfs,初始化失败,图层(%0)初始化失败").arg( GB(szLayer) );
		m_pLogWriter->warnLog("初始化失败");
		delete pNewLayer;
		return false;
	}

	std::cout << "图层[" << szLayer << "]初始化成功" << std::endl;

	struct TLayerHashTableNode* pNewNode = (struct TLayerHashTableNode*)malloc( sizeof(struct TLayerHashTableNode) );
	memset(pNewNode->szName, 0, THP_WMTS_MAX_LAYERLEN);
	memcpy(pNewNode->szName, szLayer, nLen);
	pNewNode->pLayer = pNewLayer;
	HASH_ADD_STR(m_layers, szName, pNewNode);

	return true;
}