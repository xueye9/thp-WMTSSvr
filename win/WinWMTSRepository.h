#ifndef WIN_REPOSITORY_H__
#define WIN_REPOSITORY_H__
#include "../WMTSRepository.h"

namespace thp
{
	class WinWMTSRepository : public WMTSRepository
	{
	public:
		WinWMTSRepository();
		~WinWMTSRepository();

		virtual bool init(int nMode);

		virtual void showStatus();

	private:
		int _initByDir();
		bool _initLayer(const char* szLayer, const char* szBdiPath);
	};
}

#endif // WIN_REPOSITORY_H__
