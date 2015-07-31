#ifndef WIN_WMTSLAYER_H__
#define WIN_WMTSLAYER_H__
#include "../WMTSLayer.h"

namespace thp
{
	class WinWMTSLayer : public WMTSLayer
	{
	public:
		~WinWMTSLayer();

		virtual int init(const char* szBdiPath);

		virtual unsigned int getTile(int nLvl, int nRow, int nCol, QByteArray& arTile, int& nDetail);

	private:
		// 初始化一个图层的等级, 返回初始化成功的等级个数
		int _initLevels(const std::map<int, TLevelBundleExistStatus*>& mapBdi);

		// 初始化一个等级
		int _initLevel(char* szLvlPath , int nLvl, const TLevelBundleExistStatus* pNode);

		void _clear();

	};
}



#endif // WIN_WMTSLAYER_H__
