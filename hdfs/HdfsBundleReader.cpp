#include "../StdAfx.h"
#include "HdfsBundleReader.h"
#include "HdfsUrl.h"
#include "json/json.h"
#include "../ParamDef.h"

#include <QString>


using namespace thp;

thp::HdfsBundleReader::HdfsBundleReader()
{

}

thp::HdfsBundleReader::~HdfsBundleReader()
{

}

bool thp::HdfsBundleReader::open(const char* szFile)
{
	size_t nLen = strlen(szFile);
	if(nLen >= BUNDLE_MAX_PATH || nLen < 20)
	{
		sprintf(m_szLastErr, "file path is too long!the limits length is %d", BUNDLE_MAX_PATH);
		QString qsLog = QString("hdfs,bundle文件(%s)打开失败").arg( GB(szFile) );
		return false;
	}// bundle 文件名长度为 17 +'\0' 

	memcpy(m_szBundleFile, szFile, nLen);
	m_szBundleFile[nLen] = '\0';

	char szBundlxFile[BUNDLE_MAX_PATH];
	memcpy(szBundlxFile, m_szBundleFile, THP_MAX_PATH);
	szBundlxFile[nLen-1] = 'x';
	if( !_loadBundlx(szBundlxFile) )
		return false;

	// 得到Bundle文件名
	std::string sBundle(szFile);
	size_t nPos0 = sBundle.rfind('\\');
	if(nPos0 == std::string::npos)
		nPos0 = sBundle.rfind('/');

	size_t nPos1 = sBundle.rfind('.');
	if(nPos1 == std::string::npos)
		return false;

	std::string sBundleName = sBundle.substr(nPos0, (nPos1 - nPos0));

	// 计算起始行列号
	nPos0 = sBundleName.find('R');
	nPos1 = sBundleName.find('C');

	std::string sBeginRow = sBundleName.substr(nPos0+1, nPos1 - nPos0 -1);
	std::string sBeginCol = sBundleName.substr(nPos1+1, sBundleName.size() - nPos1);

	sscanf(sBeginRow.c_str(), "%x", &m_nBundleBeginRow);
	sscanf(sBeginCol.c_str(), "%x", &m_nBundleBeginCol);

	return true;
}

// ? 有网络传输的场景下 在打开bundle时一次打开索引应该是没有必要的-即可以有话这部分代码
bool thp::HdfsBundleReader::_loadBundlx(const char* szFile)
{
	CURL* curlHandle = curl_easy_init();

	_initCurl(curlHandle);

	QString qsUrl = QString("%0?op=OPEN").arg( GB(szFile) );
	std::string sUrl = (const char*)( qsUrl.toLocal8Bit() );
	
	QByteArray arBundlx;

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arBundlx);

	// 设置请求url
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );
	if(CURLE_OK != res)
		return false;

	res = curl_easy_perform(curlHandle);
	if(CURLE_OK != res)
		return false;

	if( CURLE_OK != res )
	{
		QString sLog = QString("%0,%1,%2").arg("webhdfs").arg(GB("请求失败")).arg(qsUrl);
		return false;
	}

	if( arBundlx.size() != (BUNDLX_CONTENT_SIZE + BUNDLX_DOMX2) )
	{
		// 索引文件内容出错
		return false;
	}

	int nPtr = 0;
	int nTemp = 0;
	// 文件前16个字节无用
	nPtr += BUNDLX_DOM;

	memcpy(m_szBundlxContents, arBundlx.data() + nPtr, BUNDLX_CONTENT_SIZE);
	nPtr += BUNDLX_CONTENT_SIZE;

	curl_easy_cleanup(curlHandle);
	return true;
}

