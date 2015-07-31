#include "../StdAfx.h"
#include "HdfsWMTSLayer.h"
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <QString>
#include <CLogThreadMgr.h>
#include <CLogWriter.h>
#include "../WMTSLevel.h"
#include "../LayerLRUCache.h"
#include "../Tile.h"
#include "../WMTSLayer.h"
#include "../Bundle.h"
#include "../bdi/bdiapi.h"
#include "./HdfsUrl.h"
#include "../WMTSFactory.h"

using namespace thp;

thp::HdfsWMTSLayer::~HdfsWMTSLayer()
{
	_clear();
}

int HdfsWMTSLayer::init(const char* szBdiPath)
{
	std::map<int, TLevelBundleExistStatus*> mapBdi;

	if( !readBdiWithWebhdfs(szBdiPath, mapBdi) )
		return 1;

	// 处理成图层目录
	std::string sLayerPath = szBdiPath;
	size_t nPos = sLayerPath.rfind(".bdi");
	sLayerPath.erase(nPos, 4);
	sLayerPath.append("/");

	int nRes = 0;
	nRes = _initLevels( sLayerPath.c_str(), mapBdi);

	if(  !mapBdi.empty() )
	{
		std::map<int, TLevelBundleExistStatus*>::iterator it = mapBdi.begin();
		for( ;it != mapBdi.end(); ++it)
			delete it->second;
	}

	return nRes>0?0:1;
}

unsigned int HdfsWMTSLayer::getTile(int nLvl, int nRow, int nCol, QByteArray& arTile, int& nDetail)
{
#ifdef _THP_TJ
	InterlockedIncrement( (LONG*)(&m_nCount) );
#endif// _THP_TJ

	if( nLvl > THP_MAX_LEVEL)
	{
		QString qsInfo = QString( GB("%0,%1,%2,%3,大于最大等级") ).arg( GB("info") ).arg(nLvl).arg(nRow).arg(nCol);
		m_pLogWriter->debugLog(qsInfo);
		return 0;
	}// 超出定义最大范围

	if( nRow < 0 || nRow > (1 << (nLvl-1)) )
	{
		QString qsInfo = QString( GB("%0,%1,%2,%3,参数越界") ).arg( GB("info") ).arg(nLvl).arg(nRow).arg(nCol);
		m_pLogWriter->debugLog(qsInfo);
		return 0;
	}// 超出范围定义的瓦片行数

	if( nCol < 0 || nCol > (1 << nLvl) )
	{
		QString qsInfo = QString( GB("%0,%1,%2,%3,参数越界") ).arg( GB("info") ).arg(nLvl).arg(nRow).arg(nCol);
		m_pLogWriter->debugLog(qsInfo);
		return 0;
	}// 超出瓦片列数

	// 没有对应的等级
	WMTSLevel* pLv = m_pLvl[nLvl];
	if( NULL == pLv )
	{
		QString qsInfo = QString( GB("%0,%1,%2,%3,没有对应等级") ).arg( GB("info") ).arg(nLvl).arg(nRow).arg(nCol);
		m_pLogWriter->debugLog(qsInfo);
		return 0;
	}

	TBundleIDex tbnNo;
	_calcBundleNo(nLvl, nRow, nCol, tbnNo);

	// 测试缓存
	std::tr1::shared_ptr<Bundle> spBundle; 

	// 查找资源
	TBundleRecord* pRecord = NULL;
	HASH_FIND(hh, m_pBundleRecords, &(tbnNo.tID), sizeof(TBundleID), pRecord);

	// 获取操作成功但是没有对应的数据则尝试创建
	if( NULL == pRecord )
	{
		// bundle 不存在
		if( !pLv->exist(tbnNo) )
		{
			QString qsInfo = QString( GB("%0,%1,%2,%3,没有对应bundle") ).arg( GB("info") ).arg(nLvl).arg(nRow).arg(nCol);
			m_pLogWriter->debugLog(qsInfo);
			return 0;
		}

		{
			QMutexLocker locker(&m_pRecordsMutex);

			HASH_FIND(hh, m_pBundleRecords, &(tbnNo.tID), sizeof(TBundleID), pRecord);
			if(NULL == pRecord)
			{
				pRecord = new TBundleRecord;

				pRecord->tID = tbnNo.tID;
				pRecord->nBundleRow = tbnNo.nBundleRow;
				pRecord->nBundleCol = tbnNo.nBundleCol;

				HASH_ADD(hh, m_pBundleRecords, tID, sizeof(TBundleID), pRecord);

#ifdef _THP_TJ
				// 记录访问内存过bundle的文件大小
				spBundle = pRecord->loadBundle(pLv);
				unsigned int nbKb = spBundle->getMaxKB();

				QString qsInfo = QString( GB("%0,%1,%2") ).arg( GB("FILE") ).arg( spBundle->getPath() ).arg( nbKb );
				m_pLogWriter->debugLog(qsInfo);
#endif// _THP_TJ

			}
		}
	}// 加载bundle

	spBundle = pRecord->loadBundle(pLv);

	// 资源无法创建返回，没有对应的切片文件,服务没有对应的bundle资源
	if( NULL == spBundle.get() )
	{
		QString sErrInfo = QString("none tile resource, %0,%1,%2,%3");
		m_pLogWriter->errorLog( GB("IO资源错误") );
		return 0;
	}

	// 保证有 8 个以上的bundle可以缓存
	if( (GTS_MEM == m_eGTS) && (spBundle->getMaxKB() < (m_pLyrLRU->getCapacity() >> 3)) )
	{
		if( !spBundle->isCached() )
		{
			spBundle->cache();
		}

		m_pLyrLRU->add(spBundle);
	}

	if(spBundle->getID().unLv < 0 || spBundle->getID().unLv > 21)
	{
		m_pLogWriter->errorLog( GB("IO资源错误") );
		return 0;
	}

	return spBundle->getTile(nRow, nCol, arTile, nDetail);
}

