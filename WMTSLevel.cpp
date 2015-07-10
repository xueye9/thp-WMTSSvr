#include "WMTSLevel.h"
#include <math.h>
#include <memory.h>
#include <io.h>
#include "Bundle.h"
#include "BundleReader.h"
#include "bdi/bdiapi.h"

#pragma warning(once:4996)

using namespace thp;

WMTSLevel::WMTSLevel(int nLv)
{
	m_nLvl = nLv;

	// 检测参数异常
	//m_nBundleRowCount = 1;
	//m_nBundleColCount = 1;
	//if( nLv > 7)
	//{
	//	// 8级之前的等级只有一个 bundle(一行一列)
	//	m_nBundleRowCount = 1 << (nLv - 8); 
	//	m_nBundleColCount = 1 << (nLv - 7);;
	//}

	memset(m_szPath, 0, THP_MAX_PATH);

	m_pszBit = NULL;
}
//
//bool WMTSLevel::_initLogWriter()
//{
//	// 获取写日志对象
//	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("WMTSLevel.log");
//
//	// 如果不存在，创建日志文件，加载日志配置
//	if (m_pLogWriter == NULL)
//	{
//		CLogAppender * pLogAppender = new CLogAppender("WMTSLevel", "WMTSLevel.log", "", "General"); 
//
//		// 获取写日志对象
//		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
//	}
//	return true; 
//}

WMTSLevel::~WMTSLevel()
{
	if(NULL != m_pszBit)
	{
		delete[] m_pszBit;
		m_pszBit = NULL;
	}
}

bool WMTSLevel::setPath(const char* szPath)
{
	unsigned int unLen = strlen(szPath);
	if(0 == unLen || unLen > THP_MAX_PATH)
		return false;

	memcpy(m_szPath, szPath, unLen);
	return true;
}

bool thp::WMTSLevel::exist(const TBundleIDex& tbno)
{
	return ::isExist(m_pszBit, tbno.tID.unBundleIDinLv);
}

std::tr1::shared_ptr<Bundle> thp::WMTSLevel::getBundle(const TBundleIDex& tbno)
{
	// 检查索引，看bundle文件是否存在
	if( !::isExist(m_pszBit, tbno.tID.unBundleIDinLv) )
	{
		std::tr1::shared_ptr<Bundle> sp;
		return sp;
	}

	// 生成新的 bundle 读取 bundle
	char szBundleFile[THP_MAX_PATH];
	memset(szBundleFile, 0, THP_MAX_PATH);

	int nXR = 128 * tbno.nBundleRow;
	int nXC = 128 * tbno.nBundleCol;

	sprintf(szBundleFile, "%sR%04xC%04x.bundle", m_szPath, nXR, nXC);

	std::tr1::shared_ptr<Bundle> sp(new Bundle(tbno));
	sp->open( szBundleFile );

	return sp;
}

void WMTSLevel::setBdi(int nSize, unsigned char* szbdi)
{
	if( NULL != m_pszBit)
	{
		delete[] m_pszBit;
		m_pszBit = NULL;
	}

	m_pszBit = new unsigned char[nSize];
	memcpy(m_pszBit, szbdi, nSize);
}



