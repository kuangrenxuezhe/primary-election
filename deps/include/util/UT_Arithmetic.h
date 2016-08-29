// UT_Arithmetic.h

#ifndef _UT_ARITHMETIC_H_
#define _UT_ARITHMETIC_H_

#include "UH_Define.h"

// Ascending and Descending

template <class T_Key>
inline var_4 _DefaultCompareKey_QA_(T_Key tKey1, T_Key tKey2)
{
	if(tKey1 > tKey2)
		return 1;
	if(tKey1 < tKey2)
		return -1;
	return 0;
}

template <class T_Key>
inline var_4 _DefaultCompareKey_QD_(T_Key tKey1, T_Key tKey2)
{
	if(tKey1 > tKey2)
		return -1;
	if(tKey1 < tKey2)
		return 1;
	return 0;
}

template <class T_Key>
inline var_4 _DefaultCompareKey_QA_S_(T_Key tKey1, T_Key tKey2, var_4 lKey1, var_4 lKey2)
{
	if(tKey1 > tKey2)
		return 1;
	if(tKey1 < tKey2)
		return -1;
	if(lKey1 > lKey2)
		return 1;
	if(lKey1 < lKey2)
		return -1;
	return 0;
}

template <class T_Key>
inline var_4 _DefaultCompareKey_QD_S_(T_Key tKey1, T_Key tKey2, var_4 lKey1, var_4 lKey2)
{
	if(tKey1 > tKey2)
		return -1;
	if(tKey1 < tKey2)
		return 1;
	if(lKey1 > lKey2)
		return 1;
	if(lKey1 < lKey2)
		return -1;
	return 0;
}

template<class T_Key>
inline var_4 _DefaultCompareKey_BS_(T_Key tKey1, T_Key tKey2)
{
	if(tKey1 > tKey2)
		return 1;
	if(tKey1 < tKey2)
		return -1;
	return 0;
}

template <class T_Key, class T_Annex_1 = void*, class T_Annex_2 = void*, class T_Annex_3 = void*>
class UT_Arithmetic
{
public:
	/************************************************************************/
	/* 二分查找                                                             */
	/************************************************************************/
	var_4 BinarySearch_Default(var_4 lBegin, var_4 lEnd, T_Key* tpLibrary, T_Key tKey)
	{
		if(lBegin > lEnd)
			return -1;

		T_Key tMidVal;
		var_4 lMidVal;

		while(lBegin <= lEnd)
		{
			lMidVal = (lBegin + lEnd)>>1;
			tMidVal = tpLibrary[lMidVal];
			if(tMidVal < tKey)
				lBegin = lMidVal + 1;
			else if(tMidVal > tKey)
				lEnd = lMidVal - 1;
			else
				return lMidVal;
		}

		return -1;
	}

	var_4 BinarySearch_Custom(var_4 lBegin, var_4 lEnd, T_Key* tpLibrary, T_Key tKey, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_BS_)
	{
		if(lBegin > lEnd)
			return -1;

		T_Key tMidVal;
		var_4 lMidVal;

		while(lBegin <= lEnd)
		{
			lMidVal = (lBegin + lEnd)>>1;
			tMidVal = tpLibrary[lMidVal];

			if(_DefaultCompareKey_BS_(tMidVal, tKey) < 0)
				lBegin = lMidVal + 1;
			else if(_DefaultCompareKey_BS_(tMidVal, tKey) > 0)
				lEnd = lMidVal - 1;
			else
				return lMidVal;
		}

		return -1;
	};

