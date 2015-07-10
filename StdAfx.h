#ifndef __STDHDR_H__
#define __STDHDR_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// 包含QtCore前先降低警告级别，避免出现很多的Qt内部警告
#ifdef _MSC_VER
#pragma warning(push,1)
#endif

// 包含所有QT的头文件
// 需要设置路径$(QTDIR)\include,$(QTDIR)\mkspecs\$(QMAKESPEC)
//#include <qt.h>
#include <QtCore>
#include <QtGui>
#include <QtXml>


#ifdef Q_OS_WIN32
#include <qt_windows.h>
#endif
#pragma warning(disable:4706)
#pragma warning(disable:4100)
#pragma warning(disable:4127)
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifndef GB
#define GB(text)	(QString::fromLocal8Bit(text))
#endif
#include "WMTSServiceBinding.nsmap"
#endif	//  __STDHDR_H__