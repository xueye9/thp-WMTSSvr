#ifndef GENAPI_H__
#define GENAPI_H__
#include <map>
#include <string>
#include "../ParamDef.h"


// 索引文件固定大小
// 索引文件格式
// 16 byte bom 未使用 
// 索引 0 - 21 工22 级

// 0     idxcontents
// 1	 idxcontents
// 2  
// ..

// idx contents = calcBunldeExistBit, lv<12= 1byte,lv>=12 = 2^(2*lv-23)byte
// [int] 等级, [...] 存在状态索引
//
// 
#define FILE_POSTFIX ".bundlx"
//#define FILE_NAME_LEN  17
#define MAX_LEVLE	22

// 等级 tile行号 tile列号
int calcBundleNo(int nLvl, int nRow, int nCol);


// 标记的单个字节,标记位置 0 - 7
void tag(unsigned char* pOf, int nTagIdx);

// 盘是否有标记
bool taged(unsigned char* pOf, int nTagIdx);

// 计算等级索引占用的大小
int calcBunldeExistStatusOccupyByte(int nLvl);

//IN:文件所在的路径,如：f:\example
//IN:存储的文件名
int searchLayerFolder (std::string filePath, std::map<int,TLevelBundleExistStatus*>& pBlEstIdx);

//IN:文件所在的路径,如：f:\example\L03
//层次，只有层次为0时，才完成链表中文件信息的显示和存储
//IN:存储的文件名
int searchLevelFolder (std::string sLayerLevelPath, int nLvl, unsigned char* pBundleExistIdx);

// 16 byte bom,int[等级编号],int[索引大小],索引内容
// 1 存在,0 不存在
bool writeLayerBdlExistIdx(const std::map<int, TLevelBundleExistStatus*>& idxMap, const std::string& sPath);

bool readLayerDbi(const std::string& sPath, std::map<int, TLevelBundleExistStatus*>& idxMap);

bool isExist(unsigned char* pByteDbi, unsigned int unBundleIndex);

#endif // genapi_h__
