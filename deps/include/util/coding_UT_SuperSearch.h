#ifndef __SUPERSEARCH__HEAD__
#define __SUPERSEARCH__HEAD__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MemoryManage.h"
typedef unsigned __int64 u64;
typedef unsigned long u32;
typedef unsigned char u8;
#pragma pack(1)
struct u40
{
	u8	h;
	u32	l;
	u40 operator=(u40 a)
	{
		this->h = a.h;
		this->l = a.l;
		return *this;
	};
	u40 operator=(u64 &a)
	{
		this->l = (u32)(a & 0xffffffff);
		this->h = (u8)((a & 0xff00000000) >> 32);
		return *this;
	};
	bool operator>(u40 b)
	{
		if(this->h > b.h) return true;
		if(this->h < b.h)	return false;
		return this->l > b.l;
	};
	bool operator>=(u40 b)
	{
		if(this->h > b.h) return true;
		if(this->h < b.h)	return false;
		return this->l >= b.l;
	};
	bool operator<(u40 b)
	{
		if(this->h < b.h) return true;
		if(this->h > b.h)	return false;
		return this->l < b.l;
	};
	bool operator<=(u40 b)
	{
		if(this->h < b.h) return true;
		if(this->h > b.h)	return false;
		return this->l <= b.l;
	};
	bool operator==(u40 b)
	{
		return (b.h == this->h && b.l == this->l);
	};
};
#pragma pack()

class SuperSearch
{
public:
	u40 **m_Search;
	long *m_lBlockNum;
	MemoryManage *pManage;
public:

	SuperSearch()
	{
		m_lBlockNum = new long[1<<24];
		memset(m_lBlockNum,0,4<<24);
		m_Search = new u40*[1<<24];	
		pManage = new MemoryManage(1024,10<<20);
	};
	~SuperSearch()
	{
		delete m_lBlockNum;
		delete m_Search;
		delete pManage;
	}
	long Load(char *file)
	{
		FILE *fp = fopen(file,"rb");

		u64 *lpTmp = new u64[256<<10];
		long lLen,i;
		while((lLen = fread(lpTmp,8,256<<10,fp)) > 0)
		{
			for(i = 0;i< lLen;i++)
				m_lBlockNum[lpTmp[i]>>40] ++;
		}
		long lLoop = 1 << 24;
		for(i = 0;i<lLoop;i++)
		{
			m_Search[i] = (u40*)pManage->Alloc(m_lBlockNum[i] * sizeof(u40));
		}
		memset(m_lBlockNum,0,4<<24);
		fseek(fp,0,0);
		while((lLen = fread(lpTmp,8,256<<10,fp)) > 0)
		{
			for(i = 0;i< lLen;i++)
				m_Search[lpTmp[i]>>40][m_lBlockNum[lpTmp[i]>>40]++] = lpTmp[i];
		}
		fclose(fp);
		for(i=0;i<lLoop;i++)
		{
			if((i % 10000) == 0) printf("%d\n",i);
			Sort(m_Search[i],0,m_lBlockNum[i]-1);
		}
		delete lpTmp;
		return 0;
	};
	bool IsExist(u64 Doc)
	{
		u40 * s = m_Search[Doc>>40];
		u40 key ;
		key = Doc;
		long m,l=0,r = m_lBlockNum[Doc>>40]-1;
		while(l <= r)
		{
			m = (l + r)/2;
			if(s[m] > key)
				r = m - 1;
			else if(s[m] < key)
				l = m + 1;
			else
				return true;
		}
		return false;
	};
	void Sort(u40 *lpIndex,long lBegin,long lEnd)
	{
		if(lBegin >= lEnd)
			return;
		u40 m;
		if(lBegin + 1 == lEnd)
		{
			if(lpIndex[lBegin] > lpIndex[lEnd])
			{
				m = lpIndex[lBegin];
				lpIndex[lBegin] = lpIndex[lEnd];
				lpIndex[lEnd] = m;
			}
			return ;
		}
		u40  c;
		c = lpIndex[(lBegin + lEnd)/2];
		long b = lBegin;
		long e = lEnd;
		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && lpIndex[lBegin] < c)
				lBegin ++;
			while(lBegin < lEnd && lpIndex[lEnd] > c)
				lEnd --;
			if(lBegin < lEnd)
			{
				m = lpIndex[lBegin];
				lpIndex[lBegin] = lpIndex[lEnd];
				lpIndex[lEnd] = m;

				lBegin ++;
				if(lBegin < lEnd)
					lEnd --;
			}
		}
		if(lpIndex[lBegin] < c)
			lBegin ++;
		if(b < lBegin)
			Sort(lpIndex, b , lBegin-1);	
		if(lEnd < e)
			Sort(lpIndex, lEnd, e);	
		return;
	}
private:
};


#endif//__SUPERSEARCH__HEAD__
