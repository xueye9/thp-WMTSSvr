#ifndef _PROCESSCLIENTQUESTRUNNABLE_H
#define _PROCESSCLIENTQUESTRUNNABLE_H

class simple_mutex;

class  ProcessClientQuest : public QRunnable
{
public:
	ProcessClientQuest(struct soap * soap);
	~ProcessClientQuest();

protected:
	virtual void run(); 

private:
	struct soap * m_psoap; 

};

#endif
