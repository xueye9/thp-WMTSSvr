#include <iostream>
#include <fstream>
#include <sstream>
#include <io.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include "bdiapi.h"
#include "win/WinBundleReader.h"
#include "WMTSConfig.h"
#include <QString>
#include "WMTSFactory.h"
#include "curl/curl.h"
#include "hdfs/HdfsUrl.h"
#include <json/json.h>
#include <map>
#include <QFile>
#include <QByteArray>
#include <QString>

using namespace std;
using namespace thp;

#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "lib_json.lib")


//生成bdi
int createBdi(std::string sOutDir);

// 生成bdi 返回穿件图层索引数
int createBdiOnWinSys(std::string& sLayersDir);

// 测试网络和hdfs是否开启了webhdfs
bool testWebhdfs(const std::string& sUrl);
// 返回创建图层索引数
int createBdiOnWebhdfs(std::string& sLayersUrl, const std::string& sLocalDir);
// 返回有数据的level数
int createBdiOnWebhdfsLayer(std::string& sLayerUrl, std::map<int,TLevelBundleExistStatus*>& pBlEstIdx);
// 返回Bundle个数-使用bundlx扫描，索引文件和数据文件必须同事存在否则会有问题
int createBdiOnWebhdfsLevel(std::string& sLevelUrl, int nLvl, unsigned char* pBundleExistIdx);

//////////////枚举指定Bundle的Tile
// 返回Tile总数
int enumBundleTiles(const std::string& sBundle, const std::string& sOutDir);

int main(int argc, char **argv)
{
	if( 1 == argc )
		std::cout << "详细使用信息请使用 -h 参数" << std::endl << std::endl;
	
	if(argc > 1)
	{
		if( 0 == strcmp(argv[1], "-h") )
		{
			std::cout << "使用: bdi.exe [OPTION] [FILE]..." << std::endl;
			std::cout << std::endl;
			std::cout << "参数说明:" << std::endl;
			std::cout << " -b (FILE) : " << "依据配置文件(./data/wmts_conf.ini)生成索引" << std::endl
					  << "    FILE : 输出索引文件位置(仅当bundle文件存储在hdfs上有效)" << std::endl << std::endl;

			std::cout << " -e (FILE) (FILE) : 枚举指定bundle文件中的瓦片到指定目录,只对本地文件系统有效" << std::endl;
			std::cout << "    (FILE) : " << "第一个参数,指定bundle文件位置" << std::endl;
			std::cout << "    (FILE) : " << "第二个参数,指定瓦片文件输出目录" << std::endl;

#ifdef _DEBUG
			getchar();
#endif

			return 0;
		}

		// 生成bdi文件
		if( 0 == strcmp(argv[1], "-b") )
		{
			std::string sOutDir(".");
			if(argc >= 3)
				sOutDir = argv[2];

			return createBdi(sOutDir);
		}

		// 枚举指定bundle的瓦片
		if( 0 == strcmp(argv[1], "-e") )
		{
			if( argc < 4)
			{
				std::cout << "缺少参数"  << std::endl;
				return 0;
			}
			std::string sBundle = argv[2];
			//std::string sBundle = "E:\\WMTS\\nwws\\L17\\Rd400C1a000.bundle";

			std::string sOutDir = argv[3];
			//std::string sOutDir = "./L05/png/";

			return enumBundleTiles(sBundle, sOutDir);
		}
	}

	return 0;
}

