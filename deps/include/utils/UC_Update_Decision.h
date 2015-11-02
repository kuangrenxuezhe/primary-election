//
//  UC_Update_Decision.h
//  code_library
//
//  Created by zhanghl on 13-4-8.
//  Author gaoqihang
//  Copyright (c) 2013å¹´ zhanghl. All rights reserved.
//

#ifndef _CODE_LIBRARY_UC_UPDATE_DECISION_H_
#define _CODE_LIBRARY_UC_UPDATE_DECISION_H_

#include "UC_MD5.h"
#include "ZSSegDll.h"
#include "UH_Define.h"
#include "UT_HashSearch.h"
#include "UC_Allocator_Recycle.h"

#define DEF_BUF_SIZE                 2048
#define DEF_MAX_HASH_NODE_NUM        10000000

typedef struct _UC_String_Node_
{
	var_u8      word_id;
	var_u8      info;
	var_u4      cur_sqnum;
	var_4       key_count;
	var_4       key_cur_num;
	var_4       relation;
} UC_STRING_NODE;

typedef struct _UC_Key_Node_
{
	var_u4                  key_sqnum;
	var_u8                  word_id;
	UC_STRING_NODE*         psearch_string;
	struct _UC_Key_Node_*   pnext;
} UC_KEY_NODE;


class UC_Update_Decision
{
public:
	UC_Update_Decision():m_lCurSQNum(0), m_cpHash_KEY(NULL), \
		m_cpAlloc_SearchString(NULL), m_cpAlloc_Key(NULL), m_pZSSegHand(NULL), \
		m_pDictHand(NULL)
	{
	}

	~UC_Update_Decision()
	{
		if (m_cpHash_KEY)
		{
			delete m_cpHash_KEY;
			m_cpHash_KEY = NULL;
		}
		if (m_cpAlloc_SearchString)
		{
			delete m_cpAlloc_SearchString;
			m_cpAlloc_SearchString = NULL;
		}
		if (m_cpAlloc_Key)
		{
			delete m_cpAlloc_Key;
			m_cpAlloc_Key = NULL;
		}
		if (m_pDictHand && m_pZSSegHand)
		{
			FreeZSSegHandle(m_pDictHand, m_pZSSegHand);
		}
	}
	
	var_4 init(var_1* work_path, var_4 (*update)(var_u8 info, var_vd* argv), var_vd* argv, var_4 hash_size = DEF_MAX_HASH_NODE_NUM)
	{
		m_fun = update;
		m_arg = argv;
		
		m_cpHash_KEY = new UT_HashSearch<var_u8>;
		m_cpAlloc_SearchString = new UC_Allocator_Recycle;
		m_cpAlloc_Key = new UC_Allocator_Recycle;
		if (m_cpHash_KEY == NULL || m_cpAlloc_SearchString == NULL || m_cpAlloc_Key == NULL)
		{
			printf("Malloc hash object failed.\n");
			return -1;
		}
		
		if (m_cpHash_KEY->InitHashSearch(hash_size, sizeof(UC_KEY_NODE*) ) != 0)
		{
			printf("Init hash failed.\n");
			return -1;
		}
		
		if (m_cpAlloc_SearchString->initMem(sizeof(UC_STRING_NODE)) != 0)
		{
			printf("Init search key failed.\n");
			return -1;
		}
		
		if ( m_cpAlloc_Key->initMem(sizeof(UC_KEY_NODE)) != 0)
		{
			printf("Init key failed.\n");
			return -1;
		}
		
		if (GetDictHandle(m_pDictHand) != 0)
		{
			printf("Split module get dict handle failed.\n");
			return -1;
		}
		
		if (GetZSSegHandle(m_pDictHand, m_pZSSegHand) != 0)
		{
			printf("Get split handle failed.\n");
			return -1;
		}
		
		return 0;
	}
	
