#ifndef WIN_BUNDLE_H__
#define WIN_BUNDLE_H__
#include "../Bundle.h"

namespace thp
{
	class WinBundle : public Bundle 
	{
	public:
		WinBundle(const TBundleIDex& tNoEx);
		~WinBundle();

		virtual bool open(const char* szFile); 

		virtual int cache();

		virtual unsigned int getTile(int nRow, int nCol, QByteArray& arTile,int &nDetail);
	};
}


#endif // WIN_BUNDLE_H__