int createBdi(std::string sOutDir)
{
	// 通过命令行参数获取配置信息
	QString sConfig( ".\\data\\wmts_conf.ini" );
	WMTSConfig::Instance()->initConfigData( sConfig );

	thp::FST eFsType = (thp::FST)WMTSConfig::Instance()->getFileSysType();

	int nLayers = 0;
	switch(eFsType)
	{
	case WIN32_FILE_SYS:
		{
			//std::string sFilePath = "D:\\test\\ArcGisParseBoudle\\ParseAGBoundle\\Layers";
			std::cout << "文件系统: Windows file system" << std::endl;
			QString qsDataDir = WMTSConfig::Instance()->getDataDir();
			std::string sDataDir = (const char*)qsDataDir.toLocal8Bit();
			nLayers = createBdiOnWinSys(sDataDir);
		}
		break;

		// 由于webhdfs上传索引文件有目录权限问题所在在本地目录生成
	case HDFS_SYS:
		{
			std::cout << "文件系统: HDFS" << std::endl;
			QString qsHdfsServer = WMTSConfig::Instance()->getHdfsServer();
			int nHdfsPort = WMTSConfig::Instance()->getHdfsNameNodeWebPort();
			QString qsDataDir = WMTSConfig::Instance()->getDataDir();

			QString qsUrl = QString("http://%0:%1/webhdfs/v1%2").arg(qsHdfsServer).arg(nHdfsPort).arg(qsDataDir);

			std::string sUrl = (const char*)qsUrl.toLocal8Bit();

			std::cout << "索引输出目录: " << sOutDir << std::endl;

			nLayers = createBdiOnWebhdfs(sUrl, sOutDir);
		}

		break;

	case UNIX_FILE_SYS:
		std::cout << "文件系统: Unix file system" << std::endl 
			<< "不支持" << std::endl;
		break;

	default:
		{
			std::cout << "未知文件系统:请修改配置文件" << std::endl;
		}
		break;
	}

	return nLayers;
}

int createBdiOnWinSys(std::string& sLayersDir)
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

	int nCount = 0;
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
			if(-1 == searchWinSysLayerFolder(filePathSub, mapBdlIdx) )
			{
				std::cout<< "目录[" << filePathSub <<"]不是WMTS Server 目录"<< std::endl;
				continue;
			}

			// 写一个图层的索引文件
			std::string sBdlIdxName = filePathSub + ".bdi";
			if( write_bdi(mapBdlIdx, sBdlIdxName) )
			{
				std::cout << "成功生成图层[" << sBdlIdxName << "] 索引" << std::endl;
			}

			for (std::map<int, TLevelBundleExistStatus*>::iterator it = mapBdlIdx.begin(); it != mapBdlIdx.end(); ++it)
			{
				delete it->second;
			}

			++nCount;
		}
	}while(!(done=_findnext64i32(handle,&fileInfo)));

	_findclose(handle);	

	std::cout << "完成扫描"<< std::endl; 
	return nCount;
}

bool testWebhdfs(const std::string& sUrl)
{
	CURL* curlHandle = curl_easy_init();
	if(NULL == curlHandle)
		return false;

	std::string sTotalUrl = sUrl + "?op=LISTSTATUS";

	// 初始化连接参数
	::_initCurl(curlHandle);

	QByteArray arDirectoryStatus;

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arDirectoryStatus);
	if(CURLE_OK != res)
		return false;

	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sTotalUrl.c_str() );
	if(CURLE_OK != res)
		return false;

	res = curl_easy_perform(curlHandle);
	if(CURLE_OK != res)
		return false;

	curl_easy_cleanup(curlHandle);
	return true;
}

int createBdiOnWebhdfs(std::string& sLayersUrl, const std::string& sLocalDir)
{
	if( !testWebhdfs(sLayersUrl) )
	{
		std::cout << "连接hdfs失败，可能原因: 1 网络问题; 2 hdfs未开启webhdfs" << std::endl;
		return 0;
	}

	int nLayerCount = 0;
	CURL* curlHandle = curl_easy_init();

	QByteArray arDirectoryStatus;

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arDirectoryStatus);

	// 设置请求url
	std::string sUrl = sLayersUrl + "?op=LISTSTATUS";

	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );

	res = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	if( CURLE_OK != res )
	{
		std::cout << "url:" << sUrl << "请求失败" << std::endl;
		return nLayerCount;
	}

	QString qs = arDirectoryStatus;

	std::string sJson = (const char*)( qs.toLocal8Bit() );

	// 解析json串 
	Json::Reader reader;  
	Json::Value root;  
	if ( reader.parse(sJson, root) )  // reader将Json字符串解析到root，root将包含Json里所有子元素  
	{  
		Json::Value vlFileStatuses = root["FileStatuses"];
		Json::Value vlFileStatus = vlFileStatuses["FileStatus"];
		Json::Value::UInt nFileCount = vlFileStatus.size();
		for (Json::Value::UInt i = 0; i < nFileCount; ++i)
		{
			Json::Value vlTemp = vlFileStatus[i];
			std::string sType = vlTemp["type"].asString();

			// 跳过不是目录对象
			if( 0 != sType.compare("DIRECTORY") )
				continue;

			std::string sDirName = vlTemp["pathSuffix"].asString();

			std::string sLayerUrl = sLayersUrl + sDirName + "/";
		
			// lv - idx
			std::map<int, TLevelBundleExistStatus*> mapBdlIdx;
			if( createBdiOnWebhdfsLayer(sLayerUrl, mapBdlIdx) )
				++nLayerCount;

			// 写一个图层的索引文件
			std::string sBdlIdxName = sLocalDir + "/" + sDirName + ".bdi";

			if( write_bdi(mapBdlIdx, sBdlIdxName) )
			{
				// 上传到hdfs
				std::cout << "成功生成图层[" << sDirName << ".bdi] 索引" << std::endl;
			}

			for (std::map<int, TLevelBundleExistStatus*>::iterator it = mapBdlIdx.begin(); it != mapBdlIdx.end(); ++it)
			{
				delete it->second;
			}
		}
	}  

	return nLayerCount;
}

