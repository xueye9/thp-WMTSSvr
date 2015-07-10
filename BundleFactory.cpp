#include "BundleFactory.h"
#include "WMTSLevel.h"
#include "Bundle.h"
#include "ParamDef.h"
#include <stdio.h>
#include <io.h>
#include "bdi/bdiapi.h"

using namespace thp;

thp::BundleFactory::~BundleFactory()
{
	_clear();
}

bool BundleFactory::setPath(const char* szPath)
{
	unsigned int unLen = strlen(szPath);
	if(0 == unLen || unLen > THP_MAX_PATH)
		return false;

	memcpy(m_szPath, szPath, unLen);

	return true;
}

int BundleFactory::init(const char* szBdiPath)
{
	if( -1 == _access(m_szPath, 0) )
	{
		// write log WMTS 服务图层数据初始化失败,原因：图层目录不存在
		return 1;
	}

	if( -1 == _access(szBdiPath, 0) )
	{
		// write log WMTS 服务图层数据初始化失败,原因：图层目录不存在
		printf("指定bdi文件[%s]不存在", szBdiPath);
		return 1;
	}

	std::map<int, TLevelBundleExistStatus*> mapBdi;
	readLayerDbi(szBdiPath, mapBdi);

	int nRes = _initLevels(mapBdi);

	if(  !mapBdi.empty() )
	{
		std::map<int, TLevelBundleExistStatus*>::iterator it = mapBdi.begin();
		for( ;it != mapBdi.end(); ++it)
			delete it->second;
	}

	return nRes>0?0:1;
}

// 0-success
int BundleFactory::_initLevels(const std::map<int, TLevelBundleExistStatus*>& mapBdi)
{
	char szLvlPath[THP_MAX_PATH];
	memset(szLvlPath, 0, THP_MAX_PATH);
	std::map<int, TLevelBundleExistStatus*>::const_iterator it = mapBdi.begin();
	int nSuccessLv = 0;
	for (int i=0; i < THP_MAX_LEVEL; ++i)
	{
		it = mapBdi.find(i);
		if(mapBdi.end() == it)
		{
			//std::cerr<<"索引错误"<<std::endl;
			return -1;
		}

		const TLevelBundleExistStatus* pNode = it->second;
		if( NULL == pNode || 0 == pNode->nSize )
		{
			m_pLvl[i] = NULL;
			continue;
		}

		if( 0 != _initLevel(szLvlPath, i, pNode) )
		{
			m_pLvl[i] = NULL;
		}

		++nSuccessLv;
	}

	return nSuccessLv;
}

// 0--success 
int BundleFactory::_initLevel(char* szLvlPath , int nLvl, const TLevelBundleExistStatus* pNode)
{
	sprintf(szLvlPath, "%s\L%02d\\", m_szPath, nLvl);
	// 初始化 level
	if( 0 == _access(szLvlPath, 0) )
	{
		m_pLvl[nLvl] = new WMTSLevel(nLvl);
		m_pLvl[nLvl]->setPath(szLvlPath);

		m_pLvl[nLvl]->setBdi(pNode->nSize, pNode->pbyteIndex);

		return 0;

		// log lvl init success
	}// 0 levle exist 
	else
	{
		return 1;// log lvl init fail
	}
}

Bundle* thp::BundleFactory::createBundle(const TBundleIDex& key)
{
	return NULL;
}

void thp::BundleFactory::_clear()
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
