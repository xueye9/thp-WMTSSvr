#include "StdAfx.h"
#include "ProcessClientQuestRunnable.h"
#include "stdsoap2.h "
#include "WMTSServiceH.h"
//#include "glog/logging.h"
//#include "glog/raw_logging.h"

ProcessClientQuest::ProcessClientQuest(struct soap *soap)
:m_psoap(soap)
{

}

ProcessClientQuest::~ProcessClientQuest()
{
	if ( m_psoap != NULL )
	{
		// @fixed: 无论是否成功都必须释放内存
		soap_destroy(m_psoap); // dealloc C++ data
		//soap_destroy(m_psoap);       // Clean up deserialized class instances
		soap_end(m_psoap);           // Clean up deserialized data (except class instances) and temporary data
		soap_free(m_psoap);          // Detach and deallocate context (soap_new())  
	}
}

void ProcessClientQuest::run()
{
	if (soap_valid_socket(m_psoap->socket)) 
	{
		clock_t tBegin = clock();

		bool bSuccess = false;
		if (soap_serve(m_psoap) == SOAP_OK)
		{
			bSuccess = true;
		} 	

		clock_t tEnd = clock();

		//RAW_DLOG(INFO, "Request accepts connection from IP:%d.%d.%d.%d", 
		//	(int)( (m_psoap->ip>>24)&0xFF ), 
		//	(int)( (m_psoap->ip>>16)&0xFF ),
		//	(int)( (m_psoap->ip>>8)&0xFF ),
		//	(int)(m_psoap->ip&0xFF)
		//	);

		//RAW_DLOG(INFO, "耗时:%d %s", 
		//	1000*(tEnd - tBegin)/CLOCKS_PER_SEC, 
		//    "ms"	
		//	);
	}
	else
	{
		// 线程安全的写日志
		//RAW_LOG(INFO, "Request accepts connection from IP:", 
		//	(int)( (m_psoap->ip>>24)&0xFF ), 
		//	(int)( (m_psoap->ip>>16)&0xFF ),
		//	(int)( (m_psoap->ip>>8)&0xFF ),
		//	(int)(m_psoap->ip&0xFF)
		//	);
		//RAW_LOG(INFO, "Request accepts connection from IP:", 
		//	(int)( (m_psoap->ip>>24)&0xFF ), 
		//	(int)( (m_psoap->ip>>16)&0xFF ),
		//	(int)( (m_psoap->ip>>8)&0xFF ),
		//	(int)(m_psoap->ip&0xFF)
		//	);
		;
	}
} 