// 返回有数据的lv 个数
int createBdiOnWebhdfsLayer(std::string& sLayerUrl, std::map<int,TLevelBundleExistStatus*>& pBlEstIdx)
{
	CURL* curlHandle = curl_easy_init();

	QByteArray arDirectoryStatus;

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arDirectoryStatus);

	// 设置请求url
	std::string sUrl = sLayerUrl + "?op=LISTSTATUS";

	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );

	res = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	if( CURLE_OK != res )
	{
		std::cout << "url:" << sUrl << "请求失败" << std::endl;
		return 0;
	}

	QString qs = arDirectoryStatus;

	std::string sJson = (const char*)( qs.toLocal8Bit() );

	// 解析json串 
	Json::Reader reader;  
	Json::Value root;  
	if ( reader.parse(sJson, root) )  // reader将Json字符串解析到root，root将包含Json里所有子元素  
	{  
		Json::Value vlFileStatuses = root["FileStatuses"];
		Json::Value vlFileStatus = vlFileStatuses["FileStatus"];
		Json::Value::UInt nFileCount = vlFileStatus.size();
		for (Json::Value::UInt i = 0; i < nFileCount; ++i)
		{
			Json::Value vlTemp = vlFileStatus[i];
			std::string sType = vlTemp["type"].asString();

			// 跳过不是目录对象
			if( 0 != sType.compare("DIRECTORY") )
				continue;

			std::string sName = vlTemp["pathSuffix"].asString();

			if( 3 != sName.size() )
			{
				// log
				continue;
			}

			if( 'L' != sName[0] )
			{
				// log
				continue;
			}

			char szNum[4];
			memset(szNum, 0, 4);
			memcpy(szNum, sName.c_str()+1, 3 );
			szNum[3] = '\0';
			int nLv = 0;
			if( -1 == sscanf(szNum, "%d", &nLv) )
			{
				// 
				continue;
			}

			TLevelBundleExistStatus* pNode = new TLevelBundleExistStatus;
			pNode->nSize = calcBunldeExistStatusOccupyByte(nLv);
			pNode->pbyteIndex = new unsigned char[pNode->nSize];
			memset(pNode->pbyteIndex, 0, pNode->nSize);

			std::string sLevelUrl = sLayerUrl + sName + "/";
			if( createBdiOnWebhdfsLevel(sLevelUrl, nLv, pNode->pbyteIndex) )
			{
				pBlEstIdx.insert( std::make_pair(nLv, pNode) );
			}
		}
	}  

	return (int)pBlEstIdx.size();
}

