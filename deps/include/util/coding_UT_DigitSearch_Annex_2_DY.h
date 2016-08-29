// UT_DigitSearch_Annex_2_DY.h

#ifndef _UT_DIGIT_SEARCH_ANNEX_2_DY_H_
#define _UT_DIGIT_SEARCH_ANNEX_2_DY_H_

#include <io.h>

#include "UT_Arithmetic.h"

typedef unsigned __int64 u_int64;

class UT_DigitSearch_Annex_2_DY // Dynamic
{
private:
	long  m_lMaxCapability;
	long  m_lCurCapability;

	long m_lSegmentSize;
	long  m_lRightMoveNum;

	long* m_lpKeyIndex;
	u_int64* m_ui64_KeyLibrary;
	U_Arithmetic<u_int64, void*> m_cArithmetic;

	void** m_vpp_AnnexLibrary_1;
	void** m_vpp_AnnexLibrary_2;

public:
	UT_DigitSearch_Annex_2_DY()
	{
		m_lCurCapability = 0;
		m_lMaxCapability = 0;
		m_lpKeyIndex = NULL;
		m_ui64_KeyLibrary = NULL;

		m_vpp_AnnexLibrary_1 = NULL;
		m_vpp_AnnexLibrary_2 = NULL;
	}

	~UT_DigitSearch_Annex_2_DY()
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
		if(m_vpp_AnnexLibrary_1)
		{
			delete m_vpp_AnnexLibrary_1;
			m_vpp_AnnexLibrary_1 = NULL;
		}
		if(m_vpp_AnnexLibrary_2)
		{
			delete m_vpp_AnnexLibrary_2;
			m_vpp_AnnexLibrary_2 = NULL;
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
		m_vpp_AnnexLibrary_1 = new void*[m_lMaxCapability];
		m_vpp_AnnexLibrary_2 = new void*[m_lMaxCapability];
		if(m_lpKeyIndex == NULL || m_ui64_KeyLibrary == NULL || m_vpp_AnnexLibrary_1 == NULL || m_vpp_AnnexLibrary_2 == NULL)
		{
			if(m_lpKeyIndex) delete m_lpKeyIndex;
			if(m_ui64_KeyLibrary) delete m_ui64_KeyLibrary;
			if(m_vpp_AnnexLibrary_1) delete m_vpp_AnnexLibrary_1;
			if(m_vpp_AnnexLibrary_2) delete m_vpp_AnnexLibrary_2;
			return -1;
		}
		return 0;		
	}

	long AddKey(u_int64 ui64_Key, void* vpAnnex_1, void* vpAnnex_2)
	{
		if(m_lCurCapability >= m_lMaxCapability)
			return -1;
		m_vpp_AnnexLibrary_1[m_lCurCapability] = vpAnnex_1;
		m_vpp_AnnexLibrary_2[m_lCurCapability] = vpAnnex_2;
		m_ui64_KeyLibrary[m_lCurCapability++] = ui64_Key;	
		return 0;
	}

	long SearchKey(u_int64 ui64_Key, void*& vprAnnex_1, void*& vprAnnex_2)
	{
		long lRetVal = m_cArithmetic.BSearch(m_lpKeyIndex[ui64_Key>>m_lRightMoveNum], m_lpKeyIndex[(ui64_Key>>m_lRightMoveNum) + 1] - 1, m_ui64_KeyLibrary, ui64_Key);
		if(lRetVal < 0)
			return -1;
		vprAnnex_1 = m_vpp_AnnexLibrary_1[lRetVal];
		vprAnnex_2 = m_vpp_AnnexLibrary_2[lRetVal];
		return lRetVal;
	}

	void SetAnnex(long lIndex, void* vpAnnex_1, void* vpAnnex_2)
	{
		m_vpp_AnnexLibrary_1[lIndex] = vpAnnex_1;
		m_vpp_AnnexLibrary_2[lIndex] = vpAnnex_2;
	}

	void ClearUp()
	{
		m_cArithmetic.QuickSort_U(0, m_lCurCapability - 1, m_ui64_KeyLibrary, m_vpp_AnnexLibrary_1, m_vpp_AnnexLibrary_2);

		memset(m_lpKeyIndex, 0, (m_lSegmentSize + 1)<<2);	

		for(long i = 0; i < m_lCurCapability; i++)
			m_lpKeyIndex[(m_ui64_KeyLibrary[i] >> m_lRightMoveNum) + 1]++;
		for(long j = 1; j < m_lSegmentSize + 1; j++)
			m_lpKeyIndex[j] = m_lpKeyIndex[j - 1] + m_lpKeyIndex[j];
	}

	void GetValueByIndex(long lPosition, void*& vprAnnex_1, void*& vprAnnex_2)
	{
		vprAnnex_1 = m_vpp_AnnexLibrary_1[lPosition];
		vprAnnex_2 = m_vpp_AnnexLibrary_2[lPosition];
	}

	void Clear()
	{
		m_lCurCapability = 0;
	}

	long GetNum()
	{
		return m_lCurCapability;
	}
};

#endif // _UT_DIGIT_SEARCH_ANNEX_2_DY_H_