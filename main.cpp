#include "WMTSRepository.h"
#include "NwGSoapServerThread.h"
#include "LoadConfigData.h"
#include "WMTSRepository.h"
#include <ctime>
#include <iostream>

// x64 只有这一个 
//#pragma comment(lib, "pthreadVC2.lib")
//#pragma comment(lib, "pthreadVCE2.lib")
//#pragma comment(lib, "pthreadVSE2.lib")

using namespace thp;

thp::WMTSRepository* g_pWMTSDataCache = new thp::WMTSRepository;

int main(int argc, char* argv[])
{
	// 通过命令行参数获取配置信息
	QString sConfig( ".\\server.ini" );

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

	// 预装载数据
	//QString qsPreLoadLayerName = LoadConfigData::getInstance()->getPreLoadLayerName();
	//std::string strPreLoadLayerName = (const char*)qsPreLoadLayerName.toLocal8Bit();

	//if ( !qsPreLoadLayerName.isEmpty() )
	//{
	//	int i = LoadConfigData::getInstance()->getBeginLevel();
	//	int nEndLv = LoadConfigData::getInstance()->getEndLevel();
	//	for(; i <= nEndLv; ++i)
	//	{
	//		if( 0 != g_pWMTSDataCache->loadData(strPreLoadLayerName, i) )
	//			break;
	//	}
	//}
	//

	NwGSoapServerThread td;
	td.initServerStartParam();
	td.startGSoapServer();

	clock_t t1 = clock();
	clock_t tSpend = (t1 - t0)/CLOCKS_PER_SEC;

	std::cout<< "THP WMTS 开启成功>>>" << std::endl
		<< "服务目录:" << strServerDir << std::endl
		<< "缓存上限:" << g_pWMTSDataCache->getCacheMbSize() << " MB" << std::endl
		<< "耗    时:" << tSpend << "s" << std::endl;

	std::cout << ">";
	
	td.start();

	while(1)
	{
		int i = 0;
		std::cin >> i;
		if(1 == i)
		{
			td.stopGSoapServer();
		//	Sleep(10000);
			break;
		}
	}
} 
