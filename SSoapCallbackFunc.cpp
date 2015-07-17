#include "StdAfx.h"
#include "SSoapCallbackFunc.h"
#include "HttpWMSService.h"
#include "stdsoap2.h"


int http_fpost_handler(struct soap*soap, const char*, const char*, int, const char*, const char*, size_t)
{
	return http_get_handler(soap);
}
 
int SendFileData(struct soap *soap, const char *strFilePath, const char *type)
{
	FILE *fd;
	size_t r;
	fd = fopen(strFilePath, "rb"); /* open file to copy */
	if (!fd)
	{
		return 404; /* return HTTP not found */
	}
	soap->http_content = type;
	if (soap_response(soap, SOAP_FILE)) /* OK HTTP response header */
	{ 
		soap_end_send(soap);
		fclose(fd);
		return soap->error;
	}
	for (;;)
	{ 
		r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
		if (!r)
		{
			break;
		}
		if (soap_send_raw(soap, soap->tmpbuf, r))
		{ 
			soap_end_send(soap);
			fclose(fd);
			return soap->error;
		}
	}
	fclose(fd);
	return soap_end_send(soap);
}

int http_get_handler(struct soap *soap)
{
	//WMS请求处理
	QString strHttpPath = soap->path;	
	if(strHttpPath.contains("WMTS?", Qt::CaseInsensitive))
	{
		HttpWMSService httpWMSService;
		return   httpWMSService.dwmGetMap(soap, soap->path);
	}

	return 404;
}

#ifndef WITH_NOHTTP
#ifndef PALM_1 
static const struct soap_code_map h_http_error_codes[] =
{ 
	{ 200, "OK" },
	{ 201, "Created" },
	{ 202, "Accepted" },
	{ 203, "Non-Authoritative Information" },
	{ 204, "No Content" },
	{ 205, "Reset Content" },
	{ 206, "Partial Content" },
	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Found" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 307, "Temporary Redirect" },
	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Time-out" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request-URI Too Large" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Requested range not satisfiable" },
	{ 417, "Expectation Failed" },
	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Time-out" },
	{ 505, "HTTP Version not supported" },
	{   0, NULL }
};
static const char soap_padding[4] = "\0\0\0";
#define SOAP_STR_PADDING (soap_padding)
#define SOAP_STR_EOS (soap_padding)
#define SOAP_NON_NULL (soap_padding)
static const char * http_error(struct soap *soap, int status)
{ 
	register const char *msg = SOAP_STR_EOS;
#ifndef WITH_LEAN
	msg = soap_code_str(h_http_error_codes, status);
	if (!msg)
		msg = SOAP_STR_EOS;
#endif
	return msg;
}

// http的header，和gsoap实现相同，但是添加了Access-Control-Allow-Origin:*属性。
int http_set_response(struct soap *soap, int status, size_t count)
{ 
	register int err;

#ifdef WMW_RPM_IO
	if (soap->rpmreqid)
		httpOutputEnable(soap->rpmreqid);
#endif
	if (strlen(soap->http_version) > 4)
		return soap->error = SOAP_EOM;
	if (!status || status == SOAP_HTML || status == SOAP_FILE)
	{ 
		const char *s;
		if (count || ((soap->omode & SOAP_IO) == SOAP_IO_CHUNK))
			s = "200 OK";
		else
			s = "202 ACCEPTED";
		DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Status = %s\n", s));
#ifdef WMW_RPM_IO
		if (soap->rpmreqid || soap_valid_socket(soap->master) || soap_valid_socket(soap->socket)) /* RPM behaves as if standalone */
#else
		if (soap_valid_socket(soap->master) || soap_valid_socket(soap->socket)) /* standalone application (socket) or CGI (stdin/out)? */
#endif
		{ 
			sprintf(soap->tmpbuf, "HTTP/%s %s", soap->http_version, s);
			if ((err = soap->fposthdr(soap, soap->tmpbuf, NULL)))
				return err;
		}
		else if ((err = soap->fposthdr(soap, "Status", s))) /* CGI header */
			return err;
	}
	else if (status >= 200 && status < 600)
	{ 
		sprintf(soap->tmpbuf, "HTTP/%s %d %s", soap->http_version, status, http_error(soap, status));
		if ((err = soap->fposthdr(soap, soap->tmpbuf, NULL)))
			return err;
#ifndef WITH_LEAN 
		if (status == 401)
		{ 
			sprintf(soap->tmpbuf, "Basic realm=\"%s\"", (soap->authrealm && strlen(soap->authrealm) < sizeof(soap->tmpbuf) - 14) ? soap->authrealm : "gSOAP Web Service");
			if ((err = soap->fposthdr(soap, "WWW-Authenticate", soap->tmpbuf)))
				return err;
		}
		else if ((status >= 301 && status <= 303) || status == 307)
		{ 
			if ((err = soap->fposthdr(soap, "Location", soap->endpoint)))
				return err;
		}
#endif
	}
	else
	{ 
		const char *s = *soap_faultcode(soap);
		if (status >= SOAP_GET_METHOD && status <= SOAP_HTTP_METHOD)
			s = "405 Method Not Allowed";
		else if (soap->version == 2 && (!s || !strcmp(s, "SOAP-ENV:Sender")))
			s = "400 Bad Request";
		else
			s = "500 Internal Server Error";
		DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Error %s (status=%d)\n", s, status));
#ifdef WMW_RPM_IO
		if (soap->rpmreqid || soap_valid_socket(soap->master) || soap_valid_socket(soap->socket)) /* RPM behaves as if standalone */
#else
		if (soap_valid_socket(soap->master) || soap_valid_socket(soap->socket)) /* standalone application */
#endif
		{ 
			sprintf(soap->tmpbuf, "HTTP/%s %s", soap->http_version, s);
			if ((err = soap->fposthdr(soap, soap->tmpbuf, NULL)))
				return err;
		}
		else if ((err = soap->fposthdr(soap, "Status", s)))	/* CGI */
			return err;
	} 
	if (err = soap->fposthdr(soap, "Access-Control-Allow-Origin", "*"))
		return err;
	if ((err = soap->fposthdr(soap, "Server", "WebGISServer"))
		|| (err = soap_puthttphdr(soap, status, count)))
		return err;
#ifdef WITH_COOKIES
	if (soap_putsetcookies(soap))
		return soap->error;
#endif
	return soap->fposthdr(soap, NULL, NULL);
}
#endif
#endif 
//
//TGetHandler::TGetHandler(void* p)
//{
//	
//}
//
//int TGetHandler::operator()(struct soap*)
//{
//	//WMS请求处理
//	QString strHttpPath = soap->path;	
//	if(strHttpPath.contains("SERVICE=WMTS&", Qt::CaseInsensitive))
//	{
//		if (strHttpPath.contains("SERVICE=WMTS", Qt::CaseInsensitive) && 
//			strHttpPath.contains("REQUEST=GETCAPABILITIES",Qt::CaseInsensitive))
//		{
//		}
//
//
//		HttpWMSService httpWMSService;
//		return   httpWMSService.dwmGetMap(soap, soap->path);
//	}
//
//	return 404;
//}
