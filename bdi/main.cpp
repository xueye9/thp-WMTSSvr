#include <iostream>
#include <fstream>
#include <sstream>
#include <io.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include "bdiapi.h"
#include "../BundleReader.h"

using namespace std;

// 生成bdi
int genBdi(std::string& sLayersDir);

// 枚举图层的图片
// sLayer 图层文件夹
// sOut 数据目录
int enumTiles(const std::string& sLayer, const std::string& sOut, bool bOutTile);
int enumLevelTile(const std::string& sLevelDir, const std::string& sOut, bool bOutTile);
bool isLevel(const char* szLvlFolder);
bool isBundleFile(const char* szBundle);

int main(int argc, char **argv)
{
	if(1 == argc)
	{
		//std::string sFilePath = "D:\\test\\ArcGisParseBoudle\\ParseAGBoundle\\Layers";
		std::cout << "在当前目录生成 bdi 文件" << std::endl;
		//genBdi( sFilePath );
		genBdi( std::string(".") );
		return 0;
	}

	std::string sCmd = argv[1];
	if( 0 == sCmd.compare("-h") && 2 == argc)
	{
		std::cout << "使用: bdi.exe [OPTION]... [FILE]..." << std::endl;
		std::cout << std::endl;
		std::cout << "参数说明:" << std::endl;
		std::cout << "  none         " << "在当前目录生成索引。" << std::endl;
		std::cout << "  -b (FILE)    " << "生成索引文件, (FILE)指定WMTS图层组目录。" << std::endl;
		std::cout << "  -e  [OPTION] (FILE) (FILE)" << std::endl;
		std::cout << "      /t   " << "生成瓦片信息同时导出瓦片文件(*.png)。" << std::endl;
		std::cout << "      /n   " << "生成瓦片信息但不导出瓦片文件。" << std::endl;
		std::cout << "           " << "第一个(FILE)指定图层目录。" << std::endl;
		std::cout << "           " << "第二个(FILE)指定信息导出目录。" << std::endl;
		return 0;
	}//打印帮助信息

	if( 0 == sCmd.compare("-e") && 5 == argc )
	{
		std::string sOutPng = argv[2];
		std::string sLayerPath = argv[3];
		std::string sOutPath = argv[4];
		if( 0 == sOutPng.compare("/t") )
		{
			std::cout << "枚举文件夹[" << sLayerPath << "]下瓦片到[" << sOutPath << "]" << std::endl;
			enumTiles(sLayerPath, sOutPath, true);
		}
		else if( 0 == sOutPng.compare("/n") )
		{
			std::cout << "不是有效的命令" << std::endl;
			enumTiles(sLayerPath, sOutPath, true);
		}

		std::cout << "不是有效的命令" << endl;
		return 0;
	}// 枚举图片

	if( 0 == sCmd.compare("-b") && argc == 3)
	{
		// 生成 *.bdi
		std::string sFilePath = argv[2];
		genBdi( sFilePath );
		return 0;
	}

	std::cout << "不是有效的命令" << endl;
	return 0;
}

int genBdi(std::string& sLayersDir)
{
	// 保存文件信息的结构体
	struct _finddata64i32_t fileInfo;

	// 句柄
	long handle;

	// 查找nextfile是否成功
	int done;

	// 要搜索的文件夹
	std::cout << "正在扫描目录:[" << sLayersDir << "]" << std::endl;

	std::string sFileFullPath = sLayersDir + "\\*.*";

	handle = _findfirst64i32(sFileFullPath.c_str(), &fileInfo);

	if(-1 == handle)
	{
		std::cout<< "目录[" << sLayersDir <<"]不是WMTS Server 目录"<< std::endl;
		return 0;
	}

	do
	{
		if( (strcmp(fileInfo.name, ".")==0) || (strcmp(fileInfo.name,"..")==0) )
			continue;

		if( (fileInfo.attrib&_A_SUBDIR) != _A_SUBDIR )
		{
			//std::string fileNameTure = sLayersDir+"\\"+fileInfo.name;
			//std::cout << "文件["<< fileNameTure << "] 不是有效的 WMTS Server 文件" << std::endl;
			continue;
		}// 跳过文件

		// 搜索子目录
		{
			std::string filePathSub=sLayersDir+"\\"+fileInfo.name;

			// lv - idx
			std::map<int, TLevelBundleExistStatus*> mapBdlIdx;
			if(-1 == searchLayerFolder(filePathSub, mapBdlIdx) )
			{
				std::cout<< "目录[" << sLayersDir <<"]不是WMTS Server 目录"<< std::endl;
				continue;
			}

			// 写一个图层的索引文件
			std::string sBdlIdxName = filePathSub + ".bdi";
			if( writeLayerBdlExistIdx(mapBdlIdx, sBdlIdxName) )
			{
				std::cout << "成功生成图层[" << sBdlIdxName << "] 索引" << std::endl;
			}

			for (std::map<int, TLevelBundleExistStatus*>::iterator it = mapBdlIdx.begin(); it != mapBdlIdx.end(); ++it)
			{
				delete it->second;
			}
		}
	}while(!(done=_findnext64i32(handle,&fileInfo)));

	_findclose(handle);	

	std::cout << "完成扫描"<< std::endl; 
}

