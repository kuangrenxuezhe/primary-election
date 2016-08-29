// UC_IOManager.h

#ifndef _UC_IO_MANAGER_H_
#define _UC_IO_MANAGER_H_

#include "UH_Define.h"

#define LISTNODENUM   10

typedef struct _IO_NODE
{
	var_u4 ulReadPos; // 读取位置
	var_u4 ulReadLen; // 读取数量	
	var_1* szpReadBuf;        // 缓冲区	
} IO_NODE;

typedef struct _THREAD_NODE
{
	var_u4 ulFileNo;
	void* vpValue;
} THREAD_NODE;

class UC_IOManager
{
public:
	UC_IOManager()
	{
#if (SYSTEM == 1)
		_setmaxstdio(2048);	
#endif
		m_cppFileHandle = NULL;
		m_lFileHandleNum = 0;

		m_cpIONodeLib = NULL;
		m_cppIONodeLibPos = NULL;
		m_lIONodeLibNum = 0;

		m_cpCriSec = NULL;
		m_lpCurNum = NULL;

		m_cpppAddNodeList = NULL;
		m_cpppTmpNodeList = NULL;

		m_cpThreadNodeLib = NULL;

		m_bThreadRunFlag = true;
	}

	~UC_IOManager()
	{
		m_bThreadRunFlag = false;
		cp_sleep(5000);

		if(m_cpCriSec)
			delete m_cpCriSec;
		
		if(m_cpIONodeLib)
			delete m_cpIONodeLib;
		if(m_cppIONodeLibPos)
			delete m_cppIONodeLibPos;
		if(m_cpCriSec)	
			delete m_cpCriSec;
		if(m_lpCurNum)     
			delete m_lpCurNum;
		if(m_cpppAddNodeList)   
			delete m_cpppAddNodeList;
		if(m_cpppTmpNodeList)     
			delete m_cpppTmpNodeList;
		if(m_cpThreadNodeLib)
			delete m_cpThreadNodeLib;
	}

	var_4 IO_InitSystem(FILE** cppFileHandle, var_4 lFileHandleNum)
	{
		m_cppFileHandle = cppFileHandle;
		m_lFileHandleNum = lFileHandleNum;

		m_lIONodeLibNum = m_lFileHandleNum*2*LISTNODENUM;
		m_cpIONodeLib = new IO_NODE[m_lIONodeLibNum];
		m_cppIONodeLibPos = new IO_NODE*[m_lIONodeLibNum];
		for(var_4 i = 0; i < m_lIONodeLibNum; i++)
			m_cppIONodeLibPos[i] = m_cpIONodeLib + i;

		m_cpCriSec = new CP_MUTEXLOCK[m_lFileHandleNum];
		m_lpCurNum = new var_4[m_lFileHandleNum];

		m_cpppAddNodeList = new IO_NODE**[m_lFileHandleNum];
		m_cpppTmpNodeList = new IO_NODE**[m_lFileHandleNum];

		m_cpThreadNodeLib = new THREAD_NODE[m_lFileHandleNum];

		if(!m_cpIONodeLib || !m_cppIONodeLibPos || !m_cpCriSec || !m_lpCurNum || !m_cpppAddNodeList || !m_cpppTmpNodeList || !m_cpThreadNodeLib)
		{
			this->~UC_IOManager();
			return -1;
		}

		for(var_4 j = 0; j < m_lFileHandleNum; j++)
		{
			m_cpppAddNodeList[j] = m_cppIONodeLibPos + LISTNODENUM*j*2;
			m_cpppTmpNodeList[j] = m_cppIONodeLibPos + LISTNODENUM*j*2 + LISTNODENUM;

			m_lpCurNum[j] = 0;
			
			m_cpThreadNodeLib[j].ulFileNo = j;
			m_cpThreadNodeLib[j].vpValue = this;

			if(cp_create_thread(thread_Read, &m_cpThreadNodeLib[j]))
			{
				this->~UC_IOManager();
				return -1;
			}
		}

		return 0;
	}

	var_4 IO_AddBufToList(var_u4 ulFileNo, var_u4 ulReadPos, var_u4 ulReadLen, char* szpReadBuf)
	{
		IO_NODE* cpTempNode = NULL;		
		while(1)
		{
			m_cpCriSec[ulFileNo].lock();
			if(m_lpCurNum[ulFileNo] >= LISTNODENUM)
			{
				m_cpCriSec[ulFileNo].unlock();
				cp_sleep(1);
				continue;
			}
			break;
		}
		cpTempNode = m_cpppAddNodeList[ulFileNo][m_lpCurNum[ulFileNo]++];
		cpTempNode->ulReadPos = ulReadPos;
		cpTempNode->ulReadLen = ulReadLen;
		cpTempNode->szpReadBuf = szpReadBuf;
		*szpReadBuf = 0;
		m_cpCriSec[ulFileNo].unlock();

		return 0;
	}

