#include <stdio.h>
#include <stdlib.h>
#include "v_msg.h"

TMsg * TMsg::pInstance = NULL; // 初始化

TMsg::TMsg()
{

}

TInt sem_post(TSem *pSem)
{
	if (NULL == pSem)
	{
		return VOS_FALSE;
	}
	//pSem->post();
	ReleaseSemaphore(pSem, 1, NULL);
	return VOS_TRUE;
}

TInt sem_wait(TSem *pSem, TULong ulMilliseconds)
{
	if (NULL == pSem)
	{
		return VOS_FALSE;
	}
	//pSem->wait();
	if (0 != WaitForSingleObject(pSem, ulMilliseconds))   //信号量值-1
	{
		return VOS_FALSE;
	}
	return VOS_TRUE;
}

void TMsgInfo::init(int iSize)
{
	m_pMsgHead = new  TMsgNode[iSize];
	m_iMaxSize = iSize;

	m_pSem = CreateSemaphore(NULL          //信号量的安全特性
		, 0            //设置信号量的初始计数。可设置零到最大值之间的一个值
		, m_iMaxSize + 1     //设置信号量的最大计数
		, NULL         //指定信号量对象的名称
		); // 初始化 信号量
	if (NULL == m_pSem)
	{
		clear();
		return;
	}

	m_pSpinMutex = new TSpinMutex();
	if (NULL == m_pSpinMutex)
	{
		clear();
		return;
	}
}
void TMsgInfo::clear()
{
	if (NULL != m_pSpinMutex)
	{
		delete m_pSpinMutex;
	}
	CloseHandle(m_pSem);
	if (NULL != m_pMsgHead)
	{
		delete m_pMsgHead;
	}
}

TInt  TMsgInfo::Push_Msg(void *pMsgContent)
{
	{
		TMutex mutex(m_pSpinMutex);
		m_pMsgHead[m_iTailPos].pMsg = pMsgContent;
		m_iTailPos = (++m_iTailPos) % m_iMaxSize;
		++m_iMsgSize;
	}

	if (VOS_TRUE != sem_post(m_pSem))
	{
		return VOS_FALSE;
	}
	return VOS_TRUE;
}

void *TMsgInfo::Pop_Msg(TULong ulMilliseconds)
{
	void *pMsg = NULL;

	if (VOS_TRUE != sem_wait(m_pSem, ulMilliseconds))
	{
		return NULL;
	}

	{
		TMutex lock(m_pSpinMutex);
		--m_iMsgSize;
		pMsg = m_pMsgHead[m_iHeadPos].pMsg;
		m_iHeadPos = (++m_iHeadPos) % m_iMaxSize;
	}

	return pMsg;
}

TMsg *TMsg::GetInstance()
{
	if (NULL == pInstance)
	{
		pInstance = new TMsg();

		pInstance->InitMsgInfo();
	}
	return pInstance;
}
TInt TMsg::InitMsgInfo()
{
	const int iMsgSize = MAX_MSG_QUE_SIZE;
	m_pMsgQue = new TMsgInfo(iMsgSize);

	m_pFreeMsgHead = (TMsgBody *)malloc(sizeof(TMsgHead));
	m_pFreeHead    = (TMsgHead *)m_pFreeMsgHead;

	m_pFreeHead->pPrev = m_pFreeHead->pNext = m_pFreeMsgHead;
	m_pFreeHead->iReserve = m_pFreeHead->iFreeSize = 0; 

	TMsgBody *pstMsgNode = new TMsgBody[iMsgSize];
	m_pSpinMutex = new TSpinMutex();


	for (int i = 0; i < iMsgSize; ++i)
	{
		InsertFree(pstMsgNode + i);
	}

	return VOS_TRUE;
}

inline void TMsg::InsertFree(TMsgBody *pMsgNode)
{
	{
		TMutex mutex(m_pSpinMutex);
		pMsgNode->pNext = m_pFreeMsgHead;
		pMsgNode->pPrev = m_pFreeMsgHead->pPrev;
		m_pFreeMsgHead->pPrev->pNext = pMsgNode;
		m_pFreeMsgHead->pPrev        = pMsgNode;
		++(m_pFreeHead->iFreeSize);
	}
}

// 获取到的默认内存大小为4080
void *TMsg::ObtainMsg()
{
	return PopFreeMsg();
}

void  TMsg::FreeMsg(void *pvMsg)
{
	if (NULL == pvMsg)
	{
		return;
	}
	TMsgBody *pMsgNode = (TMsgBody *)((char *)pvMsg - sizeof(void *) * 2);
	InsertFree(pMsgNode);

	pvMsg = NULL;
	return;
}

void *TMsg::PopFreeMsg()
{
	TMsgBody *pMsgNode = NULL;

	if (IsEmpty()) 
	{
		return NULL;
	}

	{
		TMutex mutex(m_pSpinMutex);

		if (IsEmpty())  { return NULL; }

		pMsgNode = m_pFreeMsgHead->pNext;

		pMsgNode->pNext->pPrev = pMsgNode->pPrev;
		m_pFreeMsgHead->pNext = pMsgNode->pNext;

		--(m_pFreeHead->iFreeSize);
	}

	return pMsgNode->acBody;
}

inline TBool TMsg::IsEmpty()
{
	return  ((m_pFreeMsgHead->pNext == m_pFreeMsgHead) ? VOS_TRUE : VOS_FALSE);
}

TInt  TMsg::SendMsg(void *pvMsg, TInt iMsgLen)
{
	TMsgBody *pMsgNode = (TMsgBody *)((char *)pvMsg - sizeof(void *) * 2);
	
	return m_pMsgQue->Push_Msg(pMsgNode);
}

void *TMsg::RecvMsg(TULong ulMilliseconds)
{
	void *pvMsg = m_pMsgQue->Pop_Msg(ulMilliseconds);
	return ((pvMsg == NULL) ? pvMsg : (char *)(pvMsg)+sizeof(void *) * 2);
}