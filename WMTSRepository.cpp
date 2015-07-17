#include "StdAfx.h"
#include <io.h>
#include "WMTSRepository.h"
#include "WMTSLayer.h"
#include "Tile.h"
#include "WMTSLevel.h"
#include <iostream>
#include <algorithm>
#include <CLogThreadMgr.h>

#pragma warning(once:4996)

using namespace thp;

WMTSRepository::WMTSRepository()
{
	// 默认最大占用 1GB 内存
	m_unMaxOccupyMemMb = THP_WMTS_DEFAULT_MEM_OCCUPY;
	memset(m_szPath, 0, THP_MAX_PATH);

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

bool WMTSRepository::setPath(const char* szPath)
{
	unsigned int unLen = strlen(szPath);
	if(0 == unLen || unLen > THP_MAX_PATH)
		return false;

	memcpy(m_szPath, szPath, unLen);
	return true;
}

bool WMTSRepository::init(int nMode)
{
	// 按文件结构初始化
	bool bSuccess = false;
	int nLayerCount = 0;
	if( 0x0001 == (nMode&0x0001) )
		nLayerCount = _initByDir();

	if(nLayerCount > 0)
		return true;

	// 通过配置文件初始化
	//if( 0x0002 == (nMode&0x0002) )
	//	bSuccess = _initByConfig();
	
	return bSuccess;
}

int WMTSRepository::_initByDir()
{
	// 搜索bdi文件
	struct _finddata64i32_t fileInfo;
	long handle;
	int done;
	std::string sFileName = m_szPath; //要搜索的文件名
	sFileName += "*";
	sFileName += THP_WMTS_BUNDLE_EXIST_IDXFILE_POSFIX;
	int nLayerCount = 0;

	//查找第一个文件，返回句柄
	handle=_findfirst64i32(sFileName.c_str(), &fileInfo);
	if(handle==-1)
	{
		std::cerr << "目录["<< m_szPath << "] 不是有效的 WMTS 目录" << std::endl;
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
			std::string sFileFullPath = m_szPath;
			sFileFullPath += fileInfo.name;

			int nLen = strlen(fileInfo.name);

			//由bdi文件获取图层名
			std::string sLayerName  = fileInfo.name;
			sLayerName.erase(nLen-4);
			
			// 初始化图层
			std::transform(sLayerName.begin(), sLayerName.end(), sLayerName.begin(), toupper);
			if( _initLayer(sLayerName.c_str(), sFileFullPath.c_str()) )
				++nLayerCount;
		}

	}while( 0 == (done=_findnext64i32(handle,&fileInfo)) );

	_findclose(handle);

	return nLayerCount;
}

bool WMTSRepository::_initLayer(const char* szLayer, const char* szBdiPath)
{
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
	sprintf(szPath, "%s%s\\", m_szPath, szLayer);

	if( -1 == _access(szPath, 0) )
		return false;

	WMTSLayer* pNewLayer = new WMTSLayer;
	if( !pNewLayer->setPath(szPath) )
	{
		m_pLogWriter->warnLog("图层设置参数失败");
		delete pNewLayer;
		return false;
	}
	
	pNewLayer->setCacheMbSize(m_unMaxOccupyMemMb);
	pNewLayer->setGetTileStrategy( (WMTSLayer::GetTileStrategy)m_nMemStrategy );

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

void thp::WMTSRepository::setCacheMbSize(unsigned int nMemByMB)
{
	m_unMaxOccupyMemMb = nMemByMB;
}

int thp::WMTSRepository::getCacheMbSize() const
{
	return m_unMaxOccupyMemMb;
}

int thp::WMTSRepository::loadData(const std::string& strLayer, int nLvl)
{
	TLayerHashTableNode* pLyrNode = NULL;
	HASH_FIND_STR(m_layers, strLayer.c_str(), pLyrNode);
	if(NULL == pLyrNode)
	{
		return 2;
	}

	WMTSLayer* pLayer = pLyrNode->pLayer;
	if(NULL == pLayer)
	{	
		return 2;
	}

	return pLayer->loadData(nLvl);
}

void thp::WMTSRepository::setMemStrategy(int nMemStrategy)
{
	m_nMemStrategy = nMemStrategy;
}

int thp::WMTSRepository::getMemStrategy() const
{
	return m_nMemStrategy;
}

std::string thp::WMTSRepository::getCapabilities()
{
	std::string sPath(m_szPath);
	sPath.append(WMTS_REQUEST_VL_CAPABILITIES);
	sPath.append(".xml");

	return sPath;
}


