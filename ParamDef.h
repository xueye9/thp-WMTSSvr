#ifndef THP_WMTSPARAMDEF_H__
#define THP_WMTSPARAMDEF_H__

#define THP_WMTS_MAX_LAYERLEN	30

#define THP_MAX_PATH	512

// 最大切片等级
#define THP_MAX_LEVEL	22

// 单位MB
#define THP_WMTS_DEFAULT_MEM_OCCUPY		512//1024

// 单位kb
#define THP_WMTS_DEFAULT_LRU_CACHE      1048576 // 1024*1024 1GB

#define THP_WMTS_BUNDLE_EXIST_IDXFILE_POSFIX	".bdi"

//localhost:9092/WMTS?service=WMTS&request=GetTile&version=1.0.0&
//layer=img&style=default&format=tiles&TileMatrixSet=c&TileMatrix=3&TileRow=2&TileCol=2
#define WMTS_SERVICE		"SERVICE"
#define WMTS_REQUEST		"REQUEST"
#define WMTS_VERSION		"VERSION"
#define WMTS_LAYER			"LAYER"
#define WMTS_LAYER_STYLE	"STYLE"
#define WMTS_TILE_FORMAT	"FORMAT"
#define WMTS_TILEMATRIXSET	"TILEMATRIXSET"
#define WMTS_TILEMATRIX		"TILEMATRIX"
#define WMTS_TILEROW		"TILEROW"
#define WMTS_TILECOL		"TILECOL"

#define WMTS_SERVICE_VALUE				"WMTS"

#define WMTS_REQUEST_VL_GETTILE			"GetTile"
#define WMTS_REQUEST_VL_CAPABILITIES	"Capabilities"

#define WMTS_VERSION_VL					"1.0.0"

// LRU最多的冷资源个数
#define LAYLRU_MAX_COLDBUNDLE_NUM		10 

#ifndef NULL
#define NULL 0
#endif// #ifndef NULL

// 描述某等级的bundle存在性 
struct TLevelBundleExistStatus
{
	// pbyteIndex 字节大小
	int nSize ;

	// 1 bit 表示一个bundle的存在性
	// 等级lv下有 2^(2-8)行2^(2-7)列bundle,表示按如下对bundle解释
	// 0  ......
	// 1 
	// 2 
	// .........
	unsigned char* pbyteIndex;

	~TLevelBundleExistStatus()
	{
		delete[] pbyteIndex;
		pbyteIndex = NULL;
	}
};

namespace thp
{
	struct TBundleID
	{
		// 考虑内存占用可以用1char
		unsigned int unLv;

		// 在等级内的编号
		// nLv < 8 nBundleID = 0; nLv> 8  nBundleID = [0, 2^(2*nLv-15))
		// 序号对应
		// nLv >= 8  有 2^(nLv-8) row, 有2^(nLv-7) 列
		// 如下表示编号对应的bundle的位置
		// 0   1*(2^(nLv-8))   ...
		// 1   .
		// .   .
		// .   .
		// .   .
		unsigned int unBundleIDinLv;

		bool operator==(const TBundleID& tNoNew) const;
		bool operator<(const TBundleID& tNoNew) const;

		TBundleID();
	};

	// bundle 编号
	struct TBundleIDex
	{
		TBundleID tID;

		// bundle 所在等级的行列号可以通过unBunldeID反算，为了减少计算量添加两个变量，实际bundle编号
		// 不用行列编号参与
		// bundle 所在等级的行号 
		int nBundleRow;

		// bundle 所在等级的列号 
		int nBundleCol;
	};


}


#endif // THP_WMTSPARAMDEF_H__
