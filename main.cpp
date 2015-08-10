#include "WMTSRepository.h"
#include "NwGSoapServerThread.h"
#include "WMTSConfig.h"
#include "WMTSRepository.h"
#include "WMTSFactory.h"
#include "curl/curl.h"
#include <ctime>
#include <iostream>
#include <algorithm>
#include "vld.h"

#pragma comment(lib,"vld.lib")

#pragma comment(lib, "libcurl.lib")

using namespace thp;

thp::WMTSRepository* g_pWMTSDataCache = NULL;

int main(int argc, char* argv[])
{
	// 通过命令行参数获取配置信息
	QString sConfig( ".\\data\\wmts_conf.ini" );
	WMTSConfig::Instance()->initConfigData( sConfig );
	QString qsServerDir = WMTSConfig::Instance()->getDataDir();
	std::string strServerDir = (const char*)qsServerDir.toLocal8Bit();
	std::string sFileSystem("");
	int nFst = WMTSConfig::Instance()->getFileSysType();

	WMTSFactory::Instance()->setFileSys( thp::FST(nFst));
	if(thp::FST::HDFS_SYS == thp::FST(nFst))
	{
		sFileSystem = "HDFS";
	}
	else if( thp::FST::WIN32_FILE_SYS == thp::FST(nFst) )
	{
		sFileSystem = "WINDOWS LOCAL FILE SYSTEM";
	}
	else
	{
		sFileSystem = "UNIX LOCAL FILE SYSTEM";
	}

	g_pWMTSDataCache = WMTSFactory::Instance()->createRepository();

	if(NULL == g_pWMTSDataCache)
		return 0;

	clock_t t0 = clock();

	if ( !g_pWMTSDataCache->init( 0x0001 ) )
	{
		//LOG(ERROR) << "服务开启失败:数据初始化失败" << std::endl;
		std::cout << "服务开启失败:数据初始化失败" << std::endl;
		clock_t t1 = clock();
		clock_t tSpend = (t1 - t0)/CLOCKS_PER_SEC;
		//LOG(ERROR) << "耗时:" << tSpend << "s" << std::endl;
		std::cout << "耗时:" << tSpend << "s" << std::endl;
		return 0;
	}

	NwGSoapServerThread td;
	td.initServerStartParam();
	td.startGSoapServer();

	// 记录服务启动时间
	time_t ttNow = time(NULL);
	tm* tNow = localtime(&ttNow);
	printf("服务启动时间: %d-%02d-%02d %02d:%02d:%02d\n",
		tNow->tm_year + 1900,
		tNow->tm_mon + 1,
		tNow->tm_mday,
		tNow->tm_hour,
		tNow->tm_min,
		tNow->tm_sec);

	clock_t t1 = clock();
	clock_t tSpend = (t1 - t0)/CLOCKS_PER_SEC;

	std::cout<< "THP WMTS 开启成功>>>" << std::endl
		<< "文件系统:" << sFileSystem << std::endl
		<< "数据目录:" << strServerDir << std::endl
		<< "缓存上限:" << WMTSConfig::Instance()->getOneLayerMaxCacheMB() << " MB" << std::endl
		<< "耗    时:" << tSpend << "s" << std::endl;

	std::cout << ">";
	
	td.start();

	std::string sInput("");
	while(1)
	{
		int i = 0;
		std::cin >> sInput;
		std::transform(sInput.begin(), sInput.end(), sInput.begin(), ::tolower);  

		if(0 == sInput.compare("stop") )
		{
			td.stopGSoapServer();

			delete g_pWMTSDataCache;
			g_pWMTSDataCache = NULL;

			Sleep(5000);
			
			break;
		}
		else if ( 0 == sInput.compare("memory") )
		{
			g_pWMTSDataCache->showStatus();
		}// 显示内存使用状态
		else if (0 == sInput.compare("release") )
		{
			delete g_pWMTSDataCache;
			g_pWMTSDataCache = NULL;
		}// 释放占用内存
	}

	// 记录服务关闭时间
	ttNow = time(NULL);
	tNow = localtime(&ttNow);
	printf("服务关闭时间: %d-%02d-%02d %02d:%02d:%02d\n",
		tNow->tm_year + 1900,
		tNow->tm_mon + 1,
		tNow->tm_mday,
		tNow->tm_hour,
		tNow->tm_min,
		tNow->tm_sec);
} 