	/**************
	 * ¹¦ÄÜ: Ìí¼Ó¼ìË÷´®µ½hash¶ÔÏóÖÐ
	 * ²ÎÊýËµÃ÷: 
	 *     word           ¼ìË÷´®,×¢Òâ¼ìË÷´®¸ñÊ½
	 *     word_len       ¼ìË÷´®³¤¶È
	 *     info           ¼ìË÷´®ÐÅÏ¢
	 * ·µ»ØÖµ: ³É¹¦·µ»Ø0, Ê§°Ü·µ»Ø-1
	 *************/
	var_4 add(var_1* word, var_4 word_len, var_u8 word_id, var_u8 info)
	{
		if (word == NULL || word_len <= 0)
		{
			printf("add param has NULL.\n");
			return -1;
		}

		var_4    nFlag = -1;
		var_4    nLength = 0;
		var_4    nRelation = 1;
		var_1    szString[DEF_BUF_SIZE] = "\0";
		// parse search string
		var_u8   lWordId = parse_word(word, word_len, szString, DEF_BUF_SIZE, nLength, nRelation);
		if (lWordId == 0)
		{
			printf("Search key parser failed:%s.\n", word);
			return -1;
		}

	//	printf("Search key:%s, Relation:%d, MD5:%ju.\n", szString, nRelation, lWordId);
		UC_STRING_NODE* pSearchKey = (UC_STRING_NODE*)m_cpAlloc_SearchString->AllocMem();
		if (pSearchKey == NULL)
		{
			printf("Search key object not enough memory.\n");
			return -1;
		}

		pSearchKey->word_id = word_id;
		pSearchKey->info = info;
		pSearchKey->cur_sqnum = 0;
		pSearchKey->key_count = 0;
		pSearchKey->key_cur_num = 0;
		pSearchKey->relation = nRelation;

		var_1   szSplitKey[DEF_BUF_SIZE] = "\0";
		if ( (nLength = (var_4)ZSSegBuf(szString, nLength, szSplitKey, DEF_BUF_SIZE, m_pZSSegHand) ) > 0)
		{
			szSplitKey[nLength] = 0;
			// split key add hash
			if ( (nFlag = split_handle(szSplitKey, pSearchKey)) == 1)
			{
				m_cpAlloc_SearchString->FreeMem((var_1*)pSearchKey);
			}
		}

		return nFlag;
	}

	var_4 del(var_1* word, var_u8 word_id)
	{
		if (word == NULL)
		{
			printf("Delete param is NULL.\n");
			return -1;
		}

		cp_drop_useless_char(word);
		var_4  nLength = (var_4)strlen(word);

		var_4    nDestLen = 0;
		var_4    nRelation = 1;
		var_1    szString[DEF_BUF_SIZE] = "\0";
		// parse search string
		var_u8   lDelMask = parse_word(word, nLength, szString, DEF_BUF_SIZE, nDestLen, nRelation);
		if (lDelMask == 0)
		{
			printf("Search key parser failed:%s.\n", word);
			return -1;
		}

		var_1  szSplitBuff[DEF_BUF_SIZE] = "\0";
		if ( (nLength = (var_4)ZSSegBuf(szString, nDestLen, szSplitBuff, DEF_BUF_SIZE, m_pZSSegHand) ) > 0)
		{
			szSplitBuff[nLength] = 0;
			return split_delete(szSplitBuff, word_id);
		}

		return -1;
	}

	var_4 check(var_1* page_buf, var_4 page_len)
	{
		if (page_buf == NULL || page_len <= 0)
		{
			printf("Check param has NULL.\n");
			return -1;
		}
		m_lCurSQNum++;

		var_4   nExact = 0;
		var_4   nFuzzy = 0;
		var_4   nNot = 0;
		var_1*  pKey = page_buf;
		var_1*  pBlank = NULL;
		var_u8  nMask = 0;
		UC_MD5  MaskObj;

		pKey[page_len] = 0;
		while ( (pBlank = strchr(pKey, ' ') ) )
		{
			*pBlank = '\0';
			
			var_1* ptr_beg = pKey;
			while(*ptr_beg)
			{
				if(*ptr_beg < 0)
				{
					ptr_beg++;
					if(*ptr_beg)
						ptr_beg++;
				}
				else if(*ptr_beg > 96 && *ptr_beg < 123)
				{
					*ptr_beg -= 32;
					ptr_beg++;
				}
				else
					ptr_beg++;
			}

			nMask = MaskObj.MD5Bits64((var_u1*)pKey, (var_4)(pBlank - pKey));
			if (nMask > 0)
			{
				hash_check_key(nMask, nExact, nFuzzy, nNot);
			}
			pKey = pBlank + 1;
		}

		var_4 nLength = (var_4)strlen(pKey);
		if (nLength > 0)
		{
			nMask = MaskObj.MD5Bits64((var_u1*)pKey, nLength);
			hash_check_key(nMask, nExact, nFuzzy, nNot);
		}

//		printf("And match:%d. Or match:%d. Not match:%d\n", nExact, nFuzzy, nNot);
		return 0;
	}
	
private:
	var_u8 parse_word(var_1* word, var_4 word_len, var_1* dest_buf, var_4 buf_len, var_4& dest_len, var_4& relation)
	{
		var_4 j = 0;
		var_4 nFlag = 0;
		relation = 1;
		for (var_4 i = 0; i < word_len; i++)
		{
			if (word[i] == ')')
			{
				nFlag = 0;
			}

			if (nFlag == 1)
			{
				dest_buf[j++] = word[i];
				if (j + 1 >= buf_len)
				{
					printf("Dest buff length enough.\n");
					return 0;
				}
			}
			else
			{
				if (word[i] == '&')
					dest_buf[j++] = ' ';
				if (word[i] == '|' || word[i] == '!')
				{
					relation = (word[i] == '|') ? 2 : 3;
					dest_buf[j++] = ' ';
				}
			}
			
			if ((word[i - 1] == ':' && word[i] == '(') || (word[i - 1] == '!' && word[i] == '(') )
			{
				nFlag = 1;
			}

		}

		dest_buf[j] = 0;
		dest_len = j;
		
		var_1* ptr_beg = dest_buf;
		while(*ptr_beg)
		{
			if(*ptr_beg < 0)
			{
				ptr_beg++;
				if(*ptr_beg)
					ptr_beg++;
			}
			else if(*ptr_beg > 96 && *ptr_beg < 123)
			{
				*ptr_beg -= 32;
				ptr_beg++;
			}
			else
				ptr_beg++;
		}
		
		UC_MD5  MaskObj;
		var_u8  lWordId = MaskObj.MD5Bits64((var_u1*)dest_buf, j);
		return lWordId;
	}

