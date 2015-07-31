#ifndef THP_HDFS_BUNDLEREADER_H__
#define THP_HDFS_BUNDLEREADER_H__
#include "../BundleReader.h"

namespace thp
{
	class HdfsBundleReader : public BundleReader
	{
	public:
		HdfsBundleReader();
		~HdfsBundleReader();

		// 设置文件地址 eg：.\\L09\\R0080C0180.bundle
		bool open(const char* szFile);

		bool getTileFromFile(int nTileInBundleIndex, unsigned char*& pByteTile, unsigned int& nSize);

		virtual unsigned int getMaxByte();

		// 读入全bundle内容
		virtual int readAll(char*& pBundle);
	private:
		bool _loadBundlx(const char* szFile);
	};
}


#endif // THP_HDFS_BUNDLEREADER_H__
