#include "../StdAfx.h"
#include "WinWMTSRepository.h"
#include <io.h>
#include <sstream>
#include "../Tile.h"
#include "WinWMTSLevel.h"
#include <iostream>
#include <algorithm>
#include "../WMTSConfig.h"
#include <CLogThreadMgr.h>
#include "../WMTSFactory.h"
#include "../WMTSLayer.h"

using namespace thp;

thp::WinWMTSRepository::WinWMTSRepository()
{

}

thp::WinWMTSRepository::~WinWMTSRepository()
{

}

bool thp::WinWMTSRepository::init(int nMode)
{
	// 按文件结构初始化
	bool bSuccess = false;
	int nLayerCount = 0;
	int nFileSysType = WMTSConfig::Instance()->getFileSysType();
	if( 0x0001 == (nMode&0x0001) )
	{

		if( 0 == nFileSysType )
			nLayerCount = _initByDir();
		else
			nLayerCount = 0;
	}

	if(nLayerCount > 0)
		return true;

	return bSuccess;
}

int thp::WinWMTSRepository::_initByDir()
{
	// 搜索bdi文件
	struct _finddata64i32_t fileInfo;
	long handle;
	int done;
	QString qsFileName = WMTSConfig::Instance()->getDataDir();
	std::string sFileName = (const char*)qsFileName.toLocal8Bit(); //要搜索的文件名
	std::string sFileFullPath = sFileName + "*";
	sFileFullPath += THP_WMTS_BUNDLE_EXIST_IDXFILE_POSFIX;
	int nLayerCount = 0;

	//查找第一个文件，返回句柄
	handle=_findfirst64i32(sFileFullPath.c_str(), &fileInfo);
	if(handle==-1)
	{
		std::cerr << "目录["<< sFileName << "] 不是有效的 WMTS 目录" << std::endl;
		return false;
	}

	do
	{
		//如果是文件夹".",或者".."，则进行判断下一个文件
		if( (strcmp(fileInfo.name,".")==0) || (strcmp(fileInfo.name,"..") == 0))
			continue;

		// 跳过文件夹
		if((fileInfo.attrib&_A_SUBDIR)==_A_SUBDIR)
		{
			// log
			continue;
		}
		else
		{
			std::string sFileFullPath = sFileName;
			sFileFullPath += fileInfo.name;

			int nLen = strlen(fileInfo.name);

			//由bdi文件获取图层名
			std::string sLayerName  = fileInfo.name;
			sLayerName.erase(nLen-4);

			// windows路径不区分大小写
			if( 0 == WMTSConfig::Instance()->getFileSysType() )
				std::transform(sLayerName.begin(), sLayerName.end(), sLayerName.begin(), toupper);

			// 初始化图层
			if( _initLayer(sLayerName.c_str(), sFileFullPath.c_str()) )
				++nLayerCount;
		}
	}while( 0 == (done=_findnext64i32(handle,&fileInfo)) );

	_findclose(handle);

	return nLayerCount;
}

bool WinWMTSRepository::_initLayer(const char* szLayer, const char* szBdiPath)
{
	QString qsPath = WMTSConfig::Instance()->getDataDir();
	std::string sPath = (const char*)qsPath.toLocal8Bit();

	size_t nLen = strlen(szLayer);
	if(0 == nLen || nLen > THP_WMTS_MAX_LAYERLEN)
	{
		m_pLogWriter->warnLog("图层名过长");
		return false;
	}

	struct TLayerHashTableNode* pLayerNode = NULL;
	HASH_FIND_STR(m_layers, szLayer, pLayerNode);
	if( NULL != pLayerNode)
	{
		m_pLogWriter->warnLog("图层已存在");
		return false;
	}

	char szPath[THP_MAX_PATH];
	memset(szPath, 0, THP_MAX_PATH);
	sprintf(szPath, "%s%s\\", sPath.c_str(), szLayer);

	if( -1 == _access(szPath, 0) )
		return false;

	WMTSLayer* pNewLayer = WMTSFactory::Instance()->createLayer();
	if( !pNewLayer->setPath(szPath) )
	{
		m_pLogWriter->warnLog("图层设置参数失败");
		delete pNewLayer;
		return false;
	}

	// 图层缓存上限
	pNewLayer->setCacheMbSize( WMTSConfig::Instance()->getOneLayerMaxCacheMB() );

	// 内存管理策略
	pNewLayer->setMemStrategy( (WMTSLayer::MemStrategy)WMTSConfig::Instance()->getMemStrategy() );

	std::cout << "正在初始化图层[" << szLayer << "]" << std::endl;
	m_pLogWriter->warnLog("数据错误");
	if( 0 != pNewLayer->init(szBdiPath) )
	{
		std::cout << "初始化图层[" << szLayer << "]失败";
		m_pLogWriter->warnLog("初始化失败");
		delete pNewLayer;
		return false;
	}

	std::cout << "图层初始化成功" << std::endl;

	struct TLayerHashTableNode* pNewNode = (struct TLayerHashTableNode*)malloc( sizeof(struct TLayerHashTableNode) );
	memset(pNewNode->szName, 0, THP_WMTS_MAX_LAYERLEN);
	memcpy(pNewNode->szName, szLayer, nLen);
	pNewNode->pLayer = pNewLayer;
	HASH_ADD_STR(m_layers, szName, pNewNode);

	return true;
}