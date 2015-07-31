#include "StdAfx.h"
#include "NwGSoapServerThread.h"
#include "ProcessClientQuestRunnable.h" 
#include "SSoapCallbackFunc.h"
#include "WMTSServiceStub.h"
#include "WMTSConfig.h"
#include "WMTSRepository.h"
#include <QHostInfo>
#include <QNetworkInterface>

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
	mbindSocket = soap_bind(m_psoap, NULL, WMTSConfig::Instance()->getPort(), BACKLOG); 
	if (!soap_valid_socket(mbindSocket))
	{
		soap_print_fault(m_psoap, stderr);
		return false;
	}

	std::string sAddress = _GetLocalIPAddress().toLocal8Bit();

	std::cout << "Master socket:" << mbindSocket << std::endl;
	std::cout << "Server IP:" << sAddress << std::endl;
	std::cout << "Server Port:" << WMTSConfig::Instance()->getPort() << std::endl;

	// 其他的详细信息

	//设置最大线程数
	QThreadPool::globalInstance()->setMaxThreadCount(WMTSConfig::Instance()->getMaxThreadCount());
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

QString NwGSoapServerThread::_GetLocalIPAddress()
{
	QString vAddress;

#ifdef _WIN32

	QHostInfo vHostInfo = QHostInfo::fromName( QHostInfo::localHostName() );
	QList<QHostAddress> vAddressList = vHostInfo.addresses();

#else

	QList<QHostAddress> vAddressList = QNetworkInterface::allAddresses();

#endif

	for(int i = 0; i < vAddressList.size(); i++)

	{

		if(!vAddressList.at(i).isNull() &&

			vAddressList.at(i) != QHostAddress::LocalHost &&

			vAddressList.at(i).protocol() ==  QAbstractSocket::IPv4Protocol)

		{

			vAddress = vAddressList.at(i).toString();

			break;

		}

	}

	return vAddress;
}