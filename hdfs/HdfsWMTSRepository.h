#ifndef HDFS_REPOSITORY_H__
#define HDFS_REPOSITORY_H__
#include "../WMTSRepository.h"
#include "curl/curl.h"



namespace thp
{
	class HdfsWMTSRepository : public WMTSRepository
	{
	public:
		HdfsWMTSRepository();
		~HdfsWMTSRepository();

		virtual bool init(int nMode);

	private:
		int _initByDirWithWebhdfs();

		bool _initLayerWithWebhdfs(const char* szLayer, const char* szBaseUrl, const char* szBdiPath);
	};
}

#endif // HDFS_REPOSITORY_H__