	var_4  split_handle(var_1* split_key, UC_STRING_NODE* psearch_key)
	{
		var_4   nFlag = 0;
		var_1*  pKey = split_key;
		var_1*  pBlank = NULL;
		var_u8  nMask = 0;
		UC_MD5  MaskObj;

		while ( (pBlank = strchr(pKey, ' ') ) )
		{
			*pBlank = '\0';
			nMask = MaskObj.MD5Bits64((var_u1*)pKey, (var_4)(pBlank - pKey));
			if (nMask > 0)
			{
				if ((nFlag = hash_add_key(nMask, psearch_key)) != 0)
				{
					return nFlag;
				}
			}
			pKey = pBlank + 1;
		}

		int nLength = (var_4)strlen(pKey);
		if (nLength > 0)
		{
			nMask = MaskObj.MD5Bits64((var_u1*)pKey, nLength);
			nFlag = hash_add_key(nMask, psearch_key);
		}
		return nFlag;
	}

	var_4  hash_add_key(var_u8 word_id, UC_STRING_NODE* psearch_key)
	{
		var_vd*       pRest = NULL;
		UC_KEY_NODE*  pKeyNode = NULL;
		UC_KEY_NODE*  pRetNode = NULL;

		// add hash, find key
		if (m_cpHash_KEY->SearchKey_FL(word_id, (var_vd**)&pRest) == 0)
		{
			if (pRest == NULL)
			{
				printf("add bug.\n");
				return -1;
			}

			pRetNode = *(UC_KEY_NODE**)pRest;
			while (pRetNode)
			{
				if (pRetNode->word_id == psearch_key->word_id && \
					pRetNode->psearch_string->relation == psearch_key->relation)
				{
	//				printf("Search key exist.\n");
					return 0;
				}
				if (pRetNode->pnext == NULL)
				{
					break;
				}
				pRetNode = pRetNode->pnext;
			}
			// add key to list
			pKeyNode = (UC_KEY_NODE*)m_cpAlloc_Key->AllocMem();
			if (pKeyNode == NULL)
			{
				printf("Key node object not enough memory.\n");
				psearch_key->key_count = 30;
				return -1;
			}
			pKeyNode->key_sqnum = 0;
			pKeyNode->word_id = psearch_key->word_id;
			pKeyNode->psearch_string = psearch_key;
			pKeyNode->pnext = NULL;

			pRetNode->pnext = pKeyNode;
		}
		else
		{
			pKeyNode = (UC_KEY_NODE*)m_cpAlloc_Key->AllocMem();
			if (pKeyNode == NULL)
			{
				printf("Key node object not enough memory.\n");
				psearch_key->key_count = 30;
				return -1;
			}
			pKeyNode->key_sqnum = 0;
			pKeyNode->word_id = psearch_key->word_id;
			pKeyNode->psearch_string = psearch_key;
			pKeyNode->pnext = NULL;

			if (m_cpHash_KEY->AddKey_FL(word_id, (var_vd*)&pKeyNode) != 0)
			{
				printf("Add key to the hash failed.\n");
				psearch_key->key_count = 30;
				return -1;
			}
		}

		psearch_key->key_count++;
		return 0;
	}

	var_4  split_delete(var_1* split_key, var_u8 del_word)
	{
		var_4   nFlag = 0;
		var_1*  pKey = split_key;
		var_1*  pBlank = NULL;
		var_u8  nMask = 0;
		UC_MD5  MaskObj;

		while ( (pBlank = strchr(pKey, ' ') ) )
		{
			*pBlank = '\0';
			nMask = MaskObj.MD5Bits64((var_u1*)pKey, (var_4)(pBlank - pKey));
			if (nMask > 0)
			{
				if ( (nFlag = hash_del_key(del_word, nMask, 0) ) != 0)
				{
					return nFlag;
				}
			}
			pKey = pBlank + 1;
		}

		int nLength = (var_4)strlen(pKey);
		if (nLength > 0)
		{
			nMask = MaskObj.MD5Bits64((var_u1*)pKey, nLength);
			nFlag = hash_del_key(del_word, nMask, 1);
		}

		return nFlag;
	}