	var_4 BinarySearch_Fuzzzy(var_4 lBegin, var_4 lEnd, T_Key* tpLibrary, T_Key tKey)
	{
		if(lBegin > lEnd)
			return -1;

		T_Key tMidVal;
		var_4 lMidVal;

		while(lBegin <= lEnd)
		{
			lMidVal = (lBegin + lEnd)>>1;
			tMidVal = tpLibrary[lMidVal];
			if(tMidVal < tKey)
				lBegin = lMidVal + 1;
			else if(tMidVal > tKey)
				lEnd = lMidVal - 1;
			else
				return lMidVal;
		}

		return lMidVal;
	}
	/************************************************************************/
	/* 非稳定变长快排                                                       */
	/************************************************************************/	
	var_4 QuickSort_V(var_4 lBegin, var_4 lEnd, void* vpKey1, 
		             var_4 (*_CompareKey)(void* vpKey1, var_4 m, var_4 n),
					 var_4 (*_ChangeKey)(void* vpKey1, var_4 m, var_4 n))
	{
		if(lBegin >= lEnd)
			return 0;

		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(vpKey1, lBegin, lEnd) > 0)
				_ChangeKey(vpKey1, lBegin, lEnd);
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(vpKey1, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(vpKey1, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{				
				_ChangeKey(vpKey1, lBegin, lEnd);

				if(lBegin == lMid)
					lMid = lEnd;
				else if(lEnd == lMid)
					lMid = lBegin;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(vpKey1, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			QuickSort_V(m, lBegin - 1, vpKey1, _CompareKey, _ChangeKey);
		if(lEnd < n)
			QuickSort_V(lEnd, n, vpKey1, _CompareKey, _ChangeKey);

		return 0;
	};
	var_4 QuickSort_V(var_4 lBegin, var_4 lEnd, void* vpKey1, void* vpKey2,
					 var_4 (*_CompareKey)(void* vpKey1, void* vpKey2, var_4 m, var_4 n),
					 var_4 (*_ChangeKey)(void* vpKey1, void* vpKey2, var_4 m, var_4 n))
	{
		if(lBegin >= lEnd)
			return 0;

		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(vpKey1, vpKey2, lBegin, lEnd) > 0)
				_ChangeKey(vpKey1, vpKey2, lBegin, lEnd);
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{				
				_ChangeKey(vpKey1, vpKey2, lBegin, lEnd);

				if(lBegin == lMid)
					lMid = lEnd;
				else if(lEnd == lMid)
					lMid = lBegin;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(vpKey1, vpKey2, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			QuickSort_V(m, lBegin - 1, vpKey1, vpKey2, _CompareKey, _ChangeKey);
		if(lEnd < n)
			QuickSort_V(lEnd, n, vpKey1, vpKey2, _CompareKey, _ChangeKey);

		return 0;
	};
	var_4 QuickSort_V(var_4 lBegin, var_4 lEnd, void* vpKey1, void* vpKey2, void* vpKey3, 
		             var_4 (*_CompareKey)(void* vpKey1, void* vpKey2, void* vpKey3, var_4 m, var_4 n),
					 var_4 (*_ChangeKey)(void* vpKey1, void* vpKey2, void* vpKey3, var_4 m, var_4 n))
	{
		if(lBegin >= lEnd)
			return 0;

		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(vpKey1, vpKey2, vpKey3, lBegin, lEnd) > 0)
				_ChangeKey(vpKey1, vpKey2, vpKey3, lBegin, lEnd);
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, vpKey3, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, vpKey3, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{				
				_ChangeKey(vpKey1, vpKey2, vpKey3, lBegin, lEnd);

				if(lBegin == lMid)
					lMid = lEnd;
				else if(lEnd == lMid)
					lMid = lBegin;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(vpKey1, vpKey2, vpKey3, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			QuickSort_V(m, lBegin - 1, vpKey1, vpKey2, vpKey3, _CompareKey, _ChangeKey);
		if(lEnd < n)
			QuickSort_V(lEnd, n, vpKey1, vpKey2, vpKey3, _CompareKey, _ChangeKey);

		return 0;
	};
	var_4 QuickSort_V(var_4 lBegin, var_4 lEnd, void* vpKey1, void* vpKey2, void* vpKey3, void* vpKey4, 
		             var_4 (*_CompareKey)(void* vpKey1, void* vpKey2, void* vpKey3, void* vpKey4, var_4 m, var_4 n),
					 var_4 (*_ChangeKey)(void* vpKey1, void* vpKey2, void* vpKey3, void* vpKey4, var_4 m, var_4 n))
	{
		if(lBegin >= lEnd)
			return 0;

		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, lBegin, lEnd) > 0)
				_ChangeKey(vpKey1, vpKey2, vpKey3, vpKey4, lBegin, lEnd);
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{				
				_ChangeKey(vpKey1, vpKey2, vpKey3, vpKey4, lBegin, lEnd);

				if(lBegin == lMid)
					lMid = lEnd;
				else if(lEnd == lMid)
					lMid = lBegin;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			QuickSort_V(m, lBegin - 1, vpKey1, vpKey2, vpKey3, vpKey4, _CompareKey, _ChangeKey);
		if(lEnd < n)
			QuickSort_V(lEnd, n, vpKey1, vpKey2, vpKey3, vpKey4, _CompareKey, _ChangeKey);

		return 0;
	};
	var_4 QuickSort_V(var_4 lBegin, var_4 lEnd, void* vpKey1, void* vpKey2, void* vpKey3, void* vpKey4, void* vpKey5,
		             var_4 (*_CompareKey)(void* vpKey1, void* vpKey2, void* vpKey3, void* vpKey4, void* vpKey5, var_4 m, var_4 n),
					 var_4 (*_ChangeKey)(void* vpKey1, void* vpKey2, void* vpKey3, void* vpKey4, void* vpKey5, var_4 m, var_4 n))
	{
		if(lBegin >= lEnd)
			return 0;

		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, lBegin, lEnd) > 0)
				_ChangeKey(vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, lBegin, lEnd);
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{				
				_ChangeKey(vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, lBegin, lEnd);

				if(lBegin == lMid)
					lMid = lEnd;
				else if(lEnd == lMid)
					lMid = lBegin;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			QuickSort_V(m, lBegin - 1, vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, _CompareKey, _ChangeKey);
		if(lEnd < n)
			QuickSort_V(lEnd, n, vpKey1, vpKey2, vpKey3, vpKey4, vpKey5, _CompareKey, _ChangeKey);

		return 0;
	};

	/************************************************************************/
	/* 非稳定升序快排                                                       */
	/************************************************************************/
	var_4 QuickSort_A(var_4 lBegin, var_4 lEnd, T_Key* tKey, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QA_)
	{
		_QuickSort(lBegin, lEnd, tKey, _CompareKey);
		return 0;
	}
	var_4 QuickSort_A(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QA_)
	{
		_QuickSort(lBegin, lEnd, tKey, tAnnex, _CompareKey);
		return 0;
	}
	var_4 QuickSort_A(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QA_)
	{
		_QuickSort(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, _CompareKey);
		return 0;
	}
	var_4 QuickSort_A(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, T_Annex_3* tAnnex_Three, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QA_)
	{
		_QuickSort(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);
		return 0;
	}

	/************************************************************************/
	/* 非稳定降序快排                                                       */
	/************************************************************************/
	var_4 QuickSort_D(var_4 lBegin, var_4 lEnd, T_Key* tKey, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QD_)
	{
		_QuickSort(lBegin, lEnd, tKey, _CompareKey);
		return 0;
	}
	var_4 QuickSort_D(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QD_)
	{
		_QuickSort(lBegin, lEnd, tKey, tAnnex, _CompareKey);
		return 0;
	}
	var_4 QuickSort_D(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QD_)
	{
		_QuickSort(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, _CompareKey);
		return 0;
	}
	var_4 QuickSort_D(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, T_Annex_3* tAnnex_Three, var_4 (*_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_QD_)
	{
		_QuickSort(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);
		return 0;
	}

	/************************************************************************/
	/* 稳定升序快排                                                         */
	/************************************************************************/
	var_4 QuickSort_AS(var_4 lBegin, var_4 lEnd, T_Key* tKey, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QA_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, _CompareKey);
		return 0;
	}
	var_4 QuickSort_AS(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QA_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, tAnnex, _CompareKey);
		return 0;
	}
	var_4 QuickSort_AS(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QA_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, _CompareKey);
		return 0;
	}
	var_4 QuickSort_AS(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, T_Annex_3* tAnnex_Three, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QA_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);
		return 0;
	}