int enumTiles(const std::string& sLayer, const std::string& sOut, bool bOutTile)
{
	// 保存文件信息的结构体
	struct _finddata64i32_t fileInfo;

	// 句柄
	long handle;

	// 查找nextfile是否成功
	int done;

	// 要搜索的文件夹
	std::cout << "正在扫描目录:[" << sLayer << "]" << std::endl;

	std::string sFileFullPath = sLayer + "\\*.*";

	handle = _findfirst64i32(sFileFullPath.c_str(), &fileInfo);

	if(-1 == handle)
	{
		std::cout<< "目录[" << sLayer <<"]不是WMTS Server 目录"<< std::endl;
		return 0;
	}

	do
	{
		if( (strcmp(fileInfo.name, ".")==0) || (strcmp(fileInfo.name,"..")==0) )
			continue;

		if( (fileInfo.attrib&_A_SUBDIR) != _A_SUBDIR )
		{
			std::string fileNameTure = sLayer+"\\"+fileInfo.name;
			std::cout << "文件["<< fileNameTure << "] 不是有效的 WMTS Server 文件" << std::endl;
			continue;
		}// 跳过文件

		// 搜索子目录
		{
			// 等级
			if( !isLevel(fileInfo.name) )
				continue;
			
			std::string filePathSub = sLayer+"\\"+fileInfo.name;
			std::string sLevlOutDir = sOut + "\\" + fileInfo.name;
			std::string sMkDir = "mkdir " + sOut + "\\" + fileInfo.name;
			system( sMkDir.c_str() );

			enumLevelTile(filePathSub, sLevlOutDir, bOutTile);
		}
	}while(!(done=_findnext64i32(handle,&fileInfo)));

	_findclose(handle);	

	std::cout << "完成枚举瓦片" << std::endl; 
}

bool isLevel(const char* sLvlFolder)
{
	int nLen = strlen(sLvlFolder);
	if(nLen != 3)
		return false;

	if ( 'L' != sLvlFolder[0] )
		return false;

	int nLevel = 0;
	sscanf( (sLvlFolder+1), "%d", &nLevel);
	if(0 > nLevel || nLevel > 21)
		return false;

	return true;
}

int enumLevelTile(const std::string& sLevelDir, const std::string& sOut, bool bOutTile)
{
	// 保存文件信息的结构体
	struct _finddata64i32_t fileInfo;

	// 句柄
	long handle;

	// 查找nextfile是否成功
	int done;

	std::string sFileFullPath = sLevelDir + "\\*.*";

	// 等级含有的tile信息集合写入文件
	std::string sLevelInfo = sOut + ".txt";
	std::ofstream osInfo( sLevelInfo.c_str() );

	handle = _findfirst64i32(sFileFullPath.c_str(), &fileInfo);

	if(-1 == handle)
	{
		return 0;
	}

	std::string sLayerName = "";
	int nLvl = 0;

	size_t nPos = sLevelDir.rfind('\\');
	std::string sLvl = sLevelDir.substr(nPos+2);
	sscanf(sLvl.c_str(), "%d", &nLvl);

	size_t nPos1 = sLevelDir.rfind('\\', nPos - 1);
	sLayerName = sLevelDir.substr(nPos1 + 1, nPos - nPos1 - 1 );

	do
	{
		if( (strcmp(fileInfo.name, ".")==0) || (strcmp(fileInfo.name,"..")==0) )
			continue;

		if( (fileInfo.attrib&_A_SUBDIR) == _A_SUBDIR )
		{
			//std::string fileNameTure = sLevelDir + "\\"+fileInfo.name;
			//std::cout << "文件["<< fileNameTure << "] 不是有效的 WMTS Server 文件" << std::endl;
			continue;
		}// 跳过文件

		// 搜索 *.bundle 文件
		{
			// 等级
			if( !isBundleFile(fileInfo.name) )
				continue;

			std::string sBundlePath= sLevelDir + "\\" + fileInfo.name;

			thp::BundleReader reader;
			reader.open( sBundlePath.c_str() );

			int nRow = 0;
			int nCol = 0;
			unsigned char* pTile;
			int nTileSize = 0;
			thp::BundleReader::FetchType eType = thp::BundleReader::FetchType_Success;
			while(thp::BundleReader::FetchType_Success == eType) 
			{
				eType = reader.nextTile(nRow, nCol, pTile, nTileSize);
				if( thp::BundleReader::FetchType_Success != eType )
					break;

				osInfo << sLayerName << "," << nLvl << "," << nRow << "," << nCol << std::endl;

				if( bOutTile )
				{
					std::stringstream ss;
					ss << sOut << "\\R" << nRow << "C" << nCol << ".png";
					std::string sTile = ss.str();
					FILE* fpTile = fopen( sTile.c_str(), "wb");
					if(!fpTile)
						continue;

					fwrite(pTile, 1, nTileSize, fpTile);

					fclose(fpTile);
				}// 输出图片
			
				free(pTile);
				pTile = NULL;
				nTileSize = 0;
			}
		}
	}while(!(done=_findnext64i32(handle,&fileInfo)));

	_findclose(handle);	

	std::cout << "完成等级 " << nLvl << " 瓦片枚举" << std::endl; 
}

bool isBundleFile(const char* szBundle)
{
	int nLen = strlen(szBundle);
	if(nLen != 17)
		return false;

	const char* c = szBundle+11;
	int nLevel = 0;
	if( 0 != strcmp(szBundle+11, "bundle") )
		return false;

	return true;
}

