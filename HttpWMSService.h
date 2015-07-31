/*
 *  Http请求服务
 *
 *  包含OGC 标准规范中的WMS、WFS服务
    参考网址：
	带TOKEN请求，放在最后面
 *
 * 
 */

#ifndef __HTTPWMSSERVICE_H
#define __HTTPWMSSERVICE_H
#include "stdsoap2.h"
#include <QMutex>

class TokenSessionData;
class CLogWriter;

class HttpWMSService
{
public:
	HttpWMSService(void);
	~HttpWMSService(void);

	// 处理WMS的GetMap操作
	 int dwmGetMap(struct soap * soap, char * path);
	
private:
	int _dwmGetMapWindows(struct soap * soap, char * path);
	int _dwmGetMapUnix(struct soap * soap, char * path);

	///发送图片数据
	/// @param baMapData	[in]	图片数据
	/// @param strformat	[in]	图片格式
	 int _sendData(struct soap *soap, QByteArray &baMapData, const char* szformat);

	 int _sendCapabilities(struct soap *soap);

	///打包异常消息
	/// @param strMessage	[in]	错误消息
	/// @param strCode	    [in]	错误编码
	/// @param strVersion	[in]	服务版本
	 int _sendExceptionMessage(struct soap *soap, const QString  & strMessage, const QString &strCode, QString strVersion ="1.0.0");
	 bool _initLogWriter();

private:
	//QMutex m_mx;
	CLogWriter* m_pLogWriter;
};


#endif 
