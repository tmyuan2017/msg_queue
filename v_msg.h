#ifndef _MSG_H_
#define _MSG_H_
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include <map>
#include <vector>
#include <atomic>
#include <winsock2.h>
#include <condition_variable>
using namespace std;

#define MAX_MSG_QUE_SIZE 1000
#define MAX_MSG_BUF_LEN  4080

typedef int           TInt;
typedef unsigned int  TUInt;
typedef long          TLong;
typedef unsigned long TULong;

class TSpinMutex {
	atomic_flag *pLockWrite;// = ATOMIC_FLAG_INIT;
public:
	TSpinMutex() { pLockWrite = new atomic_flag{ ATOMIC_FLAG_INIT }; };
	TSpinMutex(const TSpinMutex&) = delete;
	TSpinMutex& operator= (const TSpinMutex&) = delete;
	void lock()   { while ((pLockWrite->test_and_set())); } // ��ȡ������
	void unlock() { pLockWrite->clear(); }                  // �ͷ�������
};


//typedef std::mutex  TMutex_t;
typedef TSpinMutex  TMutex_t;

class TMutex
{
public:
	TMutex(TMutex_t *pMutex)
	{
		m_pMutex = pMutex;
		pMutex->lock();
	}

	~TMutex()
	{
		m_pMutex->unlock();
	}

private:
	TMutex_t *m_pMutex;
};


struct TMsgNode
{
	void     *pMsg;

	TMsgNode():pMsg(NULL){}
	~TMsgNode(){pMsg = NULL;}
};
typedef void  TSem;

//typedef std::condition_variable TSem;

class TMsgInfo
{
public:
	TMsgNode *m_pMsgHead;
	TSem     *m_pSem;
	TSpinMutex *m_pSpinMutex;
	TInt      m_iMsgSize;
	TUInt     m_iTailPos;
	TUInt     m_iHeadPos;
	TUInt     m_iMaxSize;

	TMsgInfo() :m_iMsgSize(0), m_iTailPos(0), m_iHeadPos(0){ init(100); }
	TMsgInfo(TUInt iSize) :m_iMsgSize(0), m_iTailPos(0), m_iHeadPos(0) { init(iSize); }

	~TMsgInfo() { clear(); }

	TInt  Push_Msg(void *pMsgContent);
	void *Pop_Msg(TULong ulMilliseconds);
	TBool IsEmpty(){ return (m_iTailPos == m_iHeadPos ? VOS_TRUE : VOS_FALSE); }
private:
	void init(int iSize);
	void clear();
};

struct TMsgBody
{
	TMsgBody *pPrev;
	TMsgBody *pNext;
	char acBody[MAX_MSG_BUF_LEN];
};
struct TMsgHead
{
	TMsgBody *pPrev;
	TMsgBody *pNext;
	TInt      iFreeSize;
	TInt      iReserve;
};
class TMsg
{
public:
	static TMsg  *GetInstance();
	TInt  SendMsg(void *pMsgContent, TInt iMsgLen);  // ������Ϣ����Ϣ����
	void *RecvMsg(TULong ulMilliseconds);            // ����Ϣ�����л�ȡ��Ϣ�������̵߳���
	void *ObtainMsg();          // ��ȡ��Ϣbuffer
	void  FreeMsg(void *pvMsg); // �ͷŻ�ȡ����Ϣbuffer
private:
	TInt         InitMsgInfo();
	inline void  InsertFree(TMsgBody *pMsgNode);
	void        *PopFreeMsg();
	inline TBool IsEmpty();

private:
	TMsg();
	static TMsg *pInstance;

	TMsgInfo *m_pMsgQue;

	TMsgBody   *m_pFreeMsgHead;
	TMsgHead   *m_pFreeHead;    // �� m_pFreeMsgHead ��ַ��ͬ
	TSpinMutex *m_pSpinMutex;   // ���� m_pFreeMsgHead
};

#endif

