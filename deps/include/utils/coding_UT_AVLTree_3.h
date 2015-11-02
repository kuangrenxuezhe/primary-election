// U_AVLTree_3.h

#ifndef _UNIVERSAL_AVL_TREE_3_H_
#define _UNIVERSAL_AVL_TREE_3_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UT_Allocator.h"

#define max_val(a,b) (((a) > (b)) ? (a) : (b))
#define min_val(a,b) (((a) < (b)) ? (a) : (b))

template <class T_Key>
struct _Node_3_ {
	_Node_3_* cpLNode;
	_Node_3_* cpRNode;
	_Node_3_* cpMNode;
	long lBFValue;
	
	T_Key tKey;
};

template<class T_Key>
inline long _DefaultCompareKey(T_Key t1, T_Key t2)
{
	return t1 - t2;
}

template<class T_Key>
inline long _DefaultSaveNode(_Node_3_<T_Key>* cpNode, FILE* fp)
{
	fwrite(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
}

template<class T_Key>
inline long _DefaultLoadNode(_Node_3_<T_Key>* cpNode, FILE* fp, void* vpArg)
{
	fread(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
}

template <class T_Key, class T_NodeType = _Node_3_<T_Key> >
class U_AVLTree_3
{
public:
	U_AVLTree_3();
	~U_AVLTree_3();

	T_NodeType* AddKey(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key) = _DefaultCompareKey);
	T_NodeType* SearchKey(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key) = _DefaultCompareKey);

	long SaveTree(char* szpSaveFilename, long (__cdecl* _SaveNode)(T_NodeType*, FILE*) = _DefaultSaveNode);
	long LoadTree(char* szpLoadFilename, long (__cdecl* _LoadNode)(T_NodeType*, FILE*, void*) = _DefaultLoadNode, void* vpArg = NULL);

private:
	_Node_3_<T_Key>* _SearchKey(_Node_3_<T_Key>* t, T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key));
	_Node_3_<T_Key>* _AddKey(_Node_3_<T_Key>* t, T_Key tKey, _Node_3_<T_Key>*& cprFindNode, long (__cdecl* _CompareKey)(T_Key, T_Key));

	_Node_3_<T_Key>* _SaveTree(FILE* fpSave, _Node_3_<T_Key>** cppStack, long (__cdecl* _SaveNode)(T_NodeType*, FILE*));	
	_Node_3_<T_Key>* _LoadTree(FILE* fpLoad, _Node_3_<T_Key>** cppStack, long* lpStack, long (__cdecl* _LoadNode)(T_NodeType*, FILE*, void*), void* vpArg = NULL);

	inline _Node_3_<T_Key>* _LRot(_Node_3_<T_Key>* t);
	inline _Node_3_<T_Key>* _RRot(_Node_3_<T_Key>* t);	

	long m_lHightFlag;
	_Node_3_<T_Key>* m_cpClassRoot;

	U_Allocator<T_NodeType> m_tpAllocator;
};

/************************************************************************/
/*                       U_AVLTree_3 Implement                          */
/************************************************************************/
template <class T_Key, class T_NodeType>
U_AVLTree_3<T_Key, T_NodeType>::U_AVLTree_3()
{
	m_lHightFlag = -1;
	m_cpClassRoot = NULL;
}

template <class T_Key, class T_NodeType>
U_AVLTree_3<T_Key, T_NodeType>::~U_AVLTree_3()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T_Key, class T_NodeType>
T_NodeType* U_AVLTree_3<T_Key, T_NodeType>::AddKey(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key))
{
	_Node_3_<T_Key>* cpNode = NULL;
	m_cpClassRoot = (T_NodeType*)_AddKey(m_cpClassRoot, tKey, cpNode, _CompareKey);

	return (T_NodeType*)cpNode;
}

