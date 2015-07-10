/*!
 * \file BundleFactory.h
 * \author Xuebingbing
 * \brief Bundle 工厂类
 *
 * TODO: long description
 * \date 六月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */


#ifndef THP_BUNDLEFACTORY_H__
#define THP_BUNDLEFACTORY_H__
#include <map>
#include "ParamDef.h"

namespace thp
{
	class Bundle;
	class WMTSLevel;

	// 不要使用该类的指针删除子类对象
	class BundleFactory
	{
	public:
		~BundleFactory();

		// eg: .\Layers\_alllayers\L18\  必需尾'\'
		bool setPath(const char* szPath);

		// 0 - success 1 - no dir
		// bdi 文件位置
		int init(const char* szBdiPath);

		Bundle* createBundle(const TBundleIDex& key);

	private:
		// 初始化一个图层的等级, 返回初始化成功的等级个数
		int _initLevels(const std::map<int, TLevelBundleExistStatus*>& mapBdi);

		// 初始化一个等级
		int _initLevel(char* szLvlPath , int nLvl, const TLevelBundleExistStatus* pNode);

		void _clear();

	protected:
		WMTSLevel* m_pLvl[THP_MAX_LEVEL];
		char m_szPath[THP_MAX_PATH];
	};
}

#endif // BundleFactory_h__
