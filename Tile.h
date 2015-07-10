/*!
 * \file Tile.h
 * \author Xuebingbing
 * \brief 瓦片抽象类定义
 *
 * TODO: long description
 * \date 六月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */

#ifndef TILE_H__
#define TILE_H__

namespace thp
{
	class Bundle;
	class Tile
	{
	public:
		Tile();
		~Tile();

		bool clone(Tile*& pTile);

		// tile 数据所占的内存字节大小
		unsigned int	m_unSize;

		// 资源分配是直接冲堆上 或者内存池
		unsigned char*	m_byteData;

		// 标示析构函数是否释放 m_byteData
		bool bRelease;
	};
}// namespace thp


#endif // TILE_H__
