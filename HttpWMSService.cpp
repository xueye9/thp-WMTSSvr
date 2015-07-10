#include "StdAfx.h"
#include "HttpWMSService.h"
#include "ParamDef.h"
#include "WMTSRepository.h"
#include "Tile.h"
//#include "glog/logging.h"
//#include "glog/raw_logging.h"
#include "Bundle.h"
#include <CLogThreadMgr.h>

extern thp::WMTSRepository* g_pWMTSDataCache;

HttpWMSService::HttpWMSService(void)
{
	_initLogWriter();
}

bool HttpWMSService::_initLogWriter()
{
	// 获取写日志对象
	m_pLogWriter = CLogThreadMgr::instance()->getLogWriter("HttpWMSService.log");

	// 如果不存在，创建日志文件，加载日志配置
	if (m_pLogWriter == NULL)
	{
		CLogAppender * pLogAppender = new CLogAppender("HttpWMSService", "HttpWMSService.log", "", "General"); 

		// 获取写日志对象
		m_pLogWriter = CLogThreadMgr::instance()->createLogWriter(pLogAppender);
	}
	return true; 
}

HttpWMSService::~HttpWMSService(void)
{
}

int HttpWMSService::dwmGetMap(struct soap *soap, char* path)
{
	// /WMTS?service=WMTS&request=GetTile&version=1.0.0&
	// layer=img&style=default&format=tiles&TileMatrixSet=c&TileMatrix=3&TileRow=2&TileCol=2

#ifdef _DEBUG
	clock_t tStart = clock();
#endif

	try
	{
		QUrl urlSpe = QUrl::fromEncoded(path);  

		QString sTemp = urlSpe.queryItemValue(WMTS_SERVICE);
		if( 0 != sTemp.compare("wmts", Qt::CaseInsensitive) )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}

		sTemp = urlSpe.queryItemValue( WMTS_REQUEST );

		// 处理请求capatibility
		//if( 0 != sTemp.compare(WMTS_REQUEST_VL_CAPABILITIES, Qt::CaseInsensitive) )
		//{
		//	QString  strMessage ="请求格式缺少必填选项";
		//	return _sendExceptionMessage(soap, strMessage, ""); 
		//}

		if( 0 != sTemp.compare(WMTS_REQUEST_VL_GETTILE), Qt::CaseInsensitive)
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}

		sTemp = urlSpe.queryItemValue( WMTS_VERSION );
		if (0 != sTemp.compare(WMTS_VERSION_VL))
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}

		sTemp = urlSpe.queryItemValue(WMTS_LAYER);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}
		std::string stdstrLayr = sTemp.toLocal8Bit();

		// 未使用
		sTemp = urlSpe.queryItemValue(WMTS_LAYER_STYLE);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}

		// PNG
		sTemp = urlSpe.queryItemValue(WMTS_TILE_FORMAT);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}

		// 未使用,内部算法应该是只有google的切片方案
		sTemp = urlSpe.queryItemValue(WMTS_TILEMATRIXSET);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}

		// 等级
		sTemp = urlSpe.queryItemValue(WMTS_TILEMATRIX);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}
		int nlv = sTemp.toInt();

		// 行号
		sTemp = urlSpe.queryItemValue(WMTS_TILEROW);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			m_pLogWriter->errorLog(strMessage);
			return _sendExceptionMessage(soap, strMessage, ""); 
		}
		int nRow = sTemp.toInt();

		// 列号
		sTemp = urlSpe.queryItemValue(WMTS_TILECOL);
		if( sTemp.isEmpty() )
		{
			QString  strMessage = GB("请求格式缺少必填选项");
			return _sendExceptionMessage(soap, strMessage, ""); 
		}
		int nCol = sTemp.toInt();

		// 从服务程序获取tile
		int nDetail = 0;

		QByteArray qAr;
		int nSize = g_pWMTSDataCache->getTile( stdstrLayr, nlv, nRow, nCol, qAr, nDetail);

		if( nSize > 0)
		{
#ifdef _DEBUG
			clock_t tEnd = clock();
			int nSpan = 1000*(tEnd - tStart)/CLOCKS_PER_SEC;
			//RAW_DLOG(INFO, "Get Tile Spend Time:%d ms", nSpan);
			std::cout << "Get Tile Spend Time:%d ms" << nSpan;
#endif

#ifdef _DEBUG
			tStart = clock();
#endif
			int nRes = _sendData(soap, qAr, "");

#ifdef _DEBUG
			tEnd = clock();
			nSpan = 1000*(tEnd - tStart)/CLOCKS_PER_SEC;
			//DLOG(INFO) << "Send Tile Spend Time:" << nSpan << "ms";
			std::cout << "Send Tile Spend Time:%d ms" << nSpan;
#endif

			return nRes;
		}

		
		QString  strMessage = QString(GB("没有制定瓦片数据,layer:%0,lv:%1,row:%2,col:%3")).arg(stdstrLayr.c_str()).arg(nlv).arg(nRow).arg(nCol);
		m_pLogWriter->errorLog(strMessage);
		return _sendExceptionMessage(soap, strMessage, ""); 
	}
	catch(...)
	{
		m_pLogWriter->errorLog( GB("解析url出错") );

		QString  strMessage = GB("解析url出错");
		return _sendExceptionMessage(soap, strMessage, ""); 
	}
}

int HttpWMSService::_sendData(struct soap *soap,  QByteArray &baMapData, const QString &strformat)
{  
	soap->bRequestSucess = true;

	// 5.发送GisData
	soap->http_content = "image/png" ;
	if (soap_response(soap, SOAP_FILE)) /* OK HTTP response header */
	{ 
		soap_end_send(soap); 
		return soap->error;
	}

	QBuffer buffer(&baMapData);
	buffer.open(QIODevice::ReadOnly);
	QDataStream readMapData(&buffer);
	for (;;)
	{ 
		size_t r = readMapData.readRawData (soap->tmpbuf, sizeof(soap->tmpbuf));
		if (!r)
		{
			break;
		}
		if (soap_send_raw(soap, soap->tmpbuf, r))
		{ 
			soap_end_send(soap);
			buffer.close();
			return soap->error;
		}
	}
	buffer.close();
	return soap_end_send(soap);
}

int  HttpWMSService::_sendExceptionMessage(struct soap *soap, const QString  & strMessage, const QString &strCode, QString strVersion)
{
	soap->bRequestSucess = false;

	//创建标准的文件
	QDomDocument doc("MyML");
	//创建根
	QDomElement root = doc.createElement("ServiceExceptionReport");
	doc.appendChild(root);
	root.setAttribute("version", strVersion); 
	QDomElement tagServiceException = doc.createElement("ServiceException");
	tagServiceException.setAttribute("code", strCode); 
	QDomText t = doc.createTextNode(strMessage);
	tagServiceException.appendChild(t);
	root.appendChild(tagServiceException);

	QString strxml = doc.toString();
	strxml = strxml.remove("<!DOCTYPE MyML>\n");
	
	QString strformat = "text/xml";
	QByteArray byaException = strxml.toUtf8 ();
	return _sendData(soap, byaException , strformat);
}