template <class T_Key, class T_NodeType>
_Node_3_<T_Key>* U_AVLTree_3<T_Key, T_NodeType>::_AddKey(_Node_3_<T_Key>* t, T_Key tKey, _Node_3_<T_Key>*& cprFindNode, long (__cdecl* _CompareKey)(T_Key, T_Key))
{
	if(t == NULL)
	{
		cprFindNode = NULL;
		t = m_tpAllocator.Allocate();
		if(t == NULL)
			return NULL;
		cprFindNode = t;
		t->cpLNode = NULL;
		t->cpRNode = NULL;
		t->lBFValue = 0;
		t->tKey = tKey;
		m_lHightFlag = 1;
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
		t->cpRNode = _AddKey(t->cpRNode, tKey, cprFindNode, _CompareKey);		
		t->lBFValue += m_lHightFlag;
	}
	else // if(lCompareFlag < 0)
	{
		t->cpLNode = _AddKey(t->cpLNode, tKey, cprFindNode, _CompareKey);
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
}

template <class T_Key, class T_NodeType>
T_NodeType* U_AVLTree_3<T_Key, T_NodeType>::SearchKey(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key))
{
	return (T_NodeType*)_SearchKey(m_cpClassRoot, tKey, _CompareKey);
}

template <class T_Key, class T_NodeType>
_Node_3_<T_Key>* U_AVLTree_3<T_Key, T_NodeType>::_SearchKey(_Node_3_<T_Key>* t, T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key))
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
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
long UniversalDomain::AddDomain_Count(char* szpClass, long lPage)
{
	if(szpClass == NULL)
		return -1;

	if(strlen(szpClass) < 8)
		return -1;

	if(strnicmp(szpClass, "http://", 7))
		return -1;
	
	char* p = szpClass + 7;	
	while(*p && *p != '/')
		p++;
	if(*p)
		*p = 0;

	p = szpClass + 6;
	*p = '.';

	char *p1, *p2;
	DomainNode *cpFindNode = NULL, *cpCurNode = m_cpClassRoot, *cpPreNode = NULL;	

	while(p1 = strrchr(p, '.'))
	{
		*p1++ = 0;
		p2 = p1;
		while(*p2)
		{
			if(*p2 < 0 && *(p2 + 1))
			{
				p2 += 2;
				continue;
			}
			if(*p2 >= 'a' && *p2 <= 'z')
				*p2 -= 32;
			p2++;
		}
		
		if(cpPreNode == NULL)
			cpCurNode = _AddDomain_Count(cpCurNode, p1, cpFindNode);
		else
			cpCurNode = _AddDomain_Count(cpCurNode->cpChildNode, p1, cpFindNode);
				
		cpFindNode->lAllPageNum++;

		if(cpPreNode == NULL)
			m_cpClassRoot = cpCurNode;
		else
			cpPreNode->cpChildNode = cpCurNode;
		cpCurNode = cpFindNode;
		cpPreNode = cpFindNode;
	}
	
	cpFindNode->lPageNum++;

	m_lAllPageNum++;

	return 0;
}

