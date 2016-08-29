// UT_AVLTree_2_OM.h

#ifndef _UT_AVL_TREE_2_OM_H_
#define _UT_AVL_TREE_2_OM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new.h>

#define max_val(a,b) (((a) > (b)) ? (a) : (b))
#define min_val(a,b) (((a) < (b)) ? (a) : (b))

enum TRAVELTYPE
{
	LMR, MLR, RML
};

template <class T_Key>
struct _Node_2_ 
{
	_Node_2_* cpLNode;
	_Node_2_* cpRNode;
	long lBFValue;

	T_Key tKey;
};

template<class T_Key>
inline long _DefaultCompareKey_2_(T_Key t1, T_Key t2)
{
	if(t1 > t2)
		return 1;
	if(t1 < t2)
		return -1;
	return 0;
};

template<class T_Key>
inline long _DefaultTravelNode_2_(_Node_2_<T_Key>* cpNode, void* vpArg)
{
	return 0;
};

template<class T_Key>
inline long _DefaultSaveNode_2_(_Node_2_<T_Key>* cpNode, FILE* fp)
{
	fwrite(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
};

template<class T_Key>
inline long _DefaultLoadNode_2_(_Node_2_<T_Key>* cpNode, FILE* fp, void* vpArg)
{
	fread(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
};

template <class T_Key, class T_NodeType = _Node_2_<T_Key> >
class UT_AVLTree_2_OM
{
public:
	UT_AVLTree_2_OM(char* szpAllocator, long lAllocatorSize)
	{
		m_lKeyNum = 0;
		m_lHightFlag = -1;
		m_cpClassRoot = NULL;

		m_lAllocatorSize = lAllocatorSize;

		m_szpAllocator = szpAllocator;
		m_szpAllocatorBase = szpAllocator;
	};

	~UT_AVLTree_2_OM()
	{
		return;
	};
	
	T_NodeType* AddKey(T_Key tKey, long& lIsNewAdd, long (__cdecl* _CompareKey)(T_Key, T_Key) = _DefaultCompareKey_2_)
	{
		lIsNewAdd = 0;

		_Node_2_<T_Key>* cpRetNode = NULL; 
		_Node_2_<T_Key>* cpTmpNode = _AddKey(m_cpClassRoot, tKey, lIsNewAdd, cpRetNode, _CompareKey);
		if(cpRetNode)
			m_cpClassRoot = cpTmpNode;

		return (T_NodeType*)cpRetNode;
	};

	T_NodeType* SearchKey(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key) = _DefaultCompareKey_2_)	
	{
		return (T_NodeType*)_SearchKey(m_cpClassRoot, tKey, _CompareKey);
	};

	long SaveTree(char* szpSaveFilename, long (__cdecl* _SaveNode)(T_NodeType*, FILE*) = _DefaultSaveNode_2_)
	{
		long lRetVal = -1;

		FILE* fpSave = fopen(szpSaveFilename, "wb");
		if(fpSave == NULL)
			return lRetVal;

		_Node_2_<T_Key>** cppStack = new _Node_2_<T_Key>*[1<<20];
		if(_SaveTree(fpSave, cppStack, _SaveNode) == NULL)
			lRetVal = 0;
		delete cppStack;

		fclose(fpSave);
		
		return lRetVal;
	};

	long LoadTree(char* szpLoadFilename, long (__cdecl* _LoadNode)(T_NodeType*, FILE*, void*) = _DefaultLoadNode_2_, void* vpArg = NULL)
	{
		long lRetVal = -1;

		FILE* fpLoad = fopen(szpLoadFilename, "rb");
		if(fpLoad == NULL)
			return lRetVal;

		_Node_2_<T_Key>** cppStack = new _Node_2_<T_Key>*[1<<20];
		long* lpStack = new long[1<<20];
		m_cpClassRoot = _LoadTree(fpLoad, cppStack, lpStack, _LoadNode, vpArg);
		if(m_cpClassRoot)
			lRetVal = 0;
		delete lpStack;
		delete cppStack;

		fclose(fpLoad);
		
		return lRetVal;	
	};

	long GetKeyNum()
	{
		return m_lKeyNum;
	};

	long ResetTree()
	{
		m_lKeyNum = 0;
		m_lHightFlag = -1;
		m_cpClassRoot = NULL;

		m_szpAllocator = m_szpAllocatorBase;

		return 0;
	};

	long TravelTree(TRAVELTYPE eTravelType = LMR, long (__cdecl* _TravelNode)(T_NodeType*, void*) = _DefaultTravelNode_2_, void* vpArg = NULL)
	{
		if(eTravelType == LMR)
			return _TravelTree_LMR(m_cpClassRoot, _TravelNode, vpArg);
		if(eTravelType == MLR)
			return _TravelTree_MLR(m_cpClassRoot, _TravelNode, vpArg);
		if(eTravelType == RML)
			return _TravelTree_RML(m_cpClassRoot, _TravelNode, vpArg);
		return 0;
	};

	// 得到当前版本号
	char* GetVersion()
	{
		// 初始版本 - v1.000 - 2008.08.27
		return "v1.000";
	}

private:
	_Node_2_<T_Key>* _AddKey(_Node_2_<T_Key>* t, T_Key tKey, long& lIsNewAdd, _Node_2_<T_Key>*& cprFindNode, long (__cdecl* _CompareKey)(T_Key, T_Key))
	{
		if(t == NULL)
		{
			cprFindNode = NULL;
			if(m_szpAllocator - m_szpAllocatorBase + sizeof(T_NodeType) > (unsigned)m_lAllocatorSize)
				return NULL;
			t = (_Node_2_<T_Key>*)(new(m_szpAllocator) T_NodeType);
			m_szpAllocator += sizeof(T_NodeType);
			cprFindNode = t;
			t->cpLNode = NULL;
			t->cpRNode = NULL;
			t->lBFValue = 0;
			t->tKey = tKey;
			m_lHightFlag = 1;
			m_lKeyNum++;
			lIsNewAdd = 1;
			return t;
		}
		
		long lCompareFlag = _CompareKey(tKey, t->tKey);
		if(lCompareFlag == 0) 
		{
			cprFindNode = t;
			m_lHightFlag = 0;
		}
		else if(lCompareFlag > 0)
		{
			_Node_2_<T_Key>* cpTmpNode = _AddKey(t->cpRNode, tKey, lIsNewAdd, cprFindNode, _CompareKey);
			if(cpTmpNode == NULL)
				return NULL;
			t->cpRNode = cpTmpNode;
			t->lBFValue += m_lHightFlag;
		}
		else // if(lCompareFlag < 0)
		{
			_Node_2_<T_Key>* cpTmpNode = _AddKey(t->cpLNode, tKey, lIsNewAdd, cprFindNode, _CompareKey);			
			if(cpTmpNode == NULL)
				return NULL;
			t->cpLNode = cpTmpNode;
			t->lBFValue -= m_lHightFlag;		
		}
		
		if(m_lHightFlag == 0)
			return t;
		
		if(t->lBFValue < -1) 
		{
			if(t->cpLNode->lBFValue > 0)
				t->cpLNode = _LRot(t->cpLNode);
			t = _RRot(t);
			m_lHightFlag = 0;
		}
		else if(t->lBFValue > 1) 
		{	
			if(t->cpRNode->lBFValue < 0)
				t->cpRNode = _RRot(t->cpRNode);
			t = _LRot(t);
			m_lHightFlag = 0;
		}
		else if(t->lBFValue == 0)
			m_lHightFlag = 0;
		else
			m_lHightFlag = 1;

		return t;
	};

	_Node_2_<T_Key>* _SearchKey(_Node_2_<T_Key>* t, T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key))
	{
		if(t == NULL)
			return NULL;
		
		long lCompareFlag = _CompareKey(tKey, t->tKey);
		if(lCompareFlag == 0)
			return t;
		else if(lCompareFlag > 0)
			return _SearchKey(t->cpRNode, tKey, _CompareKey);
		else // if(lCompareFlag < 0)
			return _SearchKey(t->cpLNode, tKey, _CompareKey);

		return NULL;
	};

	_Node_2_<T_Key>* _LRot(_Node_2_<T_Key>* t)
	{
		_Node_2_<T_Key>* temp;
		long x,y;	
		temp = t;
		t = t->cpRNode;
		temp->cpRNode = t->cpLNode;
		t->cpLNode = temp;
		x = temp->lBFValue;
		y = t->lBFValue;
		temp->lBFValue = x-1-max_val(y, 0);
		t->lBFValue = min_val(x-2+min_val(y, 0), y-1);
		return t;
	};

	_Node_2_<T_Key>* _RRot(_Node_2_<T_Key>* t)
	{
		_Node_2_<T_Key>* temp;
		long x,y;	
		temp = t;
		t = t->cpLNode;
		temp->cpLNode = t->cpRNode;
		t->cpRNode = temp;
		x = temp->lBFValue;
		y = t->lBFValue;
		temp->lBFValue = x+1-min_val(y, 0);
		t->lBFValue = max_val(x+2+max_val(y, 0), y+1);
		return t;
	};

	_Node_2_<T_Key>* _SaveTree(FILE* fpSave, _Node_2_<T_Key>** cppStack, long (__cdecl* _SaveNode)(T_NodeType*, FILE*))
	{
		long lStackPos = 0;
		long *lpTmpList = NULL;
		_Node_2_<T_Key>* cpTmpNode = NULL;
		_Node_2_<T_Key>* cpErrNode = (_Node_2_<T_Key>*)0xCCCCCCCC;

		if(m_cpClassRoot == NULL)
		{
			if(fwrite(&lStackPos, 4, 1, fpSave) != 1) 
				return cpErrNode;
			return NULL;
		}

		cppStack[lStackPos++] = m_cpClassRoot;
		while(lStackPos > 0)
		{
			cpTmpNode = cppStack[--lStackPos];
			if(fwrite(&cpTmpNode, 4, 1, fpSave) != 1) 
				return cpErrNode;
			if(fwrite(&cpTmpNode->lBFValue, 4, 1, fpSave) != 1) 
				return cpErrNode;
			_SaveNode((T_NodeType*)cpTmpNode, fpSave);		
			if(fwrite(&cpTmpNode->cpLNode, 4, 1, fpSave) != 1) 
				return cpErrNode;
			if(fwrite(&cpTmpNode->cpRNode, 4, 1, fpSave) != 1)
				return cpErrNode;

			if(cpTmpNode->cpRNode)
				cppStack[lStackPos++] = cpTmpNode->cpRNode;
			if(cpTmpNode->cpLNode)
				cppStack[lStackPos++] = cpTmpNode->cpLNode;
		}

		if(fwrite(&cpErrNode, 4, 1, fpSave) != 1) 
			return cpErrNode;

		return NULL;
	};

	_Node_2_<T_Key>* _LoadTree(FILE* fpLoad, _Node_2_<T_Key>** cppStack, long* lpStack, long (__cdecl* _LoadNode)(T_NodeType*, FILE*, void*), void* vpArg)
	{
		long i = 0;
		long lFlag = 1;
		long lStackTop = 0;
		long lTmpVal = 0;
		long lRetVal = -1;
		long* lpTmpBuf = NULL;
		_Node_2_<T_Key>* cpHeadNode = NULL;
		_Node_2_<T_Key>* cpNewNode = NULL;

		if(fread(&lTmpVal, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(lTmpVal == NULL)	return NULL;

		if(m_szpAllocator - m_szpAllocatorBase + sizeof(T_NodeType) > (unsigned)m_lAllocatorSize)
			goto ERROREND

		cpHeadNode = (_Node_2_<T_Key>*)(new(m_szpAllocator) T_NodeType);
		m_szpAllocator += sizeof(T_NodeType);

		if(fread(&cpHeadNode->lBFValue, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		_LoadNode((T_NodeType*)cpHeadNode, fpLoad, vpArg);	
		if(fread(&cpHeadNode->cpLNode, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(fread(&cpHeadNode->cpRNode, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		cppStack[lStackTop] = cpHeadNode;
		lpStack[lStackTop++] = 1;

		while(1)
		{
			if(fread(&lTmpVal, 4, 1, fpLoad) != 1) 
				goto ERROREND;
			if(lTmpVal == 0xCCCCCCCC) 
				break;

			if(m_szpAllocator - m_szpAllocatorBase + sizeof(T_NodeType) > (unsigned)m_lAllocatorSize)
				goto ERROREND
			cpNewNode = (_Node_2_<T_Key>*)(new(m_szpAllocator) T_NodeType);
			m_szpAllocator += sizeof(T_NodeType);

			if(fread(&cpNewNode->lBFValue, 4, 1, fpLoad) != 1) 
				goto ERROREND;
			_LoadNode((T_NodeType*)cpNewNode, fpLoad, vpArg);
			if(fread(&cpNewNode->cpLNode, 4, 1, fpLoad) != 1) 
				goto ERROREND;
			if(fread(&cpNewNode->cpRNode, 4, 1, fpLoad) != 1) 
				goto ERROREND;

	NEXTNODE:
			if(lFlag == 1 && cppStack[lStackTop - 1]->cpLNode)
			{			
				cppStack[lStackTop - 1]->cpLNode = cpNewNode;			
				cppStack[lStackTop] = cpNewNode;
				lpStack[lStackTop++] = 2;
				continue;
			}
			else if(lFlag == 1)
				lFlag = 2;
			if(lFlag == 2 && cppStack[lStackTop - 1]->cpRNode)
			{
				lFlag = 1;
				cppStack[lStackTop - 1]->cpRNode = cpNewNode;			
				cppStack[lStackTop] = cpNewNode;
				lpStack[lStackTop++] = 3;
				continue;
			}

			lFlag = lpStack[--lStackTop];
			goto NEXTNODE;		
		}
		lRetVal = 0;

	ERROREND:
		if(lRetVal == 0) return cpHeadNode;
		return NULL;
	};

	long _TravelTree_LMR(_Node_2_<T_Key>* t, long (__cdecl* _TravelNode)(T_NodeType*, void*), void* vpArg)
	{
		if(t == NULL)
			return 0;
		if(_TravelTree_LMR(t->cpLNode, _TravelNode, vpArg))
			return -1;
		if(_TravelNode((T_NodeType*)t, vpArg))
			return -1;
		if(_TravelTree_LMR(t->cpRNode, _TravelNode, vpArg))
			return -1;
		return 0;
	};
	
	long _TravelTree_MLR(_Node_2_<T_Key>* t, long (__cdecl* _TravelNode)(T_NodeType*, void*), void* vpArg)
	{
		if(t == NULL)
			return 0;
		if(_TravelNode((T_NodeType*)t, vpArg))
			return -1;
		if(_TravelTree_MLR(t->cpLNode, _TravelNode, vpArg))
			return -1;
		if(_TravelTree_MLR(t->cpRNode, _TravelNode, vpArg))
			return -1;
		return 0;
	};

	long _TravelTree_RML(_Node_2_<T_Key>* t, long (__cdecl* _TravelNode)(T_NodeType*, void*), void* vpArg)
	{
		if(t == NULL)
			return 0;
		if(_TravelTree_RML(t->cpRNode, _TravelNode, vpArg))
			return -1;
		if(_TravelNode((T_NodeType*)t, vpArg))
			return -1;
		if(_TravelTree_RML(t->cpLNode, _TravelNode, vpArg))
			return -1;
		return 0;
	};

public:
	long m_lKeyNum;

	long m_lHightFlag;
	_Node_2_<T_Key>* m_cpClassRoot;

	long m_lAllocatorSize;

	char* m_szpAllocator;
	char* m_szpAllocatorBase;	
};

#endif // _UT_AVL_TREE_2_OM_H_
