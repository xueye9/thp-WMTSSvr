/*!
 * \file BundleReader.h
 * \author Xuebingbing
 * \date 六月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */
#ifndef THP_BOUNDLEREADER_H__
#define THP_BOUNDLEREADER_H__
#include "./BundleParamDef.h"
#include <stdio.h>
#include <fstream>

//class CLogWriter;
namespace thp
{
	class Bundle;
	/*!
	 * \class BundleReader
	 *
	 * \brief ArcGis 瓦片服务压缩文件读取器
	 * \detail bundle和bundlx必须在同一目录下，且索引文件名必须和bundle文件名相同
	 * \author Baldwin
	 * \date 六月 2015
	 */
	class BundleReader
	{
	public:
		
		/**< 获取瓦片操作返回 */
		enum FetchType
		{
			// 获取成功
			FetchType_Success = 0, 
			
			// 获取失败
			FetchType_Fail,

			// 读取到最后
			FetchType_EOF,
		};


		BundleReader();
		
		// 文件地址 eg：.\\L09\\R0080C0180.bundle
		BundleReader(const char* szBundleFile);

		// 该类不建议作为继承的父类
		virtual ~BundleReader();
	
		// 设置文件地址 eg：.\\L09\\R0080C0180.bundle
		virtual bool open(const char* szFile) = 0;

		void close();

		// 拷贝索引
		bool copyBundlx(char*& pBlx, int nSize = BUNDLX_CONTENT_SIZE);
		bool loadBundlx(char* pBlx, int nSize = BUNDLX_CONTENT_SIZE);

		// 读入全bundle内容 返回bundle文件的byte数
		virtual int readAll(char*& pBundle) = 0;

		// 读取了索引才有效
		// 获取缓存bundle文件需要最大的缓存
		//unsigned int getMaxCacheSizeKB();

		// 获取bundle文件大小
		virtual unsigned int getMaxByte() = 0;

		/**
		* @brief 	 getTileFromFile
		* @details	 thp::BundleReader::getTileFromFile
		* @param[in] int nTileInBundleIndex
		* @param[in] unsigned char * & pByteTile	调用者使用 delte[] 释放内存
		* @param[in] unsigned int & nSize			Tile的二进制块大小
		* @return 	 bool							存在返回true，否则返回false
		* @todo 	不检查输入,索引必须预读好了
		*/
		virtual	bool getTileFromFile(int nTileInBundleIndex, unsigned char*& pByteTile, unsigned int& nSize) = 0;

		/**
		* @brief 	 nextTile
		* @details	 读取从当前位置开始第一张可用的瓦片，并将内部迭代器移动到下一个位置
		* @param[in,out] int & nRow  瓦片的行号
		* @param[in,out] int & nCol  瓦片列号
		* @param[in,out] unsigned char * & pOut  瓦片的二进制bolb, 该块最好通过 #releaseMem 函数释放\n
		或者通过 C-API free() 释放
		* @param[in,out] int & nSize	tile的字节数
		* @return 	 thp::BundleReader::FetchType  获取数据操作
		*/
		FetchType nextTile(int& nRow, int& nCol, unsigned char*& pOut, int& nSize/*, std::ofstream* pOsInfo*/);

	protected:
		bool _calcWorldRowCol(int nIdxPos, int& nRow, int& nCol);
		//bool _initLogWriter();

		// 
		//
		// 返回tile在bundle内部的编号 -1 遍历到最后 -2 内存不足
		int _nextTile(unsigned char*& pOut,unsigned int& nSize);

		// 存放索引文件内容
		unsigned char		m_szBundlxContents[BUNDLX_CONTENT_SIZE];
		
		char				m_szLastErr[BUNDLE_MAX_PATH];

		// 
		char				m_szBundleFile[BUNDLE_MAX_PATH];

		// tile 在bundle内的编号 索引位置 0---16383
		int					m_nIdxPos;

		// 由文件名得到的 bundle 块索引
		int					m_nBundleBeginRow;
		int					m_nBundleBeginCol;

		int					m_nMAx;
		//CLogWriter *		m_pLogWriter;
#ifdef _DEBUG
		// 可用的
		int					m_nValdTileCount;
#endif// _DEBUG
	};


}// 国科恒通


#endif // THP_BOUNDLEREADER_H__