DomainNode* UniversalDomain::_AddDomain_Count(DomainNode* t, char* szpClass, DomainNode*& cprFindNode)
{
	if(t == NULL)
	{
		t = _AllocNode();
		if(t == NULL)
			return NULL;
		cprFindNode = t;
		t->lBFValue = 0;
		t->lClassNameLen = strlen(szpClass);
		t->szpClassName = _AllocMem(t->lClassNameLen + 1);
		if(t->szpClassName == NULL)
			return NULL;
		memcpy(t->szpClassName, szpClass, t->lClassNameLen);
		t->szpClassName[t->lClassNameLen] = 0;
		t->lClassNameLen++;
		m_lHightFlag = 1;
		return t;
	}
	
	long lCompareResult = strcmp(szpClass, t->szpClassName);
	if(lCompareResult == 0) 
	{
		cprFindNode = t;
		m_lHightFlag = 0;
	}
	else if(lCompareResult > 0)
	{
		t->cpRNode = _AddDomain_Count(t->cpRNode, szpClass, cprFindNode);		
		t->lBFValue += m_lHightFlag;
	}
	else // if(lCompareResult < 0)
	{
		t->cpLNode = _AddDomain_Count(t->cpLNode, szpClass, cprFindNode);
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
}

long UniversalDomain::AddDomain_Alloc()
{
	if(m_lAllPageNum <= 0)
		return -1;

	m_lpPageBuf = new long[m_lAllPageNum];
	if(m_lpPageBuf == NULL)
		return -1;

	long lAllocPos = 0;
	if(_AddDomain_Alloc(m_cpClassRoot, lAllocPos))
		return -1;
	
	return 0;
}

long UniversalDomain::_AddDomain_Alloc(DomainNode* t, long& lrAllocPos)
{
	if(t == NULL)
		return 0;

	t->lBFValue = 0;
	t->lOffSet = lrAllocPos;
	
	if(t->lPageNum && t->cpChildNode != NULL)
		lrAllocPos += t->lPageNum;

	if(t->cpChildNode == NULL)
		lrAllocPos += t->lPageNum;
	
	_AddDomain_Alloc(t->cpChildNode, lrAllocPos);
	_AddDomain_Alloc(t->cpLNode, lrAllocPos);
	_AddDomain_Alloc(t->cpRNode, lrAllocPos);
	
	return 0;
}

long UniversalDomain::AddDomain_Add(char* szpClass, long lPage)
{
	if(szpClass == NULL)
		return -1;

	if(strlen(szpClass) < 8)
		return -1;

	if(strnicmp(szpClass, "http://", 7))
		return -1;
	
	char* p = szpClass + 7;	
	while(*p && *p != '/')
		p++;	
	if(*p)
		*p = 0;

	p = szpClass + 6;
	*p = '.';

	char *p1, *p2;
	DomainNode *cpTempNode = m_cpClassRoot, *cpFindNode = NULL;

	while(p1 = strrchr(p, '.'))
	{
		*p1++ = 0;
		p2 = p1;
		while(*p2)
		{
			if(*p2 < 0 && *(p2 + 1))
			{
				p2 += 2;
				continue;
			}
			if(*p2 >= 'a' && *p2 <= 'z')
				*p2 -= 32;
			p2++;
		}

		cpFindNode = _SearchDomain(cpTempNode, p1);
		if(cpFindNode == NULL)
			break;
		cpTempNode = cpFindNode->cpChildNode;	
	}
	
	if(cpFindNode == NULL)
		return -1;

	m_lpPageBuf[cpFindNode->lOffSet + cpFindNode->lBFValue++] = lPage;

	return 0;
}

long UniversalDomain::SearchDomain(char* szpClass, long*& lprPage)
{
	lprPage = NULL;

	if(szpClass == NULL)
		return -1;

	char *p = szpClass;
	if(strnicmp(p, "http://", 7) == 0)
		p += 7;
		
	char* p1 = p;
	while(*p1 && *p1 != '/')
		p1++;	
	if(*p1)
		*p1 = 0;
	
	char* p2 = p1;
	DomainNode *cpTempNode = m_cpClassRoot, *cpFindNode = NULL;
	while((p1 = strrchr(p, '.')) || p2 - p > 0)
	{
		if(p1 == NULL)
		{
			p1 = p;
			p2 = p;
		}
		else
		{
			*p1++ = 0;
			p2 = p1;
		}
		while(*p2)
		{
			if(*p2 < 0 && *(p2 + 1))
			{
				p2 += 2;
				continue;
			}
			if(*p2 >= 'a' && *p2 <= 'z')
				*p2 -= 32;
			p2++;
		}
		p2 = p1;
		
		cpFindNode = _SearchDomain(cpTempNode, p1);
		if(cpFindNode == NULL)
			break;
		cpTempNode = cpFindNode->cpChildNode;		
	}

	if(cpFindNode == NULL)
		return -1;

	lprPage = m_lpPageBuf + cpFindNode->lOffSet;

	return cpFindNode->lAllPageNum;
}

DomainNode* UniversalDomain::_SearchDomain(DomainNode* cpNode, char* szpClass)
{
	if(cpNode == NULL)
		return NULL;

	long lCompareResult = strcmp(szpClass, cpNode->szpClassName);
	if(lCompareResult == 0)
		return cpNode;
	else if(lCompareResult > 0)
		return _SearchDomain(cpNode->cpRNode, szpClass);
	else // if(lCompareResult < 0)
		return _SearchDomain(cpNode->cpLNode, szpClass);

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char* UniversalDomain::_AllocMem(long lAllocLen)
{
	if(lAllocLen > (BUFLIBLEN - m_lBufLibLen))
	{
		if(++m_lBufLibPos >= BUFLIBNUM)
			return NULL;
		m_szpaBufLib[m_lBufLibPos] = new char[BUFLIBLEN];
		m_lBufLibLen = 0;
	}
	
	char* p = &m_szpaBufLib[m_lBufLibPos][m_lBufLibLen];
	m_lBufLibLen += lAllocLen;
	
	return p;
}

DomainNode* UniversalDomain::_AllocNode()
{
	if(m_lNodeLibLen == NODELIBLEN)
	{
		if(++m_lNodeLibPos >= NODELIBNUM)
			return NULL;
		m_cpaNodeLib[m_lNodeLibPos] = new DomainNode[NODELIBLEN];
		m_lNodeLibLen = 0;
	}
	
	return &m_cpaNodeLib[m_lNodeLibPos][m_lNodeLibLen++];
}

inline DomainNode* UniversalDomain::_LRot(DomainNode* t)
{
	DomainNode* temp;
	long x,y;	
	temp = t;
	t = t->cpRNode;
	temp->cpRNode = t->cpLNode;  
	t->cpLNode = temp;
	x = temp->lBFValue;
	y = t->lBFValue;
	temp->lBFValue = x-1-max(y, 0);
	t->lBFValue = min(x-2+min(y, 0), y-1);
	return t;
}

inline DomainNode* UniversalDomain::_RRot(DomainNode* t)
{
	DomainNode* temp;
	long x,y;	
	temp = t;
	t = t->cpLNode;
	temp->cpLNode = t->cpRNode;
	t->cpRNode = temp;
	x = temp->lBFValue;
	y = t->lBFValue;
	temp->lBFValue = x+1-min(y, 0);
	t->lBFValue = max(x+2+max(y, 0), y+1);
	return t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
long UniversalDomain::SaveDomain(char* szpSaveFilename)
{
	long lRetVal = -1;

	FILE* fpSave = fopen(szpSaveFilename, "wb");
	if(fpSave == NULL)
		return lRetVal;

	fwrite(&m_lAllPageNum, 4, 1, fpSave);
	fwrite(m_lpPageBuf, m_lAllPageNum<<2, 1, fpSave);
	
	DomainNode** cppStack = new DomainNode*[100000];
	if(_SaveTree(fpSave, cppStack) == NULL)
		lRetVal = 0;
	delete cppStack;

	fclose(fpSave);
	
	return lRetVal;
}

DomainNode* UniversalDomain::_SaveTree(FILE* fpSave, DomainNode** cppStack)
{
	DomainNode* cpTmpNode = NULL;	
	long lStackPos = 0;
	long *lpTmpList = NULL;
	DomainNode* cpErrNode = (DomainNode*)0xCCCCCCCC;

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
		if(fwrite(&cpTmpNode->lClassNameLen, 4, 1, fpSave) != 1) 
			return cpErrNode;
		if(fwrite(cpTmpNode->szpClassName, cpTmpNode->lClassNameLen, 1, fpSave) != 1) 
			return cpErrNode;
		if(fwrite(&cpTmpNode->lAllPageNum, 4, 1, fpSave) != 1) 
			return cpErrNode;
		if(fwrite(&cpTmpNode->lPageNum, 4, 1, fpSave) != 1) 
			return cpErrNode;
		if(fwrite(&cpTmpNode->lOffSet, 4, 1, fpSave) != 1)
			return cpErrNode;
		
		if(fwrite(&cpTmpNode->cpChildNode, 4, 1, fpSave) != 1) 
			return cpErrNode;
		if(fwrite(&cpTmpNode->cpLNode, 4, 1, fpSave) != 1) 
			return cpErrNode;
		if(fwrite(&cpTmpNode->cpRNode, 4, 1, fpSave) != 1)
			return cpErrNode;

		if(cpTmpNode->cpRNode)
			cppStack[lStackPos++] = cpTmpNode->cpRNode;
		if(cpTmpNode->cpLNode)
			cppStack[lStackPos++] = cpTmpNode->cpLNode;
		if(cpTmpNode->cpChildNode)
			cppStack[lStackPos++] = cpTmpNode->cpChildNode;
	}

	if(fwrite(&cpErrNode, 4, 1, fpSave) != 1) 
		return cpErrNode;

	return NULL;
}

long UniversalDomain::LoadDomain(char* szpLoadFilename)
{
	long lRetVal = -1;

	FILE* fpLoad = fopen(szpLoadFilename, "rb");
	if(fpLoad == NULL)
		return lRetVal;

	fread(&m_lAllPageNum, 4, 1, fpLoad);
	m_lpPageBuf = new long[m_lAllPageNum];
	fread(m_lpPageBuf, m_lAllPageNum<<2, 1, fpLoad);

	DomainNode** cppStack = new DomainNode*[100000];
	long* lpStack = new long[100000];
	m_cpClassRoot = _LoadTree(fpLoad, cppStack, lpStack);
	if(m_cpClassRoot)
		lRetVal = 0;
	delete lpStack;
	delete cppStack;

	fclose(fpLoad);
	
	return lRetVal;	
}

DomainNode* UniversalDomain::_LoadTree(FILE* fpLoad, DomainNode** cppStack, long* lpStack)
{
	long i = 0;
	long lFlag = 0;
	long lStackTop = 0;
	long lTmpVal = 0;
	long lRetVal = -1;
	long* lpTmpBuf = NULL;
	DomainNode* cpHeadNode = NULL;
	DomainNode* cpNewNode = NULL;

	if(fread(&lTmpVal, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	if(lTmpVal == NULL)	return NULL;

	cpHeadNode = _AllocNode();
	if(fread(&cpHeadNode->lBFValue, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	if(fread(&cpHeadNode->lClassNameLen, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	cpHeadNode->szpClassName = _AllocMem(cpHeadNode->lClassNameLen);
	if(cpHeadNode->szpClassName == NULL)
		return NULL;
	if(fread(cpHeadNode->szpClassName, cpHeadNode->lClassNameLen, 1, fpLoad) != 1) 
		goto ERROREND;
	if(fread(&cpHeadNode->lAllPageNum, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	if(fread(&cpHeadNode->lPageNum, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	if(fread(&cpHeadNode->lOffSet, 4, 1, fpLoad) != 1) 
		goto ERROREND;

	if(fread(&cpHeadNode->cpChildNode, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	if(fread(&cpHeadNode->cpLNode, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	if(fread(&cpHeadNode->cpRNode, 4, 1, fpLoad) != 1) 
		goto ERROREND;
	cppStack[lStackTop] = cpHeadNode;
	lpStack[lStackTop++] = 0;

	while(1)
	{
		if(fread(&lTmpVal, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(lTmpVal == 0xCCCCCCCC) break;

		cpNewNode = _AllocNode();
		if(fread(&cpNewNode->lBFValue, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(fread(&cpNewNode->lClassNameLen, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		cpNewNode->szpClassName = _AllocMem(cpNewNode->lClassNameLen);
		if(cpNewNode->szpClassName == NULL)
			return NULL;
		if(fread(cpNewNode->szpClassName, cpNewNode->lClassNameLen, 1, fpLoad) != 1) 
			goto ERROREND;
		if(fread(&cpNewNode->lAllPageNum, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(fread(&cpNewNode->lPageNum, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(fread(&cpNewNode->lOffSet, 4, 1, fpLoad) != 1) 
			goto ERROREND;

		if(fread(&cpNewNode->cpChildNode, 4, 1, fpLoad) != 1)
			goto ERROREND;
		if(fread(&cpNewNode->cpLNode, 4, 1, fpLoad) != 1) 
			goto ERROREND;
		if(fread(&cpNewNode->cpRNode, 4, 1, fpLoad) != 1) 
			goto ERROREND;
NEXTNODE:
		if(lFlag == 0 && cppStack[lStackTop - 1]->cpChildNode)
		{
			lFlag = 0;
			cppStack[lStackTop - 1]->cpChildNode = cpNewNode;
			cppStack[lStackTop] = cpNewNode;
			lpStack[lStackTop++] = 1;		
			continue;
		}
		else if(lFlag == 0)
			lFlag = 1;
		if(lFlag == 1 && cppStack[lStackTop - 1]->cpLNode)
		{
			lFlag = 0;
			cppStack[lStackTop - 1]->cpLNode = cpNewNode;			
			cppStack[lStackTop] = cpNewNode;
			lpStack[lStackTop++] = 2;
			continue;
		}
		else if(lFlag == 1)
			lFlag = 2;
		if(lFlag == 2 && cppStack[lStackTop - 1]->cpRNode)
		{
			lFlag = 0;
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
}

#endif // _UNIVERSAL_AVL_TREE_3_H_