#ifndef _NWGSOAPSEVINTERFACE_H
#define _NWGSOAPSEVINTERFACE_H

#include <QThread>
#include <QWaitCondition>

#define BACKLOG (1000) // Max. request backlog 
struct soap;

namespace thp
{
	class WMTSRepository;
}

class  NwGSoapServerThread : public QThread
{
public:
	NwGSoapServerThread();
	virtual ~NwGSoapServerThread();

	//起动GSOAP服务
	void startGSoapServer();

	//停止服务
	void stopGSoapServer();

	///初始化服务启动参数设置，如端口绑定、发送 接收超时时间 回调函数重载 
	bool initServerStartParam();
protected:
	// QThread重载函数
	virtual void run();
	QString _GetLocalIPAddress();
private:
	volatile bool            m_bExit;              // 退出标志
	QWaitCondition           m_waitCondition;
	struct soap              *m_psoap; 
};

#endif