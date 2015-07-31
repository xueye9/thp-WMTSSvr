#ifndef THP_HDFS_WMTSLEVEL_H__
#define THP_HDFS_WMTSLEVEL_H__
#include "../WMTSLevel.h"

namespace thp
{
	class HdfsWMTSLevel : public WMTSLevel
	{
	public:
		HdfsWMTSLevel(int nLv);
		~HdfsWMTSLevel();
	};
}

#endif// THP_HDFS_WMTSLEVEL_H__