#include "WMTSMaintainThread.h"
#include "WMTSRepository.h"
#include "WMTSLayer.h"
#include <iostream>

using namespace thp;

extern WMTSRepository* g_pWMTSDataCache;

// 唤醒时间间隔 second
#define WM_WAKEUP_SPAN 7200 // 2h


WMTSMaintainThread::WMTSMaintainThread(QObject *parent /*= 0*/) :
	QThread(parent)
{
	m_bStop = false;
}


void WMTSMaintainThread::run()
{
	if(m_bStop)
		return ;

	for(;;)
	{
		if ( m_bStop )
			break;

		if(NULL == g_pWMTSDataCache)
			QThread::sleep(WM_WAKEUP_SPAN);

		// 每两小时视图整理一次 
		maintain();

		std::cout << "数据维护完成" << std::endl;
		QThread::sleep(WM_WAKEUP_SPAN);
	}
}

void WMTSMaintainThread::stop()
{
	m_bStop = true;
}

void WMTSMaintainThread::maintain()
{
	int nLayerCount = g_pWMTSDataCache->getLayerCount();
	for (int i = 0; i < nLayerCount; ++i)
	{
		WMTSLayer* pLyr = g_pWMTSDataCache->getLayer(i);
		pLyr->maintain();
	}
}
