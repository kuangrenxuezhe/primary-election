// UT_DigitSearch.h

#ifndef _UT_DIGIT_SEARCH_H_
#define _UT_DIGIT_SEARCH_H_

#include ""

#include "../CodeLibrary/complete/UT_Arithmetic.h"

#define SEGMENTNUM	(16<<20)
#define GETINDEX(x)	((long)(x>>40))

typedef unsigned __int64 u_int64;

class UT_DigitSearch
{
private:
	long  m_lMaxCapability;
	long  m_lCurCapability;
	long* m_lpKeyIndex;
	u_int64* m_ui64_KeyLibrary;
	UT_Arithmetic<u_int64> m_cArithmetic;

public:
	UT_DigitSearch()
	{
		m_lCurCapability = 0;
		m_lMaxCapability = 0;
		m_lpKeyIndex = NULL;
		m_ui64_KeyLibrary = NULL;
	}

	~UT_DigitSearch()
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

	long InitDigitSearch(long lMaxCapability)
	{
		m_lCurCapability = 0;
		m_lMaxCapability = lMaxCapability;
		m_lpKeyIndex = new long[SEGMENTNUM + 1];
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
		long lSegIdx = GETINDEX(ui64_Key);
		return m_cArithmetic.BinarySearch_Default(m_lpKeyIndex[lSegIdx], m_lpKeyIndex[lSegIdx + 1] - 1, m_ui64_KeyLibrary, ui64_Key);
	}

	void ClearUp()
	{
		m_cArithmetic.QuickSort_A(0, m_lCurCapability - 1, m_ui64_KeyLibrary);

		memset(m_lpKeyIndex, 0, (SEGMENTNUM + 1)<<2);	
		for(long i = 0; i < m_lCurCapability; i++)
			m_lpKeyIndex[GETINDEX(m_ui64_KeyLibrary[i]) + 1]++;
		for(long j = 1; j < SEGMENTNUM + 1; j++)
			m_lpKeyIndex[j] = m_lpKeyIndex[j-1]+m_lpKeyIndex[j];
	}

	void Clear()
	{
		m_lCurCapability = 0;
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

#endif // _UT_DIGIT_SEARCH_H_
