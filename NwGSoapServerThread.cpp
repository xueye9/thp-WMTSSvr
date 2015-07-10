#include "StdAfx.h"
#include "NwGSoapServerThread.h"
#include "ProcessClientQuestRunnable.h" 
#include "SSoapCallbackFunc.h"
#include "WMTSServiceStub.h"
#include "LoadConfigData.h"
#include "WMTSRepository.h"
//#include "glog/logging.h"
//#include "glog/raw_logging.h"

int  __gis__TestData(struct soap*, struct __gis__TestDataResponse &_param_1)
{
	return SOAP_OK;
}

NwGSoapServerThread::NwGSoapServerThread()
{
	m_psoap = soap_new();
	soap_init(m_psoap);

	m_bExit = false;
}

NwGSoapServerThread::~NwGSoapServerThread()
{
	if(m_psoap != NULL)
	{
		soap_destroy(m_psoap);

		soap_end(m_psoap);

		soap_free(m_psoap);

		soap_done(m_psoap); 
	}

	m_psoap = NULL;
}
//启动GSOAP服务
void NwGSoapServerThread::startGSoapServer()
{
	if (isRunning())
	{
		m_waitCondition.wakeAll();
	}
	else
	{
		m_bExit = false;
		start();
	}
}
//停止服务
void NwGSoapServerThread::stopGSoapServer()
{
	m_bExit = true;
}

bool NwGSoapServerThread::initServerStartParam()
{
	soap_set_mode(m_psoap, SOAP_C_UTFSTRING|SOAP_XML_STRICT|SOAP_XML_CANONICAL);
	m_psoap->fget = http_get_handler;
	m_psoap->fpost = http_fpost_handler;
	m_psoap->fresponse = http_set_response;
	m_psoap->accept_timeout =10;
	m_psoap->send_timeout =10;
	m_psoap->recv_timeout = 10; 
	SOAP_SOCKET mbindSocket; 
	mbindSocket = soap_bind(m_psoap, NULL, LoadConfigData::getInstance()->getPort(), BACKLOG); 
	if (!soap_valid_socket(mbindSocket))
	{
		soap_print_fault(m_psoap, stderr);
		return false;
	}

	std::cout << "Master socket:" << mbindSocket << std::endl;
	std::cout << "Request accepts connection from IP:" 
		<< (int)( (m_psoap->ip>>24)&0xFF ) << "."
		<< (int)( (m_psoap->ip>>16)&0xFF ) << "."
		<< (int)( (m_psoap->ip>>8)&0xFF ) <<"."
		<< (int)(m_psoap->ip&0xFF);
	//LOG(INFO) << "Port:" << m_psoap->port << std::endl;  // ?绑定后端口号没变化
	std::cout << "bind to port:" << LoadConfigData::getInstance()->getPort() << std::endl;

	// 其他的详细信息

	//设置最大线程数
	QThreadPool::globalInstance()->setMaxThreadCount(LoadConfigData::getInstance()->getMaxThreadCount());
	return true;
}

void NwGSoapServerThread::run()
{
	SOAP_SOCKET sClientSocket; 
	struct soap *tsoap;
	//进入消息循环
	while (!m_bExit)
	{ 
		sClientSocket = soap_accept(m_psoap); 
		if (!soap_valid_socket(sClientSocket)) 
		{ 
			if (m_psoap->errnum) 
			{ 
				continue; // retry 
			}  
			soap_destroy(m_psoap); // dealloc C++ data
			soap_end(m_psoap);    // dealloc data and clean up

			Sleep(1000);
			continue;
		}

		// 线程安全的写日志
		//RAW_DLOG(INFO, "Request accepts connection from IP:%d.%d.%d.%d", 
		//	(int)( (m_psoap->ip>>24)&0xFF ), 
		//	(int)( (m_psoap->ip>>16)&0xFF ),
		//	(int)( (m_psoap->ip>>8)&0xFF ),
		//	(int)(m_psoap->ip&0xFF)
		//	);

		tsoap = soap_copy(m_psoap); // make a safe copy
		if (!tsoap)
		{
			continue;
		}

		ProcessClientQuest *pProcessClientQuest = new ProcessClientQuest(tsoap);
		QThreadPool::globalInstance()->start(pProcessClientQuest);
	}

	QThreadPool::globalInstance()->waitForDone ();
	QThreadPool::globalInstance()->releaseThread();
}
