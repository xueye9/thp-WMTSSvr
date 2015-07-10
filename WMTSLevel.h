
#ifndef WMTSLEVEL_H__
#define WMTSLEVEL_H__
#include "ParamDef.h"
#include <memory>

// bundle 存在
#define BUNDLE_STATUS_EXIST		0x01

// bundle 已经载入内存
#define BUNDLE_STATUS_LOADED	0x02

namespace thp
{
class Bundle;
class Tile;

class WMTSLevel
{
public:
	WMTSLevel(int nLv);
	~WMTSLevel();

	int level();

	bool exist(const TBundleIDex& tbno);

	// 返回值从堆上创建
	std::tr1::shared_ptr<Bundle> getBundle(const TBundleIDex& tbno);

	// 设置位置 eg: ".\Layers\_alllayers\L18\"
	bool setPath(const char* pszPath);

	void setBdi(int nSize, unsigned char* szbdi);
private:
	// 对应的目录
	char    m_szPath[THP_MAX_PATH];	

	// 等级 - 0-- (maxlv-1 = 19) 19
	int		m_nLvl;

	// <12 占1byte, >=10占2^(2*lv-18)byte 
	unsigned char*	m_pszBit; 
};

}// namespace thp


#endif // WMTSLEVEL_H__