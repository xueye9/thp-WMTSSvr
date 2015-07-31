#ifndef WIN_BUNDLEREADER_H__
#define WIN_BUNDLEREADER_H__
#include "../BundleReader.h"

namespace thp
{
	class WinBundleReader : public BundleReader
	{
	public:
		WinBundleReader();
		~WinBundleReader();

		// 设置文件地址 eg：.\\L09\\R0080C0180.bundle
		virtual bool open(const char* szFile);

		virtual bool getTileFromFile(int nTileInBundleIndex, unsigned char*& pByteTile, unsigned int& nSize);

		virtual unsigned int getMaxByte();

		// 读入全bundle内容
		virtual int readAll(char*& pBundle);
	private:
		bool _loadBundlx(const char* szFile);
	};
}


#endif // WIN_BUNDLEREADER_H__
