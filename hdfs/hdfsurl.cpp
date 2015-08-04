#include <QtCore>
#include <QtGui>
#include <QtXml>
#include "hdfsurl.h"
#include <QByteArray>

size_t writeData(void *pIn, size_t nSize, size_t nItems, void *pOut)
{
	QByteArray* pAr = (QByteArray*)pOut;
	pAr->append( (const char*)pIn, nSize * nItems);

	return nSize * nItems;//	一共写了多少字节
}

bool _initCurl(CURL* handle)
{
	// 设置获取详细的联系信息 当前为打开状态可以关闭 掉 
	CURLcode res;
	//res = curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
	//if(res != CURLE_OK)
	//	return false;

	// 设置没有进度显示
	res = curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
	if(res != CURLE_OK)
		return false;

	// 设置超时 默认超时
	//curl_easy_setopt(handle, CURLOPT_TIMEOUT, 2); 

	// 设置写数据回调函数
	res = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeData);
	if(res != CURLE_OK)
		return false;

	// 开启自动定向
	res = curl_easy_setopt(handle, CURLOPT_AUTOREFERER, true);
	if(res != CURLE_OK)
		return false;
	//返回的头部中有Location(一般直接请求的url没找到)，则继续请求Location对应的数据 
	res = curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	if(res != CURLE_OK)
		return false;
	//查找次数，防止查找太深
	res = curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 1);		
	if(res != CURLE_OK)
		return false;
	//连接超时，这个数值如果设置太短可能导致数据请求不到就断开
	res = curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 3 );	
	if(res != CURLE_OK)
		return false;

	return true;
}