	var_4  hash_del_key(var_u8 del_word, var_u8 word_id, var_4 flag)
	{
		var_vd*       pRest = NULL;
		UC_KEY_NODE*  pKeyNode = NULL;
		UC_KEY_NODE*  pDeleteNode = NULL;
		UC_STRING_NODE*  pStringNode = NULL;

		// Not found key
		if (m_cpHash_KEY->SearchKey_FL(word_id, (var_vd**)&pRest) != 0)
		{
	//		printf("Delete search string not found.\n");
			return 1;
		}

		if (pRest == NULL)
		{
			printf("del Bug.\n");
			return -1;
		}
		pKeyNode = *(UC_KEY_NODE**)pRest;
		if (pKeyNode->word_id == del_word)
		{
			if (m_cpHash_KEY->DeleteKey_FL(word_id) == -1)
			{
				printf("Hash delete failed.\n");
				return -1;
			}

			pDeleteNode = pKeyNode;
			pStringNode = pDeleteNode->psearch_string;
			pKeyNode = pKeyNode->pnext;
			m_cpAlloc_Key->FreeMem((var_1*)pDeleteNode);
			if (pKeyNode != NULL)
			{
				if (m_cpHash_KEY->AddKey_FL(word_id, (var_vd*)&pKeyNode) != 0)
				{
					printf("Add key failed.\n");
					return -1;
				}
			}
			if (flag == 1)
				m_cpAlloc_SearchString->FreeMem((var_1*)pStringNode);
			return 0;
		}

		while (pKeyNode->pnext)
		{
			if (pKeyNode->pnext->word_id == del_word)
			{
				pDeleteNode = pKeyNode->pnext;
				pStringNode = pDeleteNode->psearch_string;
				pKeyNode->pnext = pKeyNode->pnext->pnext;
				m_cpAlloc_Key->FreeMem((var_1*)pDeleteNode);
				if (flag == 1)
					m_cpAlloc_SearchString->FreeMem((var_1*)pStringNode);
				break;
			}
			pKeyNode = pKeyNode->pnext;
		}

		return 0;
	}

	var_4  hash_check_key(var_u8  word_id, var_4& exact, var_4& fuzzy, var_4& nNot)
	{
		var_vd*       pRest = NULL;
		UC_KEY_NODE*  pKeyNode = NULL;
		UC_STRING_NODE*  pStringNode = NULL;

		// Not found key
		if (m_cpHash_KEY->SearchKey_FL(word_id, (var_vd**)&pRest) != 0)
		{
			return 0;
		}

		if (pRest == NULL)
		{
			printf("check BUG.\n");
			return -1;
		}

		pKeyNode = *(UC_KEY_NODE**)pRest;
		while (pKeyNode)
		{
			if (m_lCurSQNum != pKeyNode->key_sqnum)
			{
				pKeyNode->key_sqnum = m_lCurSQNum;
				pStringNode = pKeyNode->psearch_string;
				if (m_lCurSQNum == pStringNode->cur_sqnum)
				{
					if (pStringNode->relation == 1)
						pStringNode->key_cur_num++;
				}
				else
				{
					pStringNode->cur_sqnum = m_lCurSQNum;
					pStringNode->key_cur_num = 1;
					if (pStringNode->relation == 2)
					{
						// TODO something
						m_fun(pStringNode->info, m_arg);
						
						fuzzy++;
					}
					else if (pStringNode->relation == 3)
					{
						// TODO something
						m_fun(pStringNode->info, m_arg);

						nNot++;
					}
				}

				if (pStringNode->key_cur_num == pStringNode->key_count)
				{
					// TODO something
					m_fun(pStringNode->info, m_arg);
					
					exact++;
				}
			}
			else
			{
				break;
			}

			pKeyNode = pKeyNode->pnext;
		}

		return 0;
	}

private:
	var_u4                   m_lCurSQNum;
	UT_HashSearch<var_u8>*   m_cpHash_KEY;
	UC_Allocator_Recycle*    m_cpAlloc_SearchString;
	UC_Allocator_Recycle*    m_cpAlloc_Key;
	var_vd*                  m_pZSSegHand;
	var_vd*                  m_pDictHand;
	
	var_4 (*m_fun)(var_u8 info, var_vd* argv);
	var_vd* m_arg;
};

#endif // _CODE_LIBRARY_UC_UPDATE_DECISION_H_