bool thp::HdfsBundleReader::getTileFromFile(int nTileInBundleIndex, unsigned char*& pByteTile, unsigned int& nSize)
{
	unsigned int* pOffSet = (unsigned int*)(m_szBundlxContents + nTileInBundleIndex * BUNDLX_NODE_SIZE);

	CURL* curlHandle = curl_easy_init();

	// 初始化连接参数
	::_initCurl(curlHandle);

	// 初始化查询Tile大小的查询连接
	QString qsUrl = QString("%0?op=OPEN&offset=%1&length=%2").arg(m_szBundleFile).arg(*pOffSet).arg( BUNDLE_TILE_SIZE );
	std::string sUrl = (const char*)( qsUrl.toLocal8Bit() );

	QByteArray arTileSize;

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arTileSize);

	// 设置请求url
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );
	if(CURLE_OK != res)
		return false;

	res = curl_easy_perform(curlHandle);
	if(CURLE_OK != res)
		return false;

	// 必须用内存拷贝 用QByteArray 的转换函数得到的是0值
	memcpy(&nSize, arTileSize.data(), BUNDLE_TILE_SIZE);

	if(0 == nSize)
	{
		// 没有对应的Tile数据
		return false;
	}

	// 读取Tile
	qsUrl = QString("%0?op=OPEN&offset=%1&length=%2").arg( GB(m_szBundleFile) ).arg((*pOffSet) + BUNDLE_TILE_SIZE).arg( nSize );
	sUrl = (const char*)( qsUrl.toLocal8Bit() );

	QByteArray arTile;

	// 设置数据写入对象
	res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arTile);

	// 设置请求url
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );
	if(CURLE_OK != res)
		return false;

	// 请求
	res = curl_easy_perform(curlHandle);
	if(CURLE_OK != res)
		return false;

	// 读取内容
	pByteTile = new unsigned char[nSize];
	if(!pByteTile)
	{
		sprintf(m_szLastErr, "memory Error");
		delete[] pByteTile;
		return false;
	}

	memcpy(pByteTile, arTile.data(), nSize);

	curl_easy_cleanup(curlHandle);

	return true;
}

unsigned int thp::HdfsBundleReader::getMaxByte()
{
	unsigned int nSize = 0;

	CURL* curlHandle = curl_easy_init();
	
	// 初始化连接参数
	::_initCurl(curlHandle);

	QString qsUrl = QString("%0?op=LISTSTATUS").arg(GB(m_szBundleFile));
	std::string sUrl = (const char*)( qsUrl.toLocal8Bit() );

	QByteArray arBundleStatus;

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arBundleStatus);

	// 设置请求url
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );
	if(CURLE_OK != res)
		return false;

	res = curl_easy_perform(curlHandle);
	if(CURLE_OK != res)
		return false;

	curl_easy_cleanup(curlHandle);

	QString qsBundleStatus = arBundleStatus;
	std::string sJson = (const char*)( qsBundleStatus.toLocal8Bit() );

	// 解析json串 
	Json::Reader reader;  
	Json::Value root;  
	if ( reader.parse(sJson, root) )  // reader将Json字符串解析到root，root将包含Json里所有子元素  
	{  
		Json::Value vlFileStatuses = root["FileStatuses"];
		Json::Value vlFileStatus = vlFileStatuses["FileStatus"];
		Json::Value::UInt nFileCount = vlFileStatus.size();

		if( 1 != nFileCount)
		{
			// 文件结构错误
			return 0;
		}

		Json::Value vlTemp = vlFileStatus[ Json::Value::UInt(0) ];
		std::string sType = vlTemp["type"].asString();

		// 跳过不是文件的目录对象
		if( 0 != sType.compare("FILE") )
		{
			// 文件结构错误
			return 0;
		}
		
		// 得到文件大小
		Json::Value::UInt nSizeByte = vlTemp["length"].asUInt();
		nSize = (unsigned int)nSizeByte;
	}

	return nSize;
}

int HdfsBundleReader::readAll(char*& pBundle)
{
	// TODO: 本函数可优化 有一次多余的大内存分配
	// 获取Bundle文件大小
	unsigned int nSize = getMaxByte();

	pBundle = new char[nSize]; 
	if(!pBundle)
	{
		// log
		std::cerr << "内存不足" << std::endl;
		sprintf(m_szLastErr, "memory full");
		delete[] pBundle;
		pBundle = NULL;
		return 0;
	}

	CURL* curlHandle = curl_easy_init();

	// 初始化连接参数
	::_initCurl(curlHandle);

	QString qsUrl = QString("%0?op=OPEN").arg(m_szBundleFile);
	std::string sUrl = (const char*)( qsUrl.toLocal8Bit() );

	QByteArray arBundle;

	// 设置数据写入对象
	CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &arBundle);

	// 设置请求url
	res = curl_easy_setopt(curlHandle, CURLOPT_URL, sUrl.c_str() );
	if(CURLE_OK != res)
	{
		delete[] pBundle;
		pBundle = NULL;
		return 0;
	}

	res = curl_easy_perform(curlHandle);
	if(CURLE_OK != res)
	{
		delete[] pBundle;
		pBundle = NULL;
		return false;
	}

	memcpy(pBundle, arBundle.data(), nSize);

	return nSize;
}



