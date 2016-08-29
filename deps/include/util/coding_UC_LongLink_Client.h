// UC_LongLink_Client.h

#ifndef _UC_LONG_LINK_CLIENT_H_
#define _UC_LONG_LINK_CLIENT_H_

#include "UC_Communication.h"
#include "UT_Queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class UC_LongLink_Client
{
private:
	typedef struct _ServerInfo
	{
		long m_lTimeOut;
		unsigned short m_usConnectNum;
		unsigned __int64 m_ui64_HeadValidateCode;
		unsigned __int64 m_ui64_TailValidateCode;
		
		char m_szaIP[16];
		unsigned short m_usPort;
		
		UT_Queue<unsigned short> m_cConnected;
		UT_Queue<unsigned short> m_cUnconnected;
		
		typedef struct _ConnectInfo
		{
			SOCKET sConnect;
			long lRetCode;
			_ConnectInfo()
			{
				sConnect = -1;
				lRetCode = 0;
			}
		} CONNECTINFO;

		CONNECTINFO* m_cpConnect;
		
		_ServerInfo()
		{
			m_lTimeOut = 0;
			m_usConnectNum = 0;
			m_ui64_HeadValidateCode = 0xBBBBBBBBBBBBBBBB;
			m_ui64_TailValidateCode = 0xEEEEEEEEEEEEEEEE;
			
			*m_szaIP = 0;
			m_usPort = -1;
			
			m_cpConnect = NULL;
		};
		
		~_ServerInfo()
		{
			if(m_cpConnect)
				delete m_cpConnect;
		};
		
		static long thread_Connect(void* vpArg)
		{
			_ServerInfo* cpClass = (_ServerInfo*)vpArg;
			for(;;)
			{
				unsigned short usConnectPos = cpClass->m_cUnconnected.PopData();
				UC::CloseSocket(cpClass->m_cpConnect[usConnectPos].sConnect);

				cpClass->m_cpConnect[usConnectPos].sConnect = UC::ConnectSocket(cpClass->m_szaIP, cpClass->m_usPort);
				while(cpClass->m_cpConnect[usConnectPos].sConnect < 0)
				{
					printf("%.3d, reconnect %s:%d error, retry...\n", usConnectPos, cpClass->m_szaIP, cpClass->m_usPort);
					Sleep(10000);
					cpClass->m_cpConnect[usConnectPos].sConnect = UC::ConnectSocket(cpClass->m_szaIP, cpClass->m_usPort);
				}
				UC::SetSocketOverTime(cpClass->m_cpConnect[usConnectPos].sConnect, cpClass->m_lTimeOut);
				BOOL BOpt = TRUE;
				setsockopt(cpClass->m_cpConnect[usConnectPos].sConnect, IPPROTO_TCP, TCP_NODELAY, (char*)&BOpt, sizeof(BOOL));
				setsockopt(cpClass->m_cpConnect[usConnectPos].sConnect, SOL_SOCKET, SO_DONTLINGER, (char*)&BOpt, sizeof(BOOL));
				cpClass->m_cConnected.PushData(usConnectPos);
				printf("%.3d, reconnect %s:%d ok\n", usConnectPos, cpClass->m_szaIP, cpClass->m_usPort);
			}
		}

		long InitServerInfo(char* szpIP, unsigned short usPort, unsigned short usConnectNum, long lTimeOut, unsigned __int64 ui64_HeadValidateCode, unsigned __int64 ui64_TailValidateCode)
		{
			m_cpConnect = new CONNECTINFO[usConnectNum];		
			if(m_cConnected.InitQueue(usConnectNum) || m_cUnconnected.InitQueue(usConnectNum) || m_cpConnect == NULL)
			{
				if(m_cpConnect)
				{
					delete m_cpConnect;
					m_cpConnect = NULL;
				}
				m_cConnected.ClearQueue();
				m_cUnconnected.ClearQueue();			
				return -1;
			}
			
			strcpy(m_szaIP, szpIP);
			m_usPort = usPort;
			m_lTimeOut = lTimeOut;
			m_usConnectNum = usConnectNum;
			m_ui64_HeadValidateCode = ui64_HeadValidateCode;
			m_ui64_TailValidateCode = ui64_TailValidateCode;

			DWORD dwID;
			HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread_Connect, this, 0, &dwID);
			if(hThread == NULL)
			{
				if(m_cpConnect)
				{
					delete m_cpConnect;
					m_cpConnect = NULL;
				}
				m_cConnected.ClearQueue();
				m_cUnconnected.ClearQueue();			
				return -1;
			}
			CloseHandle(hThread);

			return 0;
		};

		void ConnectServer()
		{
			printf("add new server %s:%d, connect num %d, begin connect...\n", m_szaIP, m_usPort, m_usConnectNum);
			for(unsigned short i = 0; i < m_usConnectNum; i++)
			{
				m_cpConnect[i].sConnect = UC::ConnectSocket(m_szaIP, m_usPort);			
				while(m_cpConnect[i].sConnect < 0)
				{
					printf("%.3d, connect %s:%d error, retry...\n", i, m_szaIP, m_usPort);
					Sleep(10000);
					m_cpConnect[i].sConnect = UC::ConnectSocket(m_szaIP, m_usPort);
				}
				UC::SetSocketOverTime(m_cpConnect[i].sConnect, m_lTimeOut);
				BOOL BOpt = TRUE;
				setsockopt(m_cpConnect[i].sConnect, IPPROTO_TCP, TCP_NODELAY, (char*)&BOpt, sizeof(BOOL));
				setsockopt(m_cpConnect[i].sConnect, SOL_SOCKET, SO_DONTLINGER, (char*)&BOpt, sizeof(BOOL));
				m_cConnected.PushData(i);				
				printf("%.3d, connect %s:%d ok\n", i, m_szaIP, m_usPort);
			}
			printf("all connect ok\n\n");
		};
	} SERVERINFO;