int createBdiOnWebhdfsLevel(std::string& sLevelUrl, int nLvl, unsigned char* pBundleExistIdx)
{
	int nBundleCount = 0;
	CURL* curlHandle = curl_easy_init();

	QByteArray arDirectoryStatus;

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arDirectoryStatus);

	// 设置请求url
	std::string sUrl = sLevelUrl + "?op=LISTSTATUS";

	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );

	res = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	if( CURLE_OK != res )
	{
		std::cout << "url:" << sUrl << "请求失败" << std::endl;
		return nBundleCount;
	}

	// 解析json串 
	QString qs = arDirectoryStatus;
	std::string sJson = (const char*)( qs.toLocal8Bit() );

	Json::Reader reader;  
	Json::Value root;
	// reader将Json字符串解析到root，root将包含Json里所有子元素 
	if ( reader.parse(sJson, root) )   
	{  
		Json::Value vlFileStatuses = root["FileStatuses"];
		Json::Value vlFileStatus = vlFileStatuses["FileStatus"];
		Json::Value::UInt nFileCount = vlFileStatus.size();

		for (Json::Value::UInt i = 0; i < nFileCount; ++i)
		{
			Json::Value vlTemp = vlFileStatus[i];
			std::string sType = vlTemp["type"].asString();

			// 跳过不是文件对象
			if( 0 != sType.compare("FILE") )
				continue;

			std::string sName = vlTemp["pathSuffix"].asString();

			// 跳过不是索引文件的文件
			if ( std::string::npos == sName.find(FILE_POSTFIX) )
				continue;

			size_t pos0 = sName.find('R');
			size_t pos1 = sName.find('C');
			size_t pos2 = sName.find('.');

			std::string sNum = sName.substr(pos0+1, pos1 - pos0 - 1);
			unsigned int nRow = 0;
			if( -1 == sscanf(sNum.c_str(), "%x", &nRow) )
				continue;

			unsigned int nCol = 0;
			sNum = sName.substr(pos1+1, pos2 - pos1 -1);
			if( -1 == sscanf(sNum.c_str(), "%x", &nCol) )
				continue;

			// 计算bundle编号
			unsigned int nBundleIndex = calcBundleNo(nLvl, nRow, nCol);

			// 得到字节的偏移
			unsigned int nByteOffset = nBundleIndex >> 3;  // <==> nBundleIndex / 3
			unsigned char* pOf = pBundleExistIdx + nByteOffset;

			// 标记位置 0-7
			unsigned int nTagIdx = nBundleIndex - (nByteOffset << 3);
			tag(pOf, nTagIdx);
			++nBundleCount;
		}
	}

	return nBundleCount;
}

int enumBundleTiles(const std::string& sBundle, const std::string& sOutDir)
{
	int nTileCount = 0;
	thp::WinBundleReader reader;
	reader.open( sBundle.c_str() );

	int nRow = 0;
	int nCol = 0;
	unsigned char* pTile;
	int nTileSize = 0;
	thp::BundleReader::FetchType eType = thp::BundleReader::FetchType_Success;

	size_t nPos = sBundle.rfind('\\');
	if(nPos == std::string::npos)
		nPos = sBundle.rfind('/');

	size_t nPos2 = sBundle.rfind('.');
	std::string sBundleName = sBundle.substr(nPos, (nPos2 - nPos));

	std::stringstream ss;
	ss << sOutDir << sBundleName << ".txt";
	std::string sTxt = ss.str();
	std::ofstream of(sTxt.c_str());

	// 计算其实行列号
	//nPos = sBundleName.find('R');
	//nPos2 = sBundleName.find('C');
	//int nBeginRow = 0;
	//int nBeginCol = 0;

	//std::string sBeginRow = sBundleName.substr(nPos+1, nPos2 - nPos -1);
	//std::string sBeginCol = sBundleName.substr(nPos2+1, sBundleName.size() - nPos2);

	//sscanf(sBeginRow.c_str(), "%x", &nBeginRow);
	//sscanf(sBeginCol.c_str(), "%x", &nBeginCol);

	while(thp::BundleReader::FetchType_Success == eType) 
	{
		eType = reader.nextTile(nRow, nCol, pTile, nTileSize);
		if( thp::BundleReader::FetchType_Success != eType )
			break;

		QString sFile = QString("%0R%1C%2.png").arg( QString::fromLocal8Bit(sOutDir.c_str())).arg(nRow).arg(nCol);

		of << nRow << "," << nCol << std::endl;

		std::string s = sFile.toLocal8Bit();

		QFile file( sFile );
		file.open(QIODevice::WriteOnly);

		QByteArray bydata;
		bydata.append((const char*)pTile, nTileSize);
		file.write(bydata);
		file.close();
		++nTileCount;
	}

	std::cout << "输出目录:" << sOutDir << std::endl;
	std::cout << "一共枚举:" << nTileCount << " 瓦片" << std::endl;
	return nTileCount;
}