	/************************************************************************/
	/* 稳定降序快排                                                         */
	/************************************************************************/
	var_4 QuickSort_DS(var_4 lBegin, var_4 lEnd, T_Key* tKey, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QD_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, _CompareKey);
		return 0;
	}
	var_4 QuickSort_DS(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QD_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, tAnnex, _CompareKey);
		return 0;
	}
	var_4 QuickSort_DS(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QD_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, _CompareKey);
		return 0;
	}
	var_4 QuickSort_DS(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, T_Annex_3* tAnnex_Three, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4) = _DefaultCompareKey_QD_S_)
	{
		_QuickSort_S(lBegin, lEnd, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);
		return 0;
	}

private:
	var_4 _QuickSort(var_4 lBegin, var_4 lEnd, T_Key* tKey, var_4 (*_CompareKey)(T_Key, T_Key))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd]) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;
				
				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort(m, lBegin - 1, tKey, _CompareKey);
		if(lEnd < n)
			_QuickSort(lEnd, n, tKey, _CompareKey);

		return 0;
	};	
	var_4 _QuickSort(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, var_4 (*_CompareKey)(T_Key, T_Key))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		T_Annex_1 tA_TmpVal_1;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd]) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;
				
				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort(m, lBegin - 1, tKey, tAnnex_One, _CompareKey);
		if(lEnd < n)
			_QuickSort(lEnd, n, tKey, tAnnex_One, _CompareKey);

		return 0;
	};
	var_4 _QuickSort(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, var_4 (*_CompareKey)(T_Key, T_Key))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		T_Annex_1 tA_TmpVal_1;
		T_Annex_2 tA_TmpVal_2;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd]) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort(m, lBegin - 1, tKey, tAnnex_One, tAnnex_Two, _CompareKey);
		if(lEnd < n)
			_QuickSort(lEnd, n, tKey, tAnnex_One, tAnnex_Two, _CompareKey);

		return 0;
	};
	var_4 _QuickSort(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, T_Annex_3* tAnnex_Three, var_4 (*_CompareKey)(T_Key, T_Key))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		T_Annex_1 tA_TmpVal_1;
		T_Annex_2 tA_TmpVal_2;
		T_Annex_3 tA_TmpVal_3;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd]) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;

				tA_TmpVal_3 = tAnnex_Three[lBegin];
				tAnnex_Three[lBegin] = tAnnex_Three[lEnd];
				tAnnex_Three[lEnd] = tA_TmpVal_3;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;

				tA_TmpVal_3 = tAnnex_Three[lBegin];
				tAnnex_Three[lBegin] = tAnnex_Three[lEnd];
				tAnnex_Three[lEnd] = tA_TmpVal_3;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort(m, lBegin - 1, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);
		if(lEnd < n)
			_QuickSort(lEnd, n, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);

		return 0;
	};
	var_4 _QuickSort_S(var_4 lBegin, var_4 lEnd, T_Key* tKey, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd], lBegin, lEnd) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;
				
				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort_S(m, lBegin - 1, tKey, _CompareKey);
		if(lEnd < n)
			_QuickSort_S(lEnd, n, tKey, _CompareKey);

		return 0;
	};	
	var_4 _QuickSort_S(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		T_Annex_1 tA_TmpVal_1;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd], lBegin, lEnd) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;
				
				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort_S(m, lBegin - 1, tKey, tAnnex_One, _CompareKey);
		if(lEnd < n)
			_QuickSort_S(lEnd, n, tKey, tAnnex_One, _CompareKey);

		return 0;
	};
	var_4 _QuickSort_S(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		T_Annex_1 tA_TmpVal_1;
		T_Annex_2 tA_TmpVal_2;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd], lBegin, lEnd) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort_S(m, lBegin - 1, tKey, tAnnex_One, tAnnex_Two, _CompareKey);
		if(lEnd < n)
			_QuickSort_S(lEnd, n, tKey, tAnnex_One, tAnnex_Two, _CompareKey);

		return 0;
	};
	var_4 _QuickSort_S(var_4 lBegin, var_4 lEnd, T_Key* tKey, T_Annex_1* tAnnex_One, T_Annex_2* tAnnex_Two, T_Annex_3* tAnnex_Three, var_4 (*_CompareKey)(T_Key, T_Key, var_4, var_4))
	{
		if(lBegin >= lEnd)
			return 0;

		T_Key tK_TmpVal;
		T_Annex_1 tA_TmpVal_1;
		T_Annex_2 tA_TmpVal_2;
		T_Annex_3 tA_TmpVal_3;
		
		if(lBegin + 1 == lEnd)
		{
			if(_CompareKey(tKey[lBegin], tKey[lEnd], lBegin, lEnd) > 0)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;

				tA_TmpVal_3 = tAnnex_Three[lBegin];
				tAnnex_Three[lBegin] = tAnnex_Three[lEnd];
				tAnnex_Three[lEnd] = tA_TmpVal_3;			
			}
			return 0;
		}
		
		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		T_Key tK_MidVal = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && _CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0) lBegin++;
			while(lBegin < lEnd && _CompareKey(tKey[lEnd], tK_MidVal, lEnd, lMid) > 0) lEnd--;
			if(lBegin < lEnd)
			{
				tK_TmpVal = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tK_TmpVal;

				tA_TmpVal_1 = tAnnex_One[lBegin];
				tAnnex_One[lBegin] = tAnnex_One[lEnd];
				tAnnex_One[lEnd] = tA_TmpVal_1;

				tA_TmpVal_2 = tAnnex_Two[lBegin];
				tAnnex_Two[lBegin] = tAnnex_Two[lEnd];
				tAnnex_Two[lEnd] = tA_TmpVal_2;

				tA_TmpVal_3 = tAnnex_Three[lBegin];
				tAnnex_Three[lBegin] = tAnnex_Three[lEnd];
				tAnnex_Three[lEnd] = tA_TmpVal_3;			

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(_CompareKey(tKey[lBegin], tK_MidVal, lBegin, lMid) < 0)
			lBegin++;

		if(lBegin > m)
			_QuickSort_S(m, lBegin - 1, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);
		if(lEnd < n)
			_QuickSort_S(lEnd, n, tKey, tAnnex_One, tAnnex_Two, tAnnex_Three, _CompareKey);

		return 0;
	};
};

#endif // _UT_ARITHMETIC_H_