public:
	UC_LongLink_Client()
	{
		m_usMaxServerInfoNum = -1;
		m_usCurServerInfoNum = -1;
		m_cpServerInfo = NULL;
		InitializeCriticalSection(&m_cServerInfoCriSec);
	};

	~UC_LongLink_Client()
	{		
		UC::ClearSocket();

		if(m_cpServerInfo)
			delete m_cpServerInfo;
		DeleteCriticalSection(&m_cServerInfoCriSec);
	};

	long InitSystem(unsigned short usMaxServerNum)
	{
		if(UC::InitSocket())
			return -1;

		if(usMaxServerNum > 65534)
			return -1;

		m_usMaxServerInfoNum = usMaxServerNum;
		m_usCurServerInfoNum = 0;
		m_cpServerInfo = new SERVERINFO[usMaxServerNum];
		if(m_cpServerInfo == NULL)
			return -1;
	}

	unsigned short AddNewServer(char* szpIP, unsigned short usPort, unsigned short usConnectNum, long lTimeOut, unsigned __int64 ui64_HeadValidateCode, unsigned __int64 ui64_TailValidateCode)
	{
		if(usConnectNum > 65534)
			return -1;

		EnterCriticalSection(&m_cServerInfoCriSec);
		if(m_usCurServerInfoNum >= m_usMaxServerInfoNum)
		{
			LeaveCriticalSection(&m_cServerInfoCriSec);
			return -1;
		}
		SERVERINFO* cpServerInfo = m_cpServerInfo + m_usCurServerInfoNum;
		if(cpServerInfo->InitServerInfo(szpIP, usPort, usConnectNum, lTimeOut, ui64_HeadValidateCode, ui64_TailValidateCode))
		{
			LeaveCriticalSection(&m_cServerInfoCriSec);
			return -1;
		}
		cpServerInfo->ConnectServer();
		unsigned short usServerID = m_usCurServerInfoNum++;
		LeaveCriticalSection(&m_cServerInfoCriSec);

		return usServerID;
	}

	long SendData_Sync(unsigned long ulConnectID, char* szpSendBuf, long lSendLen)
	{
		if(ulConnectID == -1)
			return -1;

		SOCKET sConnect = m_cpServerInfo[*(unsigned short*)&ulConnectID].m_cpConnect[*((unsigned short*)&ulConnectID + 1)].sConnect;
		m_cpServerInfo[*(unsigned short*)&ulConnectID].m_cpConnect[*((unsigned short*)&ulConnectID + 1)].lRetCode = 0;
		if(UC::SendBuffer(sConnect, (char*)&m_cpServerInfo[*(unsigned short*)&ulConnectID].m_ui64_HeadValidateCode, 8))
		{
			m_cpServerInfo[*(unsigned short*)&ulConnectID].m_cpConnect[*((unsigned short*)&ulConnectID + 1)].lRetCode = -1;
			return -1;
		}
		if(UC::SendBuffer(sConnect, szpSendBuf, lSendLen))
		{
			m_cpServerInfo[*(unsigned short*)&ulConnectID].m_cpConnect[*((unsigned short*)&ulConnectID + 1)].lRetCode = -1;
			return -1;
		}
		if(UC::SendBuffer(sConnect, (char*)&m_cpServerInfo[*(unsigned short*)&ulConnectID].m_ui64_TailValidateCode, 8))
		{
			m_cpServerInfo[*(unsigned short*)&ulConnectID].m_cpConnect[*((unsigned short*)&ulConnectID + 1)].lRetCode = -1;
			return -1;
		}
		return 0;
	}

	unsigned long GetConnectID(unsigned short usServerID)
	{
		if(usServerID >= m_usCurServerInfoNum)
			return -1;

		unsigned long ulConnectID = 0;
		*(unsigned short*)&ulConnectID = usServerID;
		*((unsigned short*)&ulConnectID + 1) = m_cpServerInfo[usServerID].m_cConnected.PopData();
		return ulConnectID;
	}

	void FreeConnectID(unsigned long& ulrConnectID)
	{
		if(m_cpServerInfo[*(unsigned short*)&ulrConnectID].m_cpConnect[*((unsigned short*)&ulrConnectID + 1)].lRetCode < 0)
			m_cpServerInfo[*(unsigned short*)&ulrConnectID].m_cUnconnected.PushData(*((unsigned short*)&ulrConnectID + 1));
		else
			m_cpServerInfo[*(unsigned short*)&ulrConnectID].m_cConnected.PushData(*((unsigned short*)&ulrConnectID + 1));

		ulrConnectID = -1;
	}

private:
	unsigned short m_usMaxServerInfoNum;
	unsigned short m_usCurServerInfoNum;
	SERVERINFO* m_cpServerInfo;
	CRITICAL_SECTION m_cServerInfoCriSec;
};

#endif // _UC_LONG_LINK_CLIENT_H_