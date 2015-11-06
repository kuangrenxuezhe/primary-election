// UT_DigitSearch_DY.h

#ifndef _UT_DIGIT_SEARCH_DY_H_
#define _UT_DIGIT_SEARCH_DY_H_

#include <io.h>

#include "UT_Arithmetic.h"

typedef unsigned __int64 u_int64;

class UT_DigitSearch_DY // Dynamic
{
private:
	long  m_lMaxCapability;
	long  m_lCurCapability;

	long m_lSegmentSize;
	long  m_lRightMoveNum;

	long* m_lpKeyIndex;
	u_int64* m_ui64_KeyLibrary;
	U_Arithmetic<u_int64> m_cArithmetic;

public:
	UT_DigitSearch_DY()
	{
		m_lCurCapability = 0;
		m_lMaxCapability = 0;
		m_lpKeyIndex = NULL;
		m_ui64_KeyLibrary = NULL;
	}

	~UT_DigitSearch_DY()
	{
		m_lCurCapability = 0;
		m_lMaxCapability = 0;
		if(m_lpKeyIndex) 
		{
			delete m_lpKeyIndex;
			m_lpKeyIndex = NULL;
		}
		if(m_ui64_KeyLibrary) 
		{
			delete m_ui64_KeyLibrary;
			m_ui64_KeyLibrary = NULL;
		}
	}

	long InitDigitSearch(long lMaxCapability, long lSegmentSize = 10)
	{
		m_lCurCapability = 0;
		m_lMaxCapability = lMaxCapability;

		m_lSegmentSize = (m_lMaxCapability + lSegmentSize - 1) / lSegmentSize;
		for(m_lRightMoveNum = 1; m_lSegmentSize >>= 1; m_lRightMoveNum++);
		if(m_lRightMoveNum > 24) m_lRightMoveNum = 24;
		m_lSegmentSize = 1<<m_lRightMoveNum;
		m_lRightMoveNum = 64 - m_lRightMoveNum;

		m_lpKeyIndex = new long[m_lSegmentSize + 1];
		m_ui64_KeyLibrary = new u_int64[m_lMaxCapability];
		if(m_lpKeyIndex == NULL || m_ui64_KeyLibrary == NULL)
		{
			if(m_lpKeyIndex) delete m_lpKeyIndex;
			if(m_ui64_KeyLibrary) delete m_ui64_KeyLibrary;
			return -1;
		}
		return 0;		
	}

	long AddKey(u_int64 ui64_Key)
	{
		if(m_lCurCapability >= m_lMaxCapability)
			return -1;
		m_ui64_KeyLibrary[m_lCurCapability++] = ui64_Key;	
		return 0;
	}

	long SearchKey(u_int64 ui64_Key)
	{
		long lRetVal = m_cArithmetic.BSearch(m_lpKeyIndex[ui64_Key>>m_lRightMoveNum], m_lpKeyIndex[(ui64_Key>>m_lRightMoveNum) + 1] - 1, m_ui64_KeyLibrary, ui64_Key);
		if(lRetVal < 0)
			return -1;
		return lRetVal;
	}

	void ClearUp()
	{
		m_cArithmetic.QuickSort_U(0, m_lCurCapability - 1, m_ui64_KeyLibrary);

		memset(m_lpKeyIndex, 0, (m_lSegmentSize + 1)<<2);	

		for(long i = 0; i < m_lCurCapability; i++)
			m_lpKeyIndex[(m_ui64_KeyLibrary[i] >> m_lRightMoveNum) + 1]++;
		for(long j = 1; j < m_lSegmentSize + 1; j++)
			m_lpKeyIndex[j] = m_lpKeyIndex[j - 1] + m_lpKeyIndex[j];
	}

	void Clear()
	{
		m_lCurCapability = 0;
	}

	long GetNum()
	{
		return m_lCurCapability;
	}

	long LoadFile(char* filename)
	{
		this->~UT_DigitSearch();

		u_int64 ui64_ID;

		FILE* fp = fopen(filename, "rb");
		if(fp == NULL)
			return -1;
		long lFileLen = filelength(fileno(fp));
		if(lFileLen%8)
		{
			fclose(fp);
			return -1;
		}
		lFileLen /= 8;
		if(InitDigitSearch(lFileLen))
		{
			fclose(fp);
			return -1;
		}
		for(long i = 0; i < lFileLen; i++)
		{
			if(fread(&ui64_ID, 8, 1, fp) != 1)
			{
				fclose(fp);
				return -1;
			}
			if(AddKey(ui64_ID) < 0)
			{
				fclose(fp);
				return -1;
			}
		}
		fclose(fp);
		ClearUp();

		return lFileLen;
	}
};

#endif // _UT_DIGIT_SEARCH_DY_H_