bool thp::HdfsWMTSLayer::readBdiWithWebhdfs(const std::string& sUrl, std::map<int, TLevelBundleExistStatus*>& idxMap)
{
	QByteArray qaBdi;

	// 读取bdi
	CURL* curlHandle = curl_easy_init();

	// 设置获取详细的联系信息 当前为打开状态可以关闭掉 
	CURLcode res;

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 设置数据写入对象
	res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &qaBdi);

	// 设置请求url
	std::string sOpenBdiUrl = sUrl + "?op=OPEN";
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sOpenBdiUrl.c_str() );

	res = curl_easy_perform(curlHandle);
	if( CURLE_OK != res )
	{
		QString sLog = QString("%0,%1,%2").arg("webhdfs").arg("op=LISTSTATUS").arg(GB("请求失败"));
		m_pLogWriter->debugLog(sLog);
	}

	curl_easy_cleanup(curlHandle);

	// 解析索引
	if( qaBdi.size() < 16 )
		return false;
	
	int nPtr = 16;
	int i = 0;

	int nSize = qaBdi.size();
	bool bSuccess = true;
	int nByteCount = 0;
	int nLv = 0;
	for (i=0; i < MAX_LEVLE; ++i)
	{
		if(nPtr + sizeof(int) > nSize)
			break;
		memcpy(&nLv, qaBdi.data() + nPtr, sizeof(int));
		nPtr += sizeof(int);

		if(nPtr + sizeof(int) > nSize)
			break;
		memcpy(&nByteCount, qaBdi.data() + nPtr, sizeof(int));
		nPtr += sizeof(int);

		if(0 == nByteCount)
		{
			idxMap.insert( std::make_pair(i, (TLevelBundleExistStatus*)NULL) );
			continue;
		}

		TLevelBundleExistStatus* pNode = new TLevelBundleExistStatus;
		pNode->nSize = nByteCount;

		pNode->pbyteIndex = new unsigned char[nByteCount];
		memcpy((void*)(pNode->pbyteIndex), qaBdi.data() + nPtr, nByteCount);
		nPtr += nByteCount;
		idxMap.insert( std::make_pair(i, pNode) );
	}

	return bSuccess;
}

int HdfsWMTSLayer::_initLevels(const char* szLayerUrl, const std::map<int, TLevelBundleExistStatus*>& mapBdi)
{
	std::map<int, TLevelBundleExistStatus*>::const_iterator it = mapBdi.begin();
	int nSuccessLv = 0;
	for (int i=0; i < THP_MAX_LEVEL; ++i)
	{
		it = mapBdi.find(i);
		if(mapBdi.end() == it)
		{
			std::cerr << "索引错误" << std::endl;
			return -1;
		}

		const TLevelBundleExistStatus* pNode = it->second;
		if( NULL == pNode || 0 == pNode->nSize )
		{
			m_pLvl[i] = NULL;
			continue;
		}

		if( 0 != _initLevel(szLayerUrl, i, pNode) )
		{
			m_pLvl[i] = NULL;
		}

		++nSuccessLv;
	}

	return nSuccessLv;
}

int HdfsWMTSLayer::_initLevel(const char* szPath , int nLvl, const TLevelBundleExistStatus* pNode)
{
	char szLvlPath[THP_MAX_PATH];
	memset(szLvlPath, 0, THP_MAX_PATH);
	sprintf(szLvlPath, "%sL%02d/", szPath, nLvl);
	
	// 初始化 level
	m_pLvl[nLvl] = WMTSFactory::Instance()->createLevel(nLvl); 
	m_pLvl[nLvl]->setPath(szLvlPath);
	m_pLvl[nLvl]->setBdi(pNode->nSize, pNode->pbyteIndex);

	return 0;
}

void HdfsWMTSLayer::_clear()
{
	try
	{
		int i = 0;
		for (; i < THP_MAX_LEVEL; ++i)
		{
			if(NULL == m_pLvl[i])
				continue;

			delete m_pLvl[i];
			m_pLvl[i] = NULL;
		}
	}
	catch (...)
	{

	}
}

