#include "WMTSRepository.h"
#include "NwGSoapServerThread.h"
#include "LoadConfigData.h"
#include "WMTSRepository.h"
#include <ctime>
#include <iostream>
#include <algorithm>

// x64 只有这一个 
//#pragma comment(lib, "pthreadVC2.lib")
//#pragma comment(lib, "pthreadVCE2.lib")
//#pragma comment(lib, "pthreadVSE2.lib")

using namespace thp;

thp::WMTSRepository* g_pWMTSDataCache = new thp::WMTSRepository;

int main(int argc, char* argv[])
{
	// 通过命令行参数获取配置信息
	QString sConfig( ".\\data\\wmts_conf.ini" );

	LoadConfigData::getInstance()->initConfigData( sConfig );

	QString qsServerDir = LoadConfigData::getInstance()->getServerDir();
	std::string strServerDir = (const char*)qsServerDir.toLocal8Bit();
	int nOneLayerMaxMem = LoadConfigData::getInstance()->getOneLayerMaxCacheMB();
	int nMemStrategy = LoadConfigData::getInstance()->getMemStrategy();

	g_pWMTSDataCache->setCacheMbSize(nOneLayerMaxMem);
	g_pWMTSDataCache->setMemStrategy(nMemStrategy);

	clock_t t0 = clock();

	if( !g_pWMTSDataCache->setPath( strServerDir.c_str() ) )
	{
		//LOG(ERROR) << "服务开启失败:服务路径错误" << std::endl;
		std::cout << "服务开启失败:服务路径错误" << std::endl;
		clock_t t1 = clock();
		clock_t tSpend = (t1 - t0)/CLOCKS_PER_SEC;
		//LOG(ERROR) << "耗时:" << tSpend << "s" << std::endl;
		std::cout << "耗时:" << tSpend << "s" << std::endl;

		return 0;
	}

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
		<< "服务目录:" << strServerDir << std::endl
		<< "缓存上限:" << g_pWMTSDataCache->getCacheMbSize() << " MB" << std::endl
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
			
			break;
		}
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