	var_4 IO_GetBufFromList(char* szpReadBuf)
	{
		while(1)
		{
			if(*szpReadBuf == 0)
			{
				cp_sleep(1);
				continue;
			}
			break;
		}
		if(*szpReadBuf == -1)
			return -1;
		return 0;
	}

	var_4 QuickSort(var_4 lBegin, var_4 lEnd, IO_NODE** cppList)
	{
		if(lBegin >= lEnd)
			return 0;

		IO_NODE* cpTemp;

		if(lEnd == lBegin + 1)
		{
			if(cppList[lBegin]->ulReadPos > cppList[lEnd]->ulReadPos)
			{
				cpTemp = cppList[lBegin];
				cppList[lBegin] = cppList[lEnd];
				cppList[lEnd] = cpTemp;
			}
			return 0;
		}

		var_4 lMid = (lBegin + lEnd)>>1;
		var_u4 lMidValue = cppList[lMid]->ulReadPos;
		var_4 m = lBegin;
		var_4 n = lEnd;

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && cppList[lBegin]->ulReadPos < lMidValue)
				lBegin++;
			while(lBegin < lEnd && cppList[lEnd]->ulReadPos > lMidValue)
				lEnd--;

			if(lBegin < lEnd)
			{
				cpTemp = cppList[lBegin];
				cppList[lBegin] = cppList[lEnd];
				cppList[lEnd] = cpTemp;
				
				lBegin++;
				if(lBegin < lEnd)
					lEnd--;
			}
		}

		if(cppList[lBegin]->ulReadPos < lMidValue)
			lBegin++;
		if(m < lBegin)
			QuickSort(m, lBegin-1, cppList);	
		if(lEnd < n)
			QuickSort(lEnd, n, cppList);	

		return 0;
	}

	static CP_THREAD_T thread_Read(void* vpArg)
	{
		THREAD_NODE* cpInfoNode = (THREAD_NODE*)vpArg;
		UC_IOManager* cpClass = (UC_IOManager*)(cpInfoNode->vpValue);
		var_u4 ulFileNo = cpInfoNode->ulFileNo;

		var_4 lTempNum = 0;
		IO_NODE** cppTempList = NULL;

		while(cpClass->m_bThreadRunFlag)
		{
			cpClass->m_cpCriSec[ulFileNo].lock();
			if(cpClass->m_lpCurNum[ulFileNo] == 0)
			{
				cpClass->m_cpCriSec[ulFileNo].unlock();
				cp_sleep(1);
				continue;
			}
			cppTempList = cpClass->m_cpppAddNodeList[ulFileNo];
			cpClass->m_cpppAddNodeList[ulFileNo] = cpClass->m_cpppTmpNodeList[ulFileNo];
			cpClass->m_cpppTmpNodeList[ulFileNo] = cppTempList;
			lTempNum = cpClass->m_lpCurNum[ulFileNo];
			cpClass->m_lpCurNum[ulFileNo] = 0;
			cpClass->m_cpCriSec[ulFileNo].unlock();

			cpClass->QuickSort(0, lTempNum - 1, cppTempList);

			for(var_4 i = 0; i < lTempNum; i++)
			{
				if(fseek(cpClass->m_cppFileHandle[ulFileNo], cppTempList[i]->ulReadPos, SEEK_SET))
				{
					printf("Seek Error\n");
					*cppTempList[i]->szpReadBuf = -1;
					continue;
				}
				if(fread(cppTempList[i]->szpReadBuf + 1, cppTempList[i]->ulReadLen, 1, cpClass->m_cppFileHandle[ulFileNo]) != 1)
				{
					printf("Read Error\n");
					*cppTempList[i]->szpReadBuf = -1;
				} 
				else
					*cppTempList[i]->szpReadBuf = 1;
			}
		}

		return 0;
	}

private:
	FILE** m_cppFileHandle;
	var_4 m_lFileHandleNum;

	IO_NODE* m_cpIONodeLib;
	IO_NODE** m_cppIONodeLibPos;
	var_4 m_lIONodeLibNum;

    CP_MUTEXLOCK* m_cpCriSec;
	var_4* m_lpCurNum;

	IO_NODE*** m_cpppAddNodeList;
	IO_NODE*** m_cpppTmpNodeList;

	THREAD_NODE* m_cpThreadNodeLib;

	bool m_bThreadRunFlag;
};

#endif // _UC_IO_MANAGER_H_

