// U_List.h

#ifndef _UNIVERSAL_LIST_S_H_
#define _UNIVERSAL_LIST_S_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UT_Allocator.h"

#define max_val(a,b) (((a) > (b)) ? (a) : (b))
#define min_val(a,b) (((a) < (b)) ? (a) : (b))

template <class T_Key>
struct _Node_List_S_ 
{
	_Node_List_S_* cpNext;
	
	T_Key tKey;
};

template<class T_Key>
inline long _DefaultCompareKey_List_(T_Key t1, T_Key t2)
{
	return t1 - t2;
}

template<class T_Key>
inline long _DefaultSaveNode_List_(_Node_List_S_<T_Key>* cpNode, FILE* fp)
{
	fwrite(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
}

template<class T_Key>
inline long _DefaultLoadNode_List_(_Node_List_S_<T_Key>* cpNode, FILE* fp, void* vpArg)
{
	fread(&cpNode->tKey, sizeof(T_Key), 1, fp);
	return 0;
}

template <class T_Key, class T_NodeType = _Node_List_S_<T_Key> >
class U_List_S
{
public:
	U_List_S();
	~U_List_S();

	T_NodeType* GetNode();
	T_NodeType* AddNode(T_Key tKey);	
	T_NodeType* SearchNode(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key) = _DefaultCompareKey_List_);

	long SaveList(char* szpSaveFilename, long (__cdecl* _SaveNode)(T_NodeType*, FILE*) = _DefaultSaveNode_List_);
	long LoadList(char* szpLoadFilename, long (__cdecl* _LoadNode)(T_NodeType*, FILE*, void*) = _DefaultLoadNode_List_, void* vpArg = NULL);

	long GetNodeNum();

private:
	long m_lKeyNum;
	
	_Node_List_S_<T_Key>* m_cpHead;	

	UT_Allocator<T_NodeType> m_tpAllocator;
};

/************************************************************************/
/*                          U_List_S Implement                          */
/************************************************************************/
template <class T_Key, class T_NodeType>
U_List_S<T_Key, T_NodeType>::U_List_S()
{
	m_lKeyNum = 0;
	m_cpHead = NULL;
}

template <class T_Key, class T_NodeType>
U_List_S<T_Key, T_NodeType>::~U_List_S()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T_Key, class T_NodeType>
T_NodeType* U_List_S<T_Key, T_NodeType>::AddNode(T_Key tKey)
{
	_Node_List_S_<T_Key>* cpNode = m_tpAllocator.Allocate();
	if(cpNode == NULL)
		return NULL;
	cpNode->cpNext = m_cpHead;	
	m_cpHead = cpNode;
	m_lKeyNum++;

	cpNode->tKey = tKey;

	return (T_NodeType*)cpNode;
}

template <class T_Key, class T_NodeType>
T_NodeType* U_List_S<T_Key, T_NodeType>::GetNode()
{
	_Node_List_S_<T_Key>* cpNode = m_cpHead;
	m_cpHead = m_cpHead->cpNext;
	m_lKeyNum--;

	return (T_NodeType*)cpNode;
}

template <class T_Key, class T_NodeType>
T_NodeType* U_List_S<T_Key, T_NodeType>::SearchNode(T_Key tKey, long (__cdecl* _CompareKey)(T_Key, T_Key))
{
	_Node_List_S_<T_Key>* cpNode = m_cpHead;
	for(long i = 0; i < m_lKeyNum; i++)
	{
		if(_CompareKey(tKey, cpNode->tKey))
			cpNode = cpNode->cpNext;
		else
			break;
	}
	if(i == m_lKeyNum)
		return NULL;

	return (T_NodeType*)cpNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T_Key, class T_NodeType>
long U_List_S<T_Key, T_NodeType>::SaveList(char* szpSaveFilename, long (__cdecl* _SaveNode)(T_NodeType*, FILE*))
{
	FILE* fpSave = fopen(szpSaveFilename, "wb");
	if(fpSave == NULL)
		return -1;
		
	if(fwrite(&m_lKeyNum, 4, 1, fpSave) != 1)
	{
		fclose(fpSave);
		return -1;
	}

	_Node_List_S_<T_Key>* cpNode = m_cpHead;
	for(long i = 0; i < m_lKeyNum; i++)
	{
		if(fwrite(&cpNode, 4, 1, fpSave) != 1)
		{
			fclose(fpSave);
			return -1;
		}
		if(_SaveNode(cpNode, fpSave))
		{
			fclose(fpSave);
			return -1;
		}
	}
	cpNode = NULL;
	if(fwrite(&cpNode, 4, 1, fpSave) != 1)
	{
		fclose(fpSave);
		return -1;
	}

	fclose(fpSave);	

	return 0;
}

template <class T_Key, class T_NodeType>
long U_List_S<T_Key, T_NodeType>::LoadList(char* szpLoadFilename, long (__cdecl* _LoadNode)(T_NodeType*, FILE*, void*), void* vpArg)
{
	FILE* fpLoad = fopen(szpLoadFilename, "rb");
	if(fpLoad == NULL)
		return -1;

	long lNodeNum = 0;
	if(fread(&lNodeNum, 4, 1, fpLoad) != 1)
	{
		fclose(fpLoad);
		return -1;
	}

	m_cpHead = NULL;	
	_Node_List_S_<T_Key>* cpNode = NULL;
	for(;;)
	{
		if(fread(&cpNode, 4, 1, fpLoad) != 1)
		{
			m_tpAllocator.FreeAllocator();
			fclose(fpLoad);
			return -1;
		}
		if(cpNode == NULL)
			break;
		cpNode = m_tpAllocator.Allocate();
		if(_LoadNode(cpNode, fpLoad, vpArg))
		{
			m_tpAllocator.FreeAllocator();
			fclose(fpLoad);
			return -1;
		}
		if(m_cpHead == NULL)
			m_cpHead = cpNode;
		else
		{
			cpNode->cpNext = m_cpHead;
			m_cpHead = cpNode;
		}
		m_lKeyNum++;
	}	
	fclose(fpLoad);

	if(m_lKeyNum != lNodeNum)
	{
		m_tpAllocator.FreeAllocator();
		return -1;
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T_Key, class T_NodeType>
long U_List_S<T_Key, T_NodeType>::GetNodeNum()
{
	return m_lKeyNum;
}

#endif // _UNIVERSAL_LIST_S_H_
