// UC_FileTransfers_Server.h

#ifndef _UC_FILE_TRANSFERS_SERVER_H_
#define _UC_FILE_TRANSFERS_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "./stable/UC_Communication.h"

#define RECVBUFLEN	1048576

class UC_FileTransfers_Server
{
public:
	UC_FileTransfers_Server()
	{
		m_lMaxConnectNum = 0;
		m_lCurConnectNum = 0;

		InitializeCriticalSection(&m_cCriSec);
	}

	~UC_FileTransfers_Server()
	{
		UC::ClearSocket();

		DeleteCriticalSection(&m_cCriSec);
	}

	long InitSystem(unsigned short usPort, long lMaxConnectNum)
	{
		if(lMaxConnectNum > 2000)
			return -1;
		m_lMaxConnectNum = lMaxConnectNum;
		if(UC::InitSocket())
			return -1;
		if(UC::CreateListen(usPort, m_sListen))
		{
			UC::ClearSocket();
			return -1;
		}
		if(UC::StartThread(Thread_Listen, this))
		{
			UC::ClearSocket();
			return -1;
		}
		_setmaxstdio(2048);
		return 0;
	}

private:
	struct _Thread_Node
	{
		UC_FileTransfers_Server* cpClass;
		SOCKET sClient;
		FILE* fpHandle;
		char szRequestType;
		char szUseFlag;
		char* szpBuffer;
	};

	static DWORD WINAPI Thread_Transfers(void* vpArg)
	{
		FILE* fpHandle = ((_Thread_Node*)vpArg)->fpHandle;
		SOCKET sClient = ((_Thread_Node*)vpArg)->sClient;
		char* szpBuffer = ((_Thread_Node*)vpArg)->szpBuffer;
		char szRequestType = ((_Thread_Node*)vpArg)->szRequestType;
		UC_FileTransfers_Server* cpClass = ((_Thread_Node*)vpArg)->cpClass;		
		((_Thread_Node*)vpArg)->szUseFlag = 0;
		
		long commbuf[2];
		if(szRequestType == 'W')		
		{			
			for(;;)
			{
				if(UC::RecvBuffer(sClient, (char*)commbuf, 8))
					break;
				if(commbuf[0] == 0)
					break;				
				while(commbuf[1])
				{
					long recvlen = (commbuf[1] >= RECVBUFLEN ? RECVBUFLEN : commbuf[1]);
					commbuf[1] -= recvlen;

					if(UC::RecvBuffer(sClient, szpBuffer, recvlen))
					{
						commbuf[0] = -1;
						break;
					}
					if(fwrite(szpBuffer, 1, recvlen, fpHandle) != (unsigned)recvlen)
					{
						commbuf[0] = -1;
						break;
					}
				}
				if(commbuf[0] == -1)
					memcpy(commbuf, "NONONONO", 8);
				else
					memcpy(commbuf, "OKOKOKOK", 8);
				if(UC::SendBuffer(sClient, (char*)commbuf, 8))
					break;
			}
		}
		else if(szRequestType == 'R')
		{
			for(;;)
			{
				if(UC::RecvBuffer(sClient, (char*)commbuf, 8))
					break;
				if(commbuf[0] == 0)
					break;
				while(commbuf[1])
				{
					long sendlen = commbuf[1] >= RECVBUFLEN ? RECVBUFLEN : commbuf[1];
					commbuf[1] -= sendlen;

					long readlen = fread(szpBuffer, 1, sendlen, fpHandle);
					if(readlen != sendlen && ferror(fpHandle))
					{
						commbuf[0] = -1;
						break;
					}
					long headbuf[2];
					headbuf[0] = sendlen;
					headbuf[1] = readlen;
					if(UC::SendBuffer(sClient, (char*)headbuf, 8))
					{
						commbuf[0] = -1;
						break;
					}
					if(UC::SendBuffer(sClient, szpBuffer, readlen))
					{
						commbuf[0] = -1;
						break;
					}
				}
				if(commbuf[0] == -1)
					break;
			}
		}

        delete szpBuffer;
		fclose(fpHandle);
		UC::CloseSocket(sClient);
		EnterCriticalSection(&cpClass->m_cCriSec);
		cpClass->m_lCurConnectNum--;
		LeaveCriticalSection(&cpClass->m_cCriSec);

		return 0;
	}

