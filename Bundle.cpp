#include "StdAfx.h"
#include <memory>
#include <assert.h>
#include  <CLogThreadMgr.h>
#include <Windows.h>
#include "Bundle.h"
#include "Tile.h"
#include "BundleReader.h"
#include "LayerLRUCache.h"

using namespace thp;

thp::Bundle::Bundle(const TBundleIDex& tNoEx)
{
	m_tID = tNoEx;
	m_nBeginRow = 128 * tNoEx.nBundleRow;
	m_nBeginCol = 128 * tNoEx.nBundleCol;
	m_bCached = false;
	m_nHeatDegree = 0;
	m_pBle = NULL;
	m_pBlx = NULL;
	m_unMaxByteSize = 0;

#ifdef _DEBUG
	m_lockReadTimes = 0;
	m_lockWriteTimes = 0;
#endif

	_initLogWriter();
}

thp::Bundle::~Bundle()
{
	close();
}

void thp::Bundle::close()
{
	// 防止西沟函数抛出异常
	try
	{
		if(m_pBlx)
		{
			delete[] m_pBlx;
			m_pBlx = NULL;
		}

		if(m_pBle)
		{
			delete[] m_pBle;
			m_pBle = NULL;
		}

		m_bCached = false;
		m_nHeatDegree = 0;
		m_unMaxByteSize = 0;

#ifdef _DEBUG
		m_lockReadTimes = 0;
		m_lockWriteTimes = 0;
#endif
	}
	catch(...)
	{

	}
}

bool Bundle::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("Bundle.log");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("Bundle", "Bundle.log", "", "DebugLog"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}

unsigned int thp::Bundle::_getTileFromCache(int nTileIndexInBundle, QByteArray& arTile)
{
	unsigned int* pOffSet = (unsigned int*)(m_pBlx + nTileIndexInBundle * BUNDLX_NODE_SIZE);
	if( *pOffSet >= m_unMaxByteSize)
	{
		m_pLogWriter->warnLog("数据错误");
		//LOG(WARNING) << "数据错误";
		return 0;
	}

	int nSize = 0;
	//取四个字节读成unsigned int
	//unsigned int unSize = 0;
	//memcpy(&unSize, (m_byteBundle + *pOffSet), sizeof(unsigned int) );
	memcpy(&nSize, (m_pBle + *pOffSet), sizeof(unsigned int) );
	if(nSize >= m_unMaxByteSize)
	{
		m_pLogWriter->warnLog("数据错误");
		return 0;
	}

	if( 0 == nSize )
		return 0;

	arTile.append((m_pBle + *pOffSet + 4), nSize);

	return nSize;
}

unsigned int thp::Bundle::getMaxKB()
{
	return m_unMaxByteSize >> 10;	
}

bool thp::Bundle::isCached()
{
	return m_bCached;
}

#ifdef _DEBUG

void thp::Bundle::check()
{
	int n = m_nBeginRow%128;
	int n2 = m_nBeginCol%128;
	if(0 != n || 0 != n2)
	{
		//LOG(ERROR) << "数据错误";
		std::cout << "数据错误";
	}

	char szLv[3];
	memset(szLv, 0, 3);

	memcpy(szLv, m_szFilePath+23, 2);

	int nl = atoi(szLv);
}

#endif

void thp::Bundle::heating()
{
	InterlockedIncrement( (LONG*)(&m_nHeatDegree) );
}

int thp::Bundle::getTemperature() const
{
	int nDegree = m_nHeatDegree;
	return nDegree;
}

void thp::Bundle::setTemperature(int nDegree)
{
	m_nHeatDegree = nDegree;
}

const thp::TBundleID& thp::Bundle::getID() const
{
	return m_tID.tID;
}

const char* thp::Bundle::getPath() const
{
	return m_szFilePath;
}

QString thp::Bundle::getName() const
{
	// 得到Bundle文件名
	std::string sBundle(m_szFilePath);
	size_t nPos0 = sBundle.rfind('\\');
	if(nPos0 == std::string::npos)
		nPos0 = sBundle.rfind('/');

	size_t nPos1 = sBundle.rfind('.');
	if(nPos1 == std::string::npos)
	{
		QString qsLog = QString("%0,%1%2").arg(GB("error")).arg(GB("FILE:R ")).arg(__FILE__);
		m_pLogWriter->errorLog(qsLog);
		return "";
	}

	std::string sBleName = sBundle.substr(nPos0+1, (nPos1 - nPos0 - 1));

	// 获取等级
	size_t nPos2 = sBundle.rfind('\\', nPos0-1);
	if(nPos0 == std::string::npos)
		nPos0 = sBundle.rfind('/', nPos0);

	std::string sLv = sBundle.substr(nPos2+1, nPos0 - nPos2 - 1);
	if(sLv.empty())
	{
		QString qsLog = QString("%0,%1%2").arg(GB("error")).arg(GB("FILE:R ")).arg(__FILE__);
		m_pLogWriter->errorLog(qsLog);
		return "";
	}

	QString qsName = QString("%1%2").arg( GB(sLv.c_str()) ).arg( GB(sBleName.c_str()) );

	return qsName;
}
