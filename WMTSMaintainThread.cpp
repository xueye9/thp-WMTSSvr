#include "WMTSMaintainThread.h"
#include "WMTSRepository.h"
#include "WMTSLayer.h"
#include <iostream>
#include <ctime>

using namespace thp;

extern WMTSRepository* g_pWMTSDataCache;

// 唤醒时间间隔 second
#define WM_WAKEUP_SPAN 7200 // 2h

// 强制进行资源整理的时间端 1天中的 [0-24),本地时间凌晨1点到凌晨4点进行一次强制的资源整理
#define FORCE_MAINTAIN_BEGIN 1
#define FORCE_MAINTAIN_END	 4


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

		time_t ttNow = time(NULL);
		tm* tNow = localtime(&ttNow);

		int nH = tNow->tm_hour;
		
		// 凌晨1点到凌晨4点内进行一次强制的数据整理
		if(nH > FORCE_MAINTAIN_BEGIN && nH < FORCE_MAINTAIN_END)
			maintain(true);
		else
			maintain(false);

		QThread::sleep(WM_WAKEUP_SPAN);
	}
}

void WMTSMaintainThread::stop()
{
	m_bStop = true;
}

void WMTSMaintainThread::maintain(bool bForce)
{
	int nLayerCount = g_pWMTSDataCache->getLayerCount();
	for (int i = 0; i < nLayerCount; ++i)
	{
		WMTSLayer* pLyr = g_pWMTSDataCache->getLayer(i);
		pLyr->maintain(bForce);
	}
}
