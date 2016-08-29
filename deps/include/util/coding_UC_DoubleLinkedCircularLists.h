// UC_DoubleLinkedCircularLists.h

#ifndef _UC_DOUBLE_LINKED_CIRCULAR_LISTS_H_
#define _UC_DOUBLE_LINKED_CIRCULAR_LISTS_H_

#include "../Code_Library/platform_cross/UH_Define.h"

template <class T_Key>
struct _Node_2_ 
{
	_Node_2_* cpLNode;
	_Node_2_* cpRNode;
	var_4 lBFValue;

	T_Key tKey;
};

template<class T_Key>
inline var_4 _DefaultCompareKey_2_(T_Key t1, T_Key t2)
{
	if(t1 > t2)
		return 1;
	if(t1 < t2)
		return -1;

	return 0;
};

template<class T_Key>
inline var_4 _DefaultTravelNode_2_(_Node_2_<T_Key>* cpNode, void* vpArg)
{
	return 0;
};

template<class T_Key>
inline var_4 _DefaultSaveNode_2_(_Node_2_<T_Key>* cpNode, FILE* fp)
{
	fwrite(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
};

template<class T_Key>
inline var_4 _DefaultLoadNode_2_(_Node_2_<T_Key>* cpNode, FILE* fp, void* vpArg)
{
	fread(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
};

template <class T_Key, class T_NodeType = _Node_2_<T_Key> >
class UC_DoubleLinkedCircularLists
{
public:
	UT_AVLTree_2_IM()
	{
		m_lKeyNum = 0;
		m_lHightFlag = -1;
		m_cpClassRoot = NULL;
	};

	~UT_AVLTree_2_IM()
	{
		return;
	};
	
	T_NodeType* AddKey(T_Key tKey, var_4& lIsNewAdd, var_4 (_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_2_)
	{
		lIsNewAdd = 0;

		_Node_2_<T_Key>* cpNode = NULL;
		m_cpClassRoot = (T_NodeType*)_AddKey(m_cpClassRoot, tKey, lIsNewAdd, cpNode, _CompareKey);

		return (T_NodeType*)cpNode;
	}

	T_NodeType* SearchKey(T_Key tKey, var_4 (_CompareKey)(T_Key, T_Key) = _DefaultCompareKey_2_)	
	{
		return (T_NodeType*)_SearchKey(m_cpClassRoot, tKey, _CompareKey);
	};

	var_4 SaveTree(char* szpSaveFilename, var_4 (_SaveNode)(T_NodeType*, FILE*) = _DefaultSaveNode_2_)
	{
		var_4 lRetVal = -1;

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

	var_4 LoadTree(char* szpLoadFilename, var_4 (_LoadNode)(T_NodeType*, FILE*, void*) = _DefaultLoadNode_2_, void* vpArg = NULL)
	{
		var_4 lRetVal = -1;

		FILE* fpLoad = fopen(szpLoadFilename, "rb");
		if(fpLoad == NULL)
			return lRetVal;

		_Node_2_<T_Key>** cppStack = new _Node_2_<T_Key>*[1<<20];
		var_4* lpStack = new var_4[1<<20];
		m_cpClassRoot = _LoadTree(fpLoad, cppStack, lpStack, _LoadNode, vpArg);
		if(m_cpClassRoot)
			lRetVal = 0;
		delete lpStack;
		delete cppStack;

		fclose(fpLoad);
		
		return lRetVal;	
	};

	var_4 GetKeyNum()
	{
		return m_lKeyNum;
	};

	var_4 ResetTree(var_4 lFlag = 0)
	{
		m_lKeyNum = 0;
		m_lHightFlag = -1;
		m_cpClassRoot = NULL;

		if(lFlag == 0)
		        m_tAllocator.ResetAllocator();
		else
			m_tAllocator.FreeAllocator();
		
		return 0;
	};

	var_4 TravelTree(TRAVELTYPE eTravelType = LMR, var_4 (_TravelNode)(T_NodeType*, void*) = _DefaultTravelNode_2_, void* vpArg = NULL)
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
	_Node_2_<T_Key>* _AddKey(_Node_2_<T_Key>* t, T_Key tKey, var_4& lIsNewAdd, _Node_2_<T_Key>*& cprFindNode, var_4 (_CompareKey)(T_Key, T_Key))
	{
		if(t == NULL)
		{
			cprFindNode = NULL;
			t = m_tAllocator.Allocate();
			if(t == NULL)
				return NULL;
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
		
		var_4 lCompareFlag = _CompareKey(tKey, t->tKey);
		if(lCompareFlag == 0) 
		{
			cprFindNode = t;
			m_lHightFlag = 0;
		}
		else if(lCompareFlag > 0)
		{
			t->cpRNode = _AddKey(t->cpRNode, tKey, lIsNewAdd, cprFindNode, _CompareKey);		
			t->lBFValue += m_lHightFlag;
		}
		else // if(lCompareFlag < 0)
		{
			t->cpLNode = _AddKey(t->cpLNode, tKey, lIsNewAdd, cprFindNode, _CompareKey);
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

	_Node_2_<T_Key>* _SearchKey(_Node_2_<T_Key>* t, T_Key tKey, var_4 (_CompareKey)(T_Key, T_Key))
	{
		if(t == NULL)
			return NULL;
		
		var_4 lCompareFlag = _CompareKey(tKey, t->tKey);
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
		var_4 x,y;	
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
		var_4 x,y;	
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

	_Node_2_<T_Key>* _SaveTree(FILE* fpSave, _Node_2_<T_Key>** cppStack, var_4 (_SaveNode)(T_NodeType*, FILE*))
	{
		var_4 lStackPos = 0;
		var_4 *lpTmpList = NULL;
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

	_Node_2_<T_Key>* _LoadTree(FILE* fpLoad, _Node_2_<T_Key>** cppStack, var_4* lpStack, var_4 (_LoadNode)(T_NodeType*, FILE*, void*), void* vpArg)
	{
		var_4 i = 0;
		var_4 lFlag = 1;
		var_4 lStackTop = 0;
		var_4 lTmpVal = 0;
		var_4 lRetVal = -1;
		var_4* lpTmpBuf = NULL;
		_Node_2_<T_Key>* cpHeadNode = NULL;
		_Node_2_<T_Key>* cpNewNode = NULL;

		if(fread(&lTmpVal, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(lTmpVal == NULL)	return NULL;

		cpHeadNode = m_tAllocator.Allocate();
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
			if(lTmpVal == 0xCCCCCCCC) break;

			cpNewNode = m_tAllocator.Allocate();
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
		if(lRetVal == 0) 
			return cpHeadNode;

		return NULL;
	};

	var_4 _TravelTree_LMR(_Node_2_<T_Key>* t, var_4 (_TravelNode)(T_NodeType*, void*), void* vpArg)
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
	
	var_4 _TravelTree_MLR(_Node_2_<T_Key>* t, var_4 (_TravelNode)(T_NodeType*, void*), void* vpArg)
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

	var_4 _TravelTree_RML(_Node_2_<T_Key>* t, var_4 (_TravelNode)(T_NodeType*, void*), void* vpArg)
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
	var_4 m_lKeyNum;

	var_4 m_lHightFlag;
	_Node_2_<T_Key>* m_cpClassRoot;

	UT_Allocator<T_NodeType> m_tAllocator;
};

#endif // _UC_DOUBLE_LINKED_CIRCULAR_LISTS_H_
