#include "WMTSFactory.h"
#include "./win/WinWMTSLayer.h"
#include "./win/WinWMTSRepository.h"
#include "./win/WinWMTSLevel.h"
#include "./win/WinBundle.h"
#include "./win/WinBundleReader.h"

#include "./hdfs/HdfsWMTSRepository.h"
#include "./hdfs/HdfsWMTSLayer.h"
#include "./hdfs/HdfsWMTSLevel.h"
#include "./hdfs/HdfsBundle.h"

#ifndef NULL
#define NULL 0
#endif// #ifndef NULL

using namespace thp;

WMTSFactory* WMTSFactory::_ins = NULL;

WMTSFactory::WMTSFactory()
{
	m_eType = WIN32_FILE_SYS;
}

WMTSFactory::~WMTSFactory()
{
	
}

WMTSFactory* thp::WMTSFactory::Instance()
{
	if(NULL == _ins)
		_ins = new WMTSFactory();

	return _ins;
}

void thp::WMTSFactory::DeInstance()
{
	delete _ins;
	_ins = NULL;
}

WMTSRepository* thp::WMTSFactory::createRepository()
{
	WMTSRepository* pResult = NULL;
	switch(m_eType)
	{
	case WIN32_FILE_SYS:
		pResult = new WinWMTSRepository;
		break;

	case HDFS_SYS:
		pResult = new HdfsWMTSRepository;
		break;

	default:
		break;
	}

	return pResult;
}

WMTSLayer* thp::WMTSFactory::createLayer()
{
	WMTSLayer* pResult = NULL;
	switch(m_eType)
	{
	case WIN32_FILE_SYS:
		pResult = new WinWMTSLayer;
		break;

	case HDFS_SYS:
		pResult = new HdfsWMTSLayer;
		break;

	default:
		break;
	}

	return pResult;
}

WMTSLevel* thp::WMTSFactory::createLevel(int nLv)
{
	WMTSLevel* pResult = NULL;
	switch(m_eType)
	{
	case WIN32_FILE_SYS:
		pResult = new WinWMTSLevel(nLv);
		break;

	case HDFS_SYS:
		pResult = new HdfsWMTSLevel(nLv);
		break;

	default:
		break;
	}

	return pResult;
}

Bundle* thp::WMTSFactory::createBundle(const TBundleIDex& tNoEx)
{
	Bundle* pResult = NULL;
	switch(m_eType)
	{
	case WIN32_FILE_SYS:
		pResult = new WinBundle(tNoEx);
		break;

	case HDFS_SYS:
		pResult = new HdfsBundle(tNoEx);
		break;

	default:
		break;
	}

	return pResult;
}

BundleReader* thp::WMTSFactory::createBundleReader()
{
	BundleReader* pResult = NULL;
	switch(m_eType)
	{
	case WIN32_FILE_SYS:
		pResult = new WinBundleReader();
		break;

	default:
		break;
	}

	return pResult;
}

void thp::WMTSFactory::setFileSys(FST efs)
{
	m_eType = efs;
}

thp::FST thp::WMTSFactory::getFileSys()
{
	return m_eType;
}




