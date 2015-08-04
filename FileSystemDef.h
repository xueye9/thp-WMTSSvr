/*!
 * \file FileSystemDef.h
 * \author Xuebingbing
 * \brief 文件系统枚举定义
 *
 * TODO: long description
 * \date 八月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */

/**< 文件系统类型定义 */
#ifndef THP_FILESYSTEMDEF_H__
#define THP_FILESYSTEMDEF_H__

namespace thp
{
	enum FST
	{
		WIN32_FILE_SYS = 0,
		UNIX_FILE_SYS,
		HDFS_SYS
	};
}

#endif // THP_FILESYSTEMDEF_H__