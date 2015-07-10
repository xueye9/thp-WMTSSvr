#ifndef _SSOAPCALLBACKFUNC_H_
#define _SSOAPCALLBACKFUNC_H_


//struct TGetHandler
//{
//	TGetHandler(void* p);
//
//	int operator()(struct soap*);
//};

int http_get_handler(struct soap*);	/* HTTP get handler */

int http_fpost_handler(struct soap*, const char*, const char*, int, const char*, const char*, size_t);

/// gsoap的回调函数，写这里的目的是为了实现js客户端的跨域访问
/// if (err = soap->fposthdr(soap, "Access-Control-Allow-Origin", "*")) :表示跨域
/// if ((err = soap->fposthdr(soap, "Server", "WebGISServer")): 指定发布服务的程序
int http_set_response(struct soap *soap, int status, size_t count);

#endif
