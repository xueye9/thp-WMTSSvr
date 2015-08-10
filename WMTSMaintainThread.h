/*!
 * \file WMTSMaintainThread.h
 * \author Xuebingbing
 * \brief WMTS 维护线程 没隔两小时就查看每个图层中记录的bundle个数,当记录的bundle数大于等于1000万时
 删除当前不再内存中的记录
 *
 * TODO: long description
 * \date 八月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */

#ifndef THP_WMTSMAINTAINTHREAD_H__
#define THP_WMTSMAINTAINTHREAD_H__

#include <QThread>

class WMTSMaintainThread : public QThread
{
	//Q_OBJECT
public:
	explicit WMTSMaintainThread(QObject *parent = 0);

	void run();

	void stop();

	void maintain();

signals:

	public slots:

	bool m_bStop;
};

#endif // THP_WMTSMAINTAINTHREAD_H__
