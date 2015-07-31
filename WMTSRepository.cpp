#include "StdAfx.h"
#include <io.h>
#include <sstream>
#include "WMTSRepository.h"
#include "WMTSLayer.h"
#include "Tile.h"
#include "WMTSLevel.h"
#include <iostream>
#include <algorithm>
#include "WMTSConfig.h"
#include <CLogThreadMgr.h>
#include "curl/curl.h"
#include "json/json.h"

#pragma warning(once:4996)

using namespace thp;

WMTSRepository::WMTSRepository()
{
	m_layers = NULL;
	_initLogWriter();
}


bool WMTSRepository::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("WMTSRepository.log");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("WMTSRepository", "WMTSRepository.log", "", "DebugLog"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}

WMTSRepository::~WMTSRepository()
{
	struct TLayerHashTableNode* s = NULL;
	struct TLayerHashTableNode* tmp = NULL;

	HASH_ITER(hh, m_layers, s, tmp) 
	{
		std::cout << "释放图层[" << s->szName << "]资源";

		HASH_DEL(m_layers, s);
		WMTSLayer* pLayer = s->pLayer;
		delete pLayer;
		free(s);
	}
}

unsigned int thp::WMTSRepository::getTile(const std::string& strLayer, int nLvl, int nRow, int nCol, QByteArray& arTile, int& nDetail)
{
	TLayerHashTableNode* pLyrNode = NULL;
	HASH_FIND_STR(m_layers, strLayer.c_str(), pLyrNode);
	if(NULL == pLyrNode)
		return 0;

	WMTSLayer* pLayer = pLyrNode->pLayer;
	if(NULL == pLayer)
	{	
		m_pLogWriter->errorLog("资源错误");
		return 0;
	}

	return pLayer->getTile(nLvl, nRow, nCol, arTile, nDetail);
}

//int thp::WMTSRepository::loadData(const std::string& strLayer, int nLvl)
//{
//	TLayerHashTableNode* pLyrNode = NULL;
//	HASH_FIND_STR(m_layers, strLayer.c_str(), pLyrNode);
//	if(NULL == pLyrNode)
//	{
//		return 2;
//	}
//
//	WMTSLayer* pLayer = pLyrNode->pLayer;
//	if(NULL == pLayer)
//	{	
//		return 2;
//	}
//
//	return pLayer->loadData(nLvl);
//}

std::string thp::WMTSRepository::getCapabilities()
{
	QString qsPath = WMTSConfig::Instance()->getDataDir();
	std::string sPath = (const char*)qsPath.toLocal8Bit();
	sPath.append(WMTS_REQUEST_VL_CAPABILITIES);
	sPath.append(".xml");

	return sPath;
}

