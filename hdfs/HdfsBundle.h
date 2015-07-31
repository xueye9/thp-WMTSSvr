#ifndef THP_HDFS_BUNDLE_H__
#define THP_HDFS_BUNDLE_H__
#include "../Bundle.h"

namespace thp
{
	class HdfsBundle : public Bundle
	{
	public:
		HdfsBundle(const TBundleIDex& tNoEx);
		~HdfsBundle();

		virtual bool open(const char* szFile); 

		virtual int cache();

		virtual unsigned int getTile(int nRow, int nCol, QByteArray& arTile,int &nDetail);
	};
}


#endif // THP_HDFS_BUNDLE_H__
