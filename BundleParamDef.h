#ifndef BUNDLEPARAMDEF_H__
#define BUNDLEPARAMDEF_H__

#define BUNDLE_MAX_PATH		512

// bundlx 前后各有16个字节无用 怀疑存放了一些描述信息 = 80KB
#define BUNDLX_SIZE		81952 

// bundlx 前后各有16个字节无用 怀疑存放了一些描述信息
#define BUNDLX_DOM			16		
#define BUNDLX_DOMX2		32
#define BUNDLX_CONTENT_SIZE 81920
#define BUNDLX_NODE_SIZE	5

// 128 * 128 个
#define BUNDLE_EDGE				128
#define BUNDLE_MAX_TILE_NUM		16384

#endif // BundleParamDef_h__