	static __int64 FileOperation(char* buffer)
	{
		// 文件存在    1
		// 文件不存在  0
		// 函数错误   -1
		if(*(long*)(buffer + 4) == *(long*)"C-A:")
		{
			int ret = access(buffer + 8, 0);
			if(ret == 0)
				return 1;
			if(errno == ENOENT)
				return 0;
			return -1;
		}
		// 删除成功    1
		// 文件不存在  0
		// 函数错误   -1
		else if(*(long*)(buffer + 4) == *(long*)"C-D:")
		{
			int ret = remove(buffer + 8);
			if(ret == 0)
				return 1;
			if(errno == ENOENT)
				return 1;
			return -1;
		}
		// 成功返回 长度
		// 失败返回   -1
		else if(*(long*)(buffer + 4) == *(long*)"C-S:")
		{
			struct _stati64 cInfo;
			if(_stati64(buffer + 8, &cInfo))
				return -1;
			return cInfo.st_size;
		}
		else
			return -1;

		return -1;
	}

	static DWORD WINAPI Thread_Listen(void* vpArg)
	{
		UC_FileTransfers_Server* cpClass = (UC_FileTransfers_Server*)vpArg;

		char szaRecvBuf[1024];
		FILE* fpHandle = NULL;
		_Thread_Node cNode;

		for(;;)
		{
			SOCKET sClient;
			if(UC::AcceptSocket(cpClass->m_sListen, sClient))
				continue;

			EnterCriticalSection(&cpClass->m_cCriSec);
			// 接收请求类型及文件
			if(UC::RecvBuffer(sClient, szaRecvBuf, 4))
			{
				UC::CloseSocket(sClient);
				LeaveCriticalSection(&cpClass->m_cCriSec);
				continue;
			}
			if(UC::RecvBuffer(sClient, szaRecvBuf + 4, *(long*)szaRecvBuf - 4))
			{
				UC::CloseSocket(sClient);
				LeaveCriticalSection(&cpClass->m_cCriSec);
				continue;
			}
			szaRecvBuf[*(long*)szaRecvBuf] = 0;
			// 文件控制请求
			if(*(long*)(szaRecvBuf + 4)== *(long*)"C-A:" || *(long*)(szaRecvBuf + 4) == *(long*)"C-D:" || *(long*)(szaRecvBuf + 4) == *(long*)"C-S:")
			{
				*(__int64*)(szaRecvBuf + 8) = FileOperation(szaRecvBuf);
				UC::SendBuffer(sClient, szaRecvBuf + 4, 12);

				UC::CloseSocket(sClient);
				LeaveCriticalSection(&cpClass->m_cCriSec);
				continue;
			}
			// 判断最大连接数
			if(cpClass->m_lCurConnectNum >= cpClass->m_lMaxConnectNum)
			{
				UC::SendBuffer(sClient, "NO", 2);
				UC::CloseSocket(sClient);
				LeaveCriticalSection(&cpClass->m_cCriSec);
				continue;
			}
			// 根据请求类型打开文件
			if(szaRecvBuf[4] == 'W')
				fpHandle = fopen(szaRecvBuf + 5, "wb");
			else if(szaRecvBuf[4] == 'R')
				fpHandle = fopen(szaRecvBuf + 5, "rb");
			else
			{
				UC::SendBuffer(sClient, "NO", 2);
				UC::CloseSocket(sClient);
				LeaveCriticalSection(&cpClass->m_cCriSec);
				continue;
			}
			if(fpHandle == NULL)
			{
				UC::SendBuffer(sClient, "NO", 2);
				UC::CloseSocket(sClient);
				LeaveCriticalSection(&cpClass->m_cCriSec);
				continue;
			}			
			// 启动工作线程			
			cNode.cpClass = cpClass;
			cNode.sClient = sClient;
			cNode.fpHandle = fpHandle;
			cNode.szUseFlag = 1;
			cNode.szRequestType = szaRecvBuf[4];
			cNode.szpBuffer = new char[RECVBUFLEN];
			if(cNode.szpBuffer == NULL)
			{
				UC::SendBuffer(sClient, "NO", 2);
				UC::CloseSocket(sClient);
				cNode.szUseFlag = 0;
			}
			if(UC::StartThread(Thread_Transfers, (void*)&cNode))
			{				
				UC::SendBuffer(sClient, "NO", 2);
				UC::CloseSocket(sClient);
				cNode.szUseFlag = 0;
			}
			else
			{
				UC::SendBuffer(sClient, "OK", 2);
				cpClass->m_lCurConnectNum++;
			}
			while(cNode.szUseFlag)
				Sleep(10);
			LeaveCriticalSection(&cpClass->m_cCriSec);
		}

		return 0;
	}

public:
	long m_lMaxConnectNum;
	long m_lCurConnectNum;
	SOCKET m_sListen;
	CRITICAL_SECTION m_cCriSec;
};

#endif // _UC_FILE_TRANSFERS_SERVER_H_