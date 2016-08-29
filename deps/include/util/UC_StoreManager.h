//  UC_StoreManager.h
//  code_library
//
//  Created by zhanghl on 12-11-1.
//  Copyright (c) 2012�� zhanghl. All rights reserved.
//

#ifndef _UC_STORE_MANAGER_H_
#define _UC_STORE_MANAGER_H_

#include "UT_Queue.h"
#include "UC_Allocator_Recycle.h"
#include "UC_DiskQuotaManager.h"
#include <math.h>

#ifdef _LINUX_ENV_
#undef  _snprintf
#define _snprintf   snprintf

#undef _vsnprintf
#define _vsnprintf  vsnprintf
#endif

#define MAX_VAL(a, b) ((a) > (b) ? (a) : (b))
#define HEIGHT_TREE(spT) ((spT) == NULL ? (-1) : ((spT)->height) )

#define _PRINTF_LOG_FILE_
#ifdef _PRINTF_LOG_FILE_
class CStoreLog
{
public:
	CStoreLog()
	{
		m_lpszMsg=NULL;
		m_fpLog=NULL;
		m_iCurDay=0;
	}

	~CStoreLog()
	{
		if(m_lpszMsg!=NULL)
		{
			delete []m_lpszMsg;
			m_lpszMsg=NULL;
		}

		if(m_fpLog != NULL)
		{
			fclose(m_fpLog);
			m_fpLog=NULL;
		}
	}

	const var_4 InitLog(const var_u4 uMsgLen, const char *const lpszPath, const bool bNeedFlush=true)
	{
		if(uMsgLen<=0 || lpszPath==NULL)
			return -1;

		m_uMsgSize=uMsgLen;

		m_lpszMsg = new char[uMsgLen];
		if(m_lpszMsg==NULL)
			return -2;

		time_t m_time_t;					//���ڴ洢��ǰʱ�����ʱ����
		struct tm* m_tm;					//���ڴ洢��ǰʱ�����ʱ����
		time(&m_time_t);
		m_tm = localtime(&m_time_t);
		m_iCurDay = m_tm->tm_mday;

		m_bFlush = bNeedFlush;

		var_u4 uLen = (var_u4)strlen(lpszPath);

		if(lpszPath[uLen-1]!='/')
		{
			if(_snprintf(m_szLogPath,255, "%s/", lpszPath)<=0)
				return -5;
		}
		else
		{
			if(_snprintf(m_szLogPath,255, "%s", lpszPath)<=0)
				return -6;
		}		
		cp_create_dir(m_szLogPath);

		return 0;
	}	

	var_4 Log(const bool bPrintScree,const char * format, ...)
	{	
		if(m_lpszMsg==NULL || m_uMsgSize<=0)
			return -1;

		//ȡ�䳤�������γ�Ҫ��ӡ��BUF		
		var_4 lRet=-1;

		m_Mutex.lock();

		va_list arg;
		va_start(arg, format);	
		lRet=_vsnprintf(m_lpszMsg, m_uMsgSize,format,arg);	
		va_end(arg);

		if(lRet<=0)
		{
			m_Mutex.unlock();
			return -1;			
		}

		struct tm * m_tm;
		time_t m_time_t;
		time(&m_time_t);
		m_tm = localtime(&m_time_t);

		if(m_fpLog == NULL)       //˵�����ǵ�һ��LOG	
		{				
			sprintf(m_szFileName, "%s%04d-%02d-%02d.log", m_szLogPath,m_tm->tm_year+1900, m_tm->tm_mon+1, m_tm->tm_mday);
			m_fpLog = fopen(m_szFileName, "a");	

		}
		else if(m_iCurDay != m_tm->tm_mday)//�ж��Ƿ����µ�һ��
		{
			if(m_fpLog)
			{
				fclose(m_fpLog);
				m_fpLog = NULL;
			}

			sprintf(m_szFileName, "%s%04d-%02d-%02d.log", m_szLogPath,m_tm->tm_year+1900, m_tm->tm_mon+1, m_tm->tm_mday);			
			m_fpLog = fopen(m_szFileName, "a");	

			m_iCurDay = m_tm->tm_mday;
		}

		if(m_fpLog != NULL)
		{
			fprintf(m_fpLog,"%02d:%02d:%02d  %s", m_tm->tm_hour, m_tm->tm_min, m_tm->tm_sec, m_lpszMsg);

			if(m_bFlush)	//ʵʱˢ��
			{
				fflush(m_fpLog);
			}
			lRet=0;
		}
		else
		{
			lRet=-1;
		}
		//��ӡ��STDOUT
		if(bPrintScree)
		{
			printf("%02d:%02d:%02d  %s", m_tm->tm_hour, m_tm->tm_min, m_tm->tm_sec, m_lpszMsg);
		}

		m_Mutex.unlock();			

		return lRet;
	}

private:
	char   * m_lpszMsg;						//log����BUF
	FILE   * m_fpLog;							//��־�ļ���ָ��
	var_4    m_iCurDay;					//�����Ƿ��Ѿ��ı���
	bool     m_bFlush;				    //�Ƿ�ʵʱˢ��

	char    m_szFileName[256];        //��ǰ��¼�ļ����ļ���,��ʽΪyyyy-mm-dd.log
	char    m_szLogPath[256];         //��־Ŀ¼

	var_u4      m_uMsgSize;

	CP_MUTEXLOCK m_Mutex;

};
#endif

#define DEBUG_ASSERT_ERROR

static CP_MUTEXLOCK g_log_mtx;

inline void STORE_PRINTF_LOG(const var_1* format, ...)
{
	g_log_mtx.lock();
	va_list arg;
	va_start(arg, format);	
	vfprintf(stdout,format, arg);  	
	va_end(arg);
	g_log_mtx.unlock();
#ifdef DEBUG_ASSERT_ERROR
	assert(0);
#endif
}

#ifdef DEBUG_ASSERT_ERROR
#define VERIFY_TREE_NUM(pst_atn, num) do{\
	if(pst_atn)\
	assert(pst_atn->lock_suf >= 0);\
	var_4 tree_num =  get_tree_num((pst_atn));\
	if(tree_num != (num))\
{\
	STORE_PRINTF_LOG("tree: get_num:%d, parent_num:%d\n", tree_num, (num));\
}\
}while(0)

#else
#define VERIFY_TREE_NUM(ptr_tree, num) do{\
	;\
}while(0)
#endif

#define DEBUG_PRINTF

//ϵͳ������֧�ֵĲ���
const var_u1 I_SYS_MAX_LEVEL = 20;

//��С���п�
#define     MIN_FREE_BLOCK  10

const var_4 I_COUNT_LOCK_MAX = 1 << 24;
const var_4 I_CHILE_TREE_MAX = 1 << 24;
const var_4 I_DCLN_MAX = 1 << 24;

const var_4  I_STACK_SIZE =  100;//(var_4)((log((double)CHILE_TREE_MAX) / log((double)2)) + 2) + 5;

const var_4  I_1M_SIZE = 1<<20;

template <class T_Key>
class CStoreAvlTree;

template <class T_Key>
class CStoreAvlTree;

template <class T_Key>
class CStoreStack;

template<class TKEY>
struct AVL_TREE_NODE;

//////////////////////////////////////////////////////////////////////
//�ࣺ      CStoreList
//���ܣ�	    �洢�ṹʹ�õ�LIST������
//��ע��
//����:      qinfei
//�������:   20121129
//////////////////////////////////////////////////////////////////////
template<class TYPE_VAL>
class CStoreList
{
private:
	typedef struct _list_node_
	{
		TYPE_VAL       tVal;
		_list_node_ *pst_next;
	}LIST_NODE;

public:
	CStoreList()
	{
		m_pst_list_head = NULL;
		m_pst_list_end = NULL;
		m_list_num = 0;
	}

	~CStoreList(){}

	const var_4 Init(const var_4 list_max)
	{
		var_4 max = I_1M_SIZE / sizeof(LIST_NODE);
		var_4 ret = m_aoc_tn_recycle.initMem(sizeof(LIST_NODE),list_max, max);

		return ret;
	}

	const var_4 Get(TYPE_VAL& t_data)
	{
		m_list_mtx.lock();
		if(m_pst_list_head == NULL || m_pst_list_end == NULL)
		{
			m_list_mtx.unlock();
			return -1;
		}

		t_data = m_pst_list_head->tVal;

		if(m_pst_list_head == m_pst_list_end)
		{
			m_pst_list_head = NULL;
			m_pst_list_end = NULL;
		}
		else
		{
			m_pst_list_head = m_pst_list_head->pst_next;
		}
		m_list_num--;
		m_list_mtx.unlock();
		return 0;
	}

	const var_4 Add(TYPE_VAL t_data)
	{
		LIST_NODE * pst_ln = (LIST_NODE * )m_aoc_tn_recycle.AllocMem();
		if(pst_ln == NULL)
		{
			STORE_PRINTF_LOG("add store list error!\n");
			for(;;)
			{
				printf("Add list failed!\n");
				cp_sleep(10000);
			}

			return -1;
		}
		m_list_num++;
		pst_ln->tVal = t_data;
		pst_ln->pst_next = NULL;
		m_list_mtx.lock();
		if(m_pst_list_end)
		{
			m_pst_list_end->pst_next = pst_ln;
			m_pst_list_end = pst_ln;
		}

		if(m_pst_list_head == NULL)
		{
			m_pst_list_head = pst_ln;
		}

		if(m_pst_list_end == NULL)
		{
			m_pst_list_end = pst_ln;
		}

		m_list_mtx.unlock();

		return 0;
	}

private:
	UC_Allocator_Recycle                  m_aoc_tn_recycle;     //�ڴ�������

	LIST_NODE                            *m_pst_list_head;
	LIST_NODE                            *m_pst_list_end;
	CP_MUTEXLOCK                          m_list_mtx;

	var_4                                 m_list_num;
};

#define _TEST_LOCK_COUNT_

#pragma pack(1)

typedef struct _data_count_lock_node_
{
	var_u1   flg;      //д��ʶ
	var_u4   r_count;   //������

	var_u4   oper_count;
#ifdef _TEST_LOCK_COUNT_
	var_u4    write;
	var_u4    read;
#endif
}DATA_COUNT_LOCK_NODE;

template <class TKEY>
struct SUBTREE_NODE
{
	AVL_TREE_NODE<TKEY> *pst_sub;        //ָ�����ӽ���ָ��

	var_u4               del_flg:2,
level:6,        //��ǰ�����ڵĲ�
tree_num:24;    //�������������
};

template <class TKEY>
struct HASH_TABLE_NODE
{
	var_u1                  flg;      //д��ʶ
	var_u4                  r_count;   //������

	SUBTREE_NODE<TKEY>      st_stn;
};

template<class TKEY>
struct AVL_TREE_NODE
{
	AVL_TREE_NODE           *pst_left;
	AVL_TREE_NODE           *pst_right;

	var_u1                  height;
	TKEY                    key;

	SUBTREE_NODE<TKEY>      st_stn;

	var_u4                  lock_suf;
	DATA_COUNT_LOCK_NODE    st_dcln;
	char                    *ptr_data;       //�����ڴ��ַ  
};

template <class TKEY>
struct AOC_MEM_BLOCK_NODE
{
	AVL_TREE_NODE<TKEY>      *pst_atn;  //ָ�������ĸ�����Ϣָ��
	var_u4                    dsk_flg:4, //��ʶ 0:�����ڴ��̿ռ�顡1:���ڴ��̿ռ��
mem_flg:4, //��ʶ 0:�������ڴ�ռ�顡1:�����ڴ�ռ��
size:24;     //������ڴ�ռ�

#ifdef DEBUG_ASSERT_ERROR
	TKEY                      key;
#endif
};

//��������
typedef struct _disk_index_node_
{
	var_4 file_no;
	var_8 off;
}DISK_INDEX_NODE;

//�ڴ渽����Ϣ
typedef struct _mem_appendix_node_
{
	//	var_u4                    dsk_flg:4, //��ʶ 0:�����ڴ��̿ռ�顡1:���ڴ��̿ռ��
	//mem_flg:4, //��ʶ 0:�������ڴ�ռ�顡1:�����ڴ�ռ��
	//mem_len:24;     //������ڴ����ݳ���
	var_u4 mem_len;
}MEM_APPENDIX_NODE;

typedef struct _write_inc_file_node_
{
	var_4  len;
	var_1  oper_flg;
	var_4  num;
}WRITE_INC_FILE_NODE;

template <class TKEY>
struct SUBTREE_SATACK_NODE
{
	AVL_TREE_NODE<TKEY> *pst_cur;
	AVL_TREE_NODE<TKEY> *pst_parent;
};

template <class TKEY>
struct TRAVEL_HEAD_INFO
{
	var_1                                     is_write;    //�Ƿ�д
	var_1                                     max_depth;  //������
	var_4                                    hash_cur;   //HASH TABLE��ǰ�������±�

	CStoreStack<AVL_TREE_NODE<TKEY>* >        tree_stack;
	CStoreStack<SUBTREE_SATACK_NODE<TKEY> >   subtree_stack;

	AVL_TREE_NODE<TKEY>                      *pst_cur;
	DATA_COUNT_LOCK_NODE                     *pst_old_dcln;

	var_4                                     type;

	var_4                                     old_lock_suf;
	var_4                                     num;
	TKEY                                      arr_key[I_SYS_MAX_LEVEL];
};

template <class TKEY>
struct INDEX_FILE_NODE
{
	var_2   level;        //��ǰ����
	TKEY    parent_key;   //������KEY
	TKEY    key;          //��ǰ���KEY
	var_4   mem_len;          //�����ݳ���
	var_4   dsk_len;          //�����ݳ���
};

#pragma pack()

//�������ļ�
inline void rename_file( const char * const src_file, const char * const desc_file)
{
	if(src_file==NULL || desc_file==NULL)
		return ;

	remove(desc_file);

	var_u4 count=0;
	while(access(src_file, 00) == 0 && access(desc_file, 00) != 0 && rename(src_file, desc_file)!=0)
	{
		cp_sleep(1);
		if(count++ >= 10000)
		{
			printf("rename failed!\n");
			count=0;
		}
	}
}

const var_4 I_MAN_SIZE = sizeof(MEM_APPENDIX_NODE);
const var_4 I_DIN_SIZE = sizeof(DISK_INDEX_NODE);
const var_4 I_WIFN_SIZE = sizeof(WRITE_INC_FILE_NODE);


//��ȡ�ڴ������ݵĳ���
template <class TKEY>
inline const var_4 GET_ALLOC_DATA_SIZE(const var_1 *ptr_mem, const var_4 mem_len, DISK_INDEX_NODE *pst_din, var_4 &alloc_size)
{
	alloc_size = sizeof(AOC_MEM_BLOCK_NODE<TKEY>);
	if(pst_din)
	{
		alloc_size += I_DIN_SIZE;
	}

	if(mem_len > 0 && ptr_mem)
	{
		alloc_size += I_MAN_SIZE + mem_len;
	}

	return alloc_size;
}


//��ȡ�ڷ������ݿ��У��ڴ��ͷ���
template <class TKEY>
inline void GET_AMBN_HEAD(char *ptr_data, AOC_MEM_BLOCK_NODE<TKEY> *&pst_ambn)
{
	pst_ambn = (AOC_MEM_BLOCK_NODE<TKEY>*)ptr_data;
}

//��ȡ�ڷ������ݿ��У���������ͷ���
template <class TKEY>
inline void GET_DISK_IDX_HEAD(char *ptr_data, DISK_INDEX_NODE *&pst_din)
{
	pst_din = NULL;

	AOC_MEM_BLOCK_NODE<TKEY>* pst_ambn = (AOC_MEM_BLOCK_NODE<TKEY>*)ptr_data;
	if(pst_ambn && pst_ambn->dsk_flg == 1)
	{
		pst_din = (DISK_INDEX_NODE*)(pst_ambn + 1);
	}
}

//��ȡ�ڷ������ݿ��У��ڴ渽����Ϣͷ���
template <class TKEY>
inline void GET_MAN_HEAD(char *ptr_data, MEM_APPENDIX_NODE *&pst_man)
{
	pst_man = NULL;
	AOC_MEM_BLOCK_NODE<TKEY>* pst_ambn = (AOC_MEM_BLOCK_NODE<TKEY>*)ptr_data;

	if(pst_ambn)
	{
		char *ptr = (char*)(pst_ambn + 1);
		if(pst_ambn->dsk_flg == 1)
		{
			ptr += I_DIN_SIZE;
		}
		pst_man = (MEM_APPENDIX_NODE*)ptr;
	}
}

template <class TKEY>
inline void SET_DISK_IDX_DATA(char *ptr_data, DISK_INDEX_NODE *pst_din)
{
	AOC_MEM_BLOCK_NODE<TKEY>* pst_ambn = (AOC_MEM_BLOCK_NODE<TKEY>*)ptr_data;

	if(pst_ambn->dsk_flg == 0 && pst_ambn->mem_flg == 1)
	{
		char *ptr = ptr_data + sizeof(AOC_MEM_BLOCK_NODE<TKEY>);

		MEM_APPENDIX_NODE *pst_tmp_man = (MEM_APPENDIX_NODE*)ptr;
		ptr += I_DIN_SIZE;

		memmove(ptr, ((char*)pst_tmp_man), pst_tmp_man->mem_len + I_MAN_SIZE);
	}
	memcpy((char*)(pst_ambn + 1), (char*)pst_din, I_DIN_SIZE);
	pst_ambn->dsk_flg = 1;
}

template <class TKEY>
inline void SET_MEM_ADX_DATA(char *ptr_data, char *ptr_mem, const var_4 mem_len)
{
	AOC_MEM_BLOCK_NODE<TKEY>* pst_ambn = (AOC_MEM_BLOCK_NODE<TKEY>*)ptr_data;
	char *ptr = ptr_data + sizeof(AOC_MEM_BLOCK_NODE<TKEY>);
	if(pst_ambn->dsk_flg == 1)
	{
		ptr += I_DIN_SIZE;
	}
	MEM_APPENDIX_NODE *pst_man = (MEM_APPENDIX_NODE*)ptr;

	pst_man->mem_len = mem_len;
	ptr += sizeof(MEM_APPENDIX_NODE);

	memcpy(ptr, ptr_mem, mem_len);
	pst_ambn->mem_flg = 1;
}

#ifdef DEBUG_ASSERT_ERROR
template <class TKEY>
static void VERIFY_DATA_NODE(const var_1 level, TKEY key, AVL_TREE_NODE<TKEY> *pst_atn, char* ptr_data)
{
	assert(pst_atn);

	assert(pst_atn->st_stn.level == level);

	if(pst_atn->st_stn.del_flg == 1)
		return;

	assert(ptr_data);

	assert(pst_atn->lock_suf >= 0);

	AOC_MEM_BLOCK_NODE<TKEY>* ptr_ambn = (AOC_MEM_BLOCK_NODE<TKEY>*)(ptr_data);
	if(ptr_ambn->pst_atn != (pst_atn))
	{
		printf("data_tree:%p, tree:%p\n", ptr_ambn->pst_atn, (pst_atn));
		assert(0);\
	}

	if(ptr_ambn->key != key)
	{
		printf("data_key:"CP_PU64", key:"CP_PU64"\n", ptr_ambn->key, key);
		assert(0);\
	}
}
#else
#define VERIFY_DATA_NODE(key, pst_atn, ptr_data) do{\
	;\
}while(0)
#endif


template <class T_Key>
class UC_StoreManager
{
public:
	UC_StoreManager():
	  I_AMBN_SIZE(sizeof(AOC_MEM_BLOCK_NODE<T_Key>)),
		  I_IFN_SIZE (sizeof(INDEX_FILE_NODE<T_Key>)),
		  I_ATN_SIZE (sizeof(AVL_TREE_NODE<T_Key>)),
		  I_HTLN_SIZE(sizeof(HASH_TABLE_NODE<T_Key>)),
		  I_TKEY_SIZE(sizeof(T_Key)),
		  I_STORE_FILE_MARK(*(var_u8*)"@#@#@#@#"),
		  I_MEMORY_APPENDIX_SIZE((1<<24) - 10)
	  {
		  m_aoc_mem_off = 0;      //��ǰ������ڴ�ƫ��
		  m_ptr_aoc_mem = NULL;      //��ǰ������ڴ���ʼ��ַ

		  m_one_aoc_mem_size = 0;  //���η�����ڴ����󳤶�
		  m_one_inc_data_size = 0;   //�����������ݵ�ռ�����ռ�

		  m_max_level = 0;     //������

		  m_pta_oper_mtx = NULL;
		  m_oper_mtx_max = 0;

		  m_psta_hash_table = NULL;    //HASH �����
		  m_hash_tln_size = 0;   //HASH����

		  m_str_sto_lib_idx[0] = 0;
		  m_fp_inc = NULL;         //�����������

		  m_exit_flg = false;
	  }

	  ~UC_StoreManager()
	  {
		  m_exit_flg = true;
		  while(m_thread_flg != -1)
		  {
			  cp_sleep(100);
		  }

		  if(m_pta_oper_mtx)
		  {
			  delete[] m_pta_oper_mtx;
			  m_pta_oper_mtx = NULL;
		  }

		  if(m_psta_hash_table)
		  {
			  delete[] m_psta_hash_table;
			  m_psta_hash_table = NULL;
		  }

		  if(m_fp_inc)
		  {
			  fclose(m_fp_inc);
			  m_fp_inc = NULL;
		  }

		  char *ptr_mem = NULL;
		  while(m_use_mem_list.Get(ptr_mem) == 0)
		  {
			  delete[] ptr_mem;
		  }

		  if(m_ptr_aoc_mem)
		  {
			  delete[] m_ptr_aoc_mem;
			  m_ptr_aoc_mem = NULL;
		  }

		  TRAVEL_HEAD_INFO<T_Key> *pst_thi = NULL;
		  while(m_travel_que.PopData_NB(pst_thi) == 0)
		  {
			  delete pst_thi;
		  }
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      init
	  // ����:      ��ʼ���洢������
	  // ���:
	  //               in_max_level: ���ּ�key���
	  //             in_max_key_num: ÿ�����Ӽ����Ա�������key����
	  //                 in_lib_sto: �ڴ沿�ִ洢�ļ�������·���ļ���,���ΪNULL,�򲻱����ڴ渽����Ϣ������
	  //                 in_lib_inc: �ڴ沿�������ļ�������·���ļ���,���ΪNULL,�򲻱����ڴ渽����Ϣ������
	  //                 in_is_disk: �Ƿ������̴洢ϵͳ,0:��������1����
	  //            in_sto_path_num: ���̴洢ϵͳ��������������ļ���·���б����
	  //                in_sto_path: ���̴洢ϵͳ��������������ļ���·�����б�(��Ҫ��'/'��β)
	  //          in_sto_total_size: ���̴洢ϵͳ��Ӧÿ����·���¿���ʹ�õĴ��̿ռ�(��GBΪ��λ)
	  //            in_sto_one_size: ���̴洢ϵͳ��ÿ�����ļ��Ĵ�С(��GBΪ��λ)
	  //          in_max_write_size: д�����ϵͳ�������������(��λ:byte)
	  //            ��in_max_travel: ϵͳͬһʱ��֧�ֵ�����������
	  //
	  // ����:
	  // ����ֵ:    �ɹ�:����0,����:���ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 init(const var_4 in_max_level, const var_4* in_max_key_num, const var_1* in_lib_sto = NULL, const var_1* in_lib_inc = NULL,
		  const var_4 in_is_disk = 0,const var_4 in_sto_path_num = 0, const var_1** in_sto_path = NULL, const var_4* in_sto_total_size = NULL,
		  const var_4 in_sto_one_size = 2, const var_4 in_max_write_size = 1048576, const var_4 in_max_travel = 10)
	  {
		  if(in_max_level < 1 || in_max_key_num == NULL)
			  return -1;

		  if(in_max_level > I_SYS_MAX_LEVEL)
			  return -2;

		  m_max_level = in_max_level;
		  m_one_inc_data_size = in_max_write_size;

		  var_4 i = 0, ret = 0;

		  var_u4  tree_node_max = 0;

		  for(i = 0; i < in_max_level; i++)
		  {
			  tree_node_max += in_max_key_num[i];
			  m_arr_tree_key_max[i] = in_max_key_num[i];
			  if(in_max_key_num[i] > I_CHILE_TREE_MAX)
				  m_arr_tree_key_max[i] = I_CHILE_TREE_MAX;
		  }

		  if(m_use_mem_list.Init(tree_node_max) != 0)
			  return -3;

		  if(m_free_tnode_list.Init(tree_node_max) != 0)
			  return -4;

#ifdef _PRINTF_LOG_FILE_		
		  m_log.InitLog(1024, "store_log/");
#endif     
		  m_one_aoc_mem_size = in_max_write_size;
		  m_ptr_aoc_mem = NULL;
		  m_aoc_mem_off = 0;

		  const var_2  I_HASH_MAX = 11;
		  var_u4 hash_max_val = 0, hash_min_val = 10;
		  m_oper_mtx_max = 10;

		  for(i = 1; i < I_HASH_MAX; i++)
		  {
			  hash_max_val = hash_min_val * 10;
			  m_oper_mtx_max *= 2;

			  if(((var_u4)in_max_key_num[0]) >= hash_min_val && ((var_u4)in_max_key_num[0]) < hash_max_val)
			  {
				  m_hash_tln_size = hash_min_val;
			  }

			  hash_min_val = hash_max_val;
		  }

		  m_psta_hash_table = new HASH_TABLE_NODE<T_Key>[m_hash_tln_size];
		  if(m_psta_hash_table == NULL)
			  return -4;
		  memset(m_psta_hash_table, 0, I_HTLN_SIZE * m_hash_tln_size);

		  ret = m_aoc_tn_recycle.initMem(I_ATN_SIZE, tree_node_max * 2, I_1M_SIZE / I_ATN_SIZE);
		  if(ret != 0)
			  return -5;

		  m_pta_oper_mtx = new CP_MUTEXLOCK[m_oper_mtx_max];
		  if(m_pta_oper_mtx == NULL)
			  return -6;

		  if(in_max_travel > 0)
		  {
			  ret = m_travel_que.InitQueue(in_max_travel);
			  if(ret != 0)
				  return -7;

			  TRAVEL_HEAD_INFO<T_Key>* pst_thi = NULL;
			  for(i = 0; i < in_max_travel; i++)
			  {
				  pst_thi = new TRAVEL_HEAD_INFO<T_Key>;
				  if(pst_thi == NULL)
					  return -8;

				  m_travel_que.PushData(pst_thi);
			  }
		  }

		  if(in_sto_path_num > 0)
		  {
			  ret = m_disk_quota_man.dqm_init(in_sto_path_num, (var_1**)in_sto_path, (var_4*)in_sto_total_size, in_sto_one_size, in_max_write_size, 5,
				  fun_judge, fun_update, this);
			  if(ret != 0)
				  return ret - 400;
		  }

		  if(in_lib_sto)
		  {
			  if(_snprintf(m_str_sto_lib_idx, 256,"%s", in_lib_sto) <= 0)
				  return -9;

			  char str_cge[256];
			  if(_snprintf(str_cge, 256, "%s.cge", m_str_sto_lib_idx) <= 0)
				  return -10;

			  if(access(str_cge, 00) == 0)
			  {
				  rename_file(str_cge, m_str_sto_lib_idx);
			  }

			  if(access(m_str_sto_lib_idx, 00) == 0)
			  {
				  ret = load();
				  if(ret != 0)
					  return ret;
			  }
		  }

		  if(in_lib_inc)
		  {
			  if(access(in_lib_inc, 0) == 0)
			  {
				  ret = load_inc_file((var_1)in_max_level, in_lib_inc);
				  if(ret < 0)
					  return ret;

				  if(ret > 0)
				  {
					  ret = save();
					  if(ret != 0)
						  return ret;
				  }
			  }

			  m_fp_inc = fopen(in_lib_inc, "wb");
			  if(m_fp_inc == NULL)
				  return -11;
		  }      

		  cp_create_thread(arrange_thread, this);
		  return 0;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      add_mem
	  // ����:      ��洢�������ڴ沿����������
	  // ���:
	  //              in_num: ��Ҫ���ӵ�key·�����
	  //              in_key: ��Ҫ���ӵ�key�б�
	  //              in_mem: ��Ӧÿһ��key���ڴ渽����Ϣ
	  //          in_mem_len: ��Ӧÿһ��key���ڴ渽����Ϣ����
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����·�����Ѿ����ڵĽ�㲻�����κδ���,���·���ϵĽ�㲻����,�����ӽ��ͬʱ�����ڴ渽����Ϣ,
	  //          �����㸽����ϢΪNULL�򳤶�Ϊ0,��ֻ��ӽ��,���ᱣ�ָ�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 add_mem(const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len)
	  {
		  return add(1, in_num, in_key, in_mem, in_mem_len, NULL, NULL);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      add_dsk
	  // ����:      ��洢���������̲�����������
	  // ���:
	  //              in_num: ��Ҫ���ӵ�key·�����
	  //              in_key: ��Ҫ���ӵ�key��
	  //              in_dsk: ��Ӧÿһ��key�Ĵ��̸�����Ϣ
	  //          in_dsk_len: ��Ӧÿһ��key�Ĵ��̸�����Ϣ����
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����·�����Ѿ����ڵĽ�㲻�����κδ���,���·���ϵĽ�㲻����,�����ӽ��ͬʱ������̸�����Ϣ,
	  //          �����㸽����ϢΪNULL�򳤶�Ϊ0,��ֻ��ӽ��,���ᱣ�渽����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 add_dsk(const var_4 in_num, const T_Key* in_key, const var_1** in_dsk, const var_4* in_dsk_len)
	  {
		  return add(1, in_num, in_key, NULL, NULL, in_dsk, in_dsk_len);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      add_mix
	  // ����:      ��洢�������ڴ沿������̲�����������
	  // ���:
	  //              in_num: ��Ҫ���ӵ�key·�����
	  //              in_key: ��Ҫ���ӵ�key�б�
	  //              in_mem: ��Ӧÿһ��key���ڴ渽����Ϣ
	  //          in_mem_len: ��Ӧÿһ��key���ڴ渽����Ϣ����
	  //              in_dsk: ��Ӧÿһ��key�Ĵ��̸�����Ϣ
	  //          in_dsk_len: ��Ӧÿһ��key�Ĵ��̸�����Ϣ����
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����·�����Ѿ����ڵĽ�㲻�����κδ���,���·���ϵĽ�㲻����,�����ӽ��ͬʱ�����ڴ�����̸�����Ϣ,
	  //          �����㸽����ϢΪNULL�򳤶�Ϊ0,��ֻ��ӽ��,���ᱣ�渽����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 add_mix(const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len, const var_1** in_dsk, const var_4* in_dsk_len)
	  {
		  return add(1, in_num, in_key, in_mem, in_mem_len, in_dsk, in_dsk_len);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      del
	  // ����:      �ڴ洢��������ɾ��ָ�����
	  // ���:
	  //              in_num: ��Ҫɾ����key·�����
	  //              in_key: ��Ҫɾ����key�б�
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ���ô˺�������ɾ������key·������ĩ������Լ��˽���µ�ȫ�����
	  //////////////////////////////////////////////////////////////////////
	  const var_4 del(const var_4 in_num, T_Key* in_key)
	  {
		  return del_key(in_num, in_key, 1);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      del_mem
	  // ����:      �ڴ洢��������ɾ��ָ�������ڴ渽����Ϣ
	  // ���:
	  //              in_num: ��Ҫɾ����key·�����
	  //              in_key: ��Ҫɾ����key�б�
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ���ô˺�������ɾ������key·������ĩ������ϵ��ڴ渽����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 del_mem(const var_4 in_num, T_Key* in_key)
	  {
		  return del_appendix(1, in_num, in_key, 1);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      del_dsk
	  // ����:      �ڴ洢��������ɾ��ָ�����Ĵ��̸�����Ϣ
	  // ���:
	  //              in_num: ��Ҫɾ����key·�����
	  //              in_key: ��Ҫɾ����key�б�
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ���ô˺�������ɾ������key·������ĩ������ϵĴ��̸�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 del_dsk(const var_4 in_num, T_Key* in_key)
	  {
		  return del_appendix(1, in_num, in_key, 2);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      del_mix
	  // ����:      �ڴ洢��������ɾ��ָ�������ڴ��Լ����̸�����Ϣ
	  // ���:
	  //              in_num: ��Ҫɾ����key·�����
	  //              in_key: ��Ҫɾ����key�б�
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ���ô˺�������ɾ������key·������ĩ������ϵ��ڴ��Լ����̸�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 del_mix(const var_4 in_num, T_Key* in_key)
	  {
		  return del_appendix(1, in_num, in_key, 3);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      update_mem
	  // ����:      ����ָ��key·���е������ڴ渽����Ϣ
	  // ���:
	  //              in_num: ��Ҫ���µ�key·�����
	  //              in_key: ��Ҫ���µ�key·���б�
	  //              in_mem: ����Ҫ���µ�key��Ӧ���ڴ渽����Ϣ
	  //          in_mem_len: ����Ҫ���µ�key��Ӧ���ڴ渽����Ϣ����
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����ڸ��µ�key·���б���������һ��key������,������·���е�����key�����ᱻ����,
	  //          �����㸽����ϢΪNULL�򳤶�Ϊ0,�򲻻���¸ý��ĸ�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 update_mem(const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len)
	  {
		  return update(1, in_num, in_key, in_mem, in_mem_len, NULL, NULL);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      update_dsk
	  // ����:      ����ָ��key·���е����д��̸�����Ϣ
	  // ���:
	  //              in_num: ��Ҫ���µ�key·�����
	  //              in_key: ��Ҫ���µ�key·���б�
	  //              in_dsk: ����Ҫ���µ�key��Ӧ�Ĵ��̸�����Ϣ
	  //          in_dsk_len: ����Ҫ���µ�key��Ӧ�Ĵ��̸�����Ϣ����
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����ڸ��µ�key·���б���������һ��key������,������·���е�����key�����ᱻ����,
	  //          �����㸽����ϢΪNULL�򳤶�Ϊ0,�򲻻���¸ý��ĸ�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 update_dsk(const var_4 in_num, const T_Key* in_key, const var_1** in_dsk, const var_4* in_dsk_len)
	  {
		  return update(1, in_num, in_key, NULL, NULL, in_dsk, in_dsk_len);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      update_mix
	  // ����:      ����ָ��key·���е������ڴ��Լ����̸�����Ϣ
	  // ���:
	  //              in_num: ��Ҫ���µ�key·�����
	  //              in_key: ��Ҫ���µ�key·���б�
	  //              in_mem: ����Ҫ���µ�key��Ӧ���ڴ渽����Ϣ
	  //          in_mem_len: ����Ҫ���µ�key��Ӧ���ڴ渽����Ϣ����
	  //              in_dsk: ����Ҫ���µ�key��Ӧ�Ĵ��̸�����Ϣ
	  //          in_dsk_len: ����Ҫ���µ�key��Ӧ�Ĵ��̸�����Ϣ����
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����ڸ��µ�key·���б���������һ��key������,������·���е�����key�����ᱻ����,
	  //          �����㸽����ϢΪNULL�򳤶�Ϊ0,�򲻻���¸ý��ĸ�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 update_mix(const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len, const var_1** in_dsk, const var_4* in_dsk_len)
	  {
		  return update(1, in_num, in_key, in_mem, in_mem_len, in_dsk, in_dsk_len);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      save
	  // ����:      ����洢�������������ݵ�����
	  // ���:
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 save()
	  {
		  if(m_str_sto_lib_idx[0] == 0)
			  return 0;

		  AVL_TREE_NODE<T_Key> *pst_tmp_atn = NULL;

		  char str_tmp[256], str_cge[256];
		  if(_snprintf(str_tmp, 256, "%s.tmp", m_str_sto_lib_idx) <= 0)
			  return -172;

		  if(_snprintf(str_cge, 256, "%s.cge", m_str_sto_lib_idx) <= 0)
			  return -173;

		  FILE *fp_w = fopen(str_tmp, "wb");
		  if(fp_w == NULL)
			  return -174;

		  var_4 ret = 0, hash_cur = 0;

		  SUBTREE_NODE<T_Key> *pst_hsn = NULL;

		  try
		  {
			  for(hash_cur = 0; hash_cur < m_hash_tln_size; hash_cur++)
			  {
				  tree_node_lock(hash_cur, 1);
				  pst_hsn = &m_psta_hash_table[hash_cur].st_stn;
				  if(pst_hsn->pst_sub == NULL)
				  {
					  tree_node_unlock(hash_cur);
					  continue;
				  }

				  if(get_tree_del_num(pst_hsn->pst_sub) > 0)
				  {
					  pst_tmp_atn = NULL;
					  arrange_tree(-1, pst_hsn->pst_sub, pst_tmp_atn);
#ifdef DEBUG_PRINTF
					  printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
#endif
					  pst_hsn->pst_sub = pst_tmp_atn;

					  pst_hsn->tree_num = get_tree_num(pst_tmp_atn);

					  VERIFY_TREE_NUM(pst_tmp_atn, pst_hsn->tree_num);
					  if(pst_tmp_atn == NULL)
					  {
						  tree_node_unlock(hash_cur);
						  continue;
					  }
				  }
				  else
				  {
					  pst_tmp_atn = pst_hsn->pst_sub;

				  }

#ifdef DEBUG_PRINTF
				  printf("[[[[%d]]]]]\n", pst_hsn->tree_num);
#endif

				  ret = save_tree(NULL, pst_tmp_atn, fp_w);
				  tree_node_unlock(hash_cur);

				  if(ret != 0)
					  throw ret;

			  }

			  fclose(fp_w);
			  fp_w = NULL;

			  rename_file(str_tmp, str_cge);
			  rename_file(str_cge, m_str_sto_lib_idx);

			  m_inc_file_lock.lock();

			  if(m_fp_inc)
			  {
				  while(fseek(m_fp_inc, 0, SEEK_SET) != 0)
				  {
					  cp_sleep(10000);
					  printf("fseek failed!\n");
				  }

				  while(cp_change_file_size(m_fp_inc, 0) != 0)
				  {
					  cp_sleep(10000);
					  printf("cp_change_file_size!\n");
				  }				  
			  }

			  m_inc_file_lock.unlock();

		  }
		  catch (const var_4 err)
		  {
			  ret = err;
			  STORE_PRINTF_LOG("save idx error! ret:%d\n", ret);

			  fclose(fp_w);
			  fp_w = NULL;
		  }

		  return 0;
	  }

	  const var_4 save_tree(AVL_TREE_NODE<T_Key> *pst_parent, AVL_TREE_NODE<T_Key> *pst_root, FILE *fp_w)
	  {
		  var_4 ret = 0;
		  if(pst_root->pst_left)
		  {
			  ret = save_tree(pst_parent, pst_root->pst_left, fp_w);
			  if(ret != 0)
				  return ret;
		  }

		  if(pst_root->pst_right)
		  {
			  ret = save_tree(pst_parent, pst_root->pst_right, fp_w);
			  if(ret != 0)
				  return ret;
		  }

		  if(pst_root->st_stn.pst_sub)
		  {
			  ret = save_tree(pst_root, pst_root->st_stn.pst_sub, fp_w);
			  if(ret != 0)
				  return ret;
		  }

		  INDEX_FILE_NODE<T_Key>  st_ifn;
		  st_ifn.parent_key = pst_parent == NULL ? -1 : pst_parent->key;
		  st_ifn.key = pst_root->key;
		  st_ifn.level = pst_root->st_stn.level;

		  if(pst_parent == NULL)
		  {
			  assert(pst_root->st_stn.level == 0);
		  }
		  else
		  {
			  assert(pst_root->st_stn.level - 1 == pst_parent->st_stn.level);
		  }
#ifdef DEBUG_PRINTF
		  printf("["CP_PU64"],"CP_PU64"[%d],", st_ifn.parent_key, st_ifn.key, st_ifn.level);

		  if(st_ifn.level == 0)
		  {
			  printf("[[%d]]\n", pst_root->st_stn.tree_num);
		  }
#endif
		  data_node_lock(&pst_root->st_dcln);

		  assert(pst_root->st_stn.del_flg == 0);
		  st_ifn.mem_len = 0;
		  st_ifn.dsk_len = 0;

		  VERIFY_DATA_NODE(pst_root->st_stn.level, pst_root->key, pst_root, pst_root->ptr_data);

		  if(pst_root->ptr_data)
		  {
			  MEM_APPENDIX_NODE *pst_man = NULL;
			  DISK_INDEX_NODE *pst_din = NULL;

			  GET_DISK_IDX_HEAD<T_Key>(pst_root->ptr_data, pst_din);
			  GET_MAN_HEAD<T_Key>(pst_root->ptr_data, pst_man);

			  if(pst_din)
			  {
				  st_ifn.dsk_len = I_DIN_SIZE;
			  }

			  if(pst_man)
			  {
				  st_ifn.mem_len = pst_man->mem_len;
			  }

			  if(fwrite(&st_ifn, I_IFN_SIZE, 1, fp_w) != 1)
			  {
				  data_node_unlock(&pst_root->st_dcln);
				  STORE_PRINTF_LOG("fwrite ifn %d error!\n", I_IFN_SIZE);
				  return -176;
			  }

			  if(st_ifn.dsk_len > 0 && fwrite(pst_din, st_ifn.dsk_len, 1, fp_w) != 1)
			  {
				  data_node_unlock(&pst_root->st_dcln);
				  STORE_PRINTF_LOG("fwrite pst_din %d error!\n", st_ifn.dsk_len);
				  return -177;
			  }

			  if(st_ifn.mem_len > 0 && fwrite((char*)(pst_man + 1), st_ifn.mem_len, 1, fp_w) != 1)
			  {
				  data_node_unlock(&pst_root->st_dcln);
				  STORE_PRINTF_LOG("fwrite pst_man %d error!\n", st_ifn.mem_len);
				  return -177;
			  }
		  }
		  else
		  {
			  assert(pst_root->st_stn.del_flg == 0);
			  if(fwrite(&st_ifn, I_IFN_SIZE, 1, fp_w) != 1)
			  {
				  data_node_unlock(&pst_root->st_dcln);
				  STORE_PRINTF_LOG("fwrite ifn %d error!\n", I_IFN_SIZE);
				  return -178;
			  }
		  }

		  data_node_unlock(&pst_root->st_dcln);

		  return ret;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      load
	  // ����:      �Ӵ��̼��ش洢����������
	  // ���:
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 load()
	  {
		  if(m_str_sto_lib_idx[0] == 0)
			  return -150;

		  var_8 file_size = cp_get_file_size(m_str_sto_lib_idx), file_len = 0;
		  if(file_size < 0)
			  return -151;

		  if(file_size == 0)
			  return 0;

		  var_4 ret = 0, i = 0, count = 0;

		  FILE *fp_r = fopen(m_str_sto_lib_idx, "rb");
		  if(fp_r == NULL)
		  {
			  return -152;
		  }

		  INDEX_FILE_NODE<T_Key>  st_ifn;

		  DISK_INDEX_NODE arr_disk_idx[I_SYS_MAX_LEVEL], *psta_dsk_idx[I_SYS_MAX_LEVEL];
		  var_4 key_num = -1, arr_mem_len[I_SYS_MAX_LEVEL], arr_dsk_len[I_SYS_MAX_LEVEL];

		  T_Key arr_key[I_SYS_MAX_LEVEL];

		  char *ptra_mem_buf[I_SYS_MAX_LEVEL];

		  for(i = 0; i < m_max_level; i++)
		  {
			  ptra_mem_buf[i] = new char[m_one_inc_data_size];
			  if(ptra_mem_buf[i] == NULL)
			  {
				  for(--i; i >= 0; i--)
				  {
					  delete[] ptra_mem_buf[i];
				  }

				  return -153;
			  }
		  }

		  try
		  {
			  while(file_len < file_size)
			  {
				  if(fread(&st_ifn, I_IFN_SIZE, 1, fp_r) != 1)
				  {
					  throw -154;
				  }

				  file_len += I_IFN_SIZE;

				  if(st_ifn.level >= m_max_level)
					  throw -155;

				  if(st_ifn.mem_len < 0 || st_ifn.mem_len > m_one_inc_data_size)
					  throw -156;

				  if(st_ifn.dsk_len < 0 || st_ifn.dsk_len > I_DIN_SIZE)
					  throw -157;

				  if(key_num == -1)
				  {
					  key_num = st_ifn.level + 1;
					  count = key_num;
				  }

				  count--;

				  assert(count == st_ifn.level);

				  arr_key[st_ifn.level] = st_ifn.key;

				  if(st_ifn.dsk_len > 0 )
				  {
					  if(fread(arr_disk_idx + st_ifn.level, st_ifn.dsk_len, 1, fp_r) != 1)
					  {
						  throw -158;
					  }
					  psta_dsk_idx[st_ifn.level] = arr_disk_idx + st_ifn.level;

					  file_len += st_ifn.dsk_len;
				  }

				  if(st_ifn.mem_len > 0)
				  {
					  if(fread(ptra_mem_buf[st_ifn.level], st_ifn.mem_len, 1, fp_r) != 1)
					  {
						  throw -159;
					  }
					  file_len += st_ifn.mem_len;
				  }

				  arr_dsk_len[st_ifn.level] = st_ifn.dsk_len;
				  arr_mem_len[st_ifn.level] = st_ifn.mem_len;

				  if(st_ifn.level == 0)
				  {
					  ret = add(0, key_num, arr_key, (const char**)ptra_mem_buf, arr_mem_len, (const char**)psta_dsk_idx, arr_dsk_len);
					  if(ret < 0)
						  throw ret;

					  key_num = -1;
				  }
			  }

			  ret = 0;
		  }
		  catch (const var_4 err)
		  {
			  ret = err;
		  }

		  fclose(fp_r);
		  fp_r = NULL;

		  for(i = 0; i < m_max_level; i++)
		  {
			  if(ptra_mem_buf[i])
			  {
				  delete[] ptra_mem_buf[i];
			  }
		  }

		  return ret;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      query_lock
	  // ����:      ��ѯָ��key·���ϵ������ڴ��Լ����̸�����Ϣ����
	  // ���:
	  //              in_num: ��Ҫ��ѯ��key·�����
	  //              in_key: ��Ҫ��ѯ��key·���б�
	  //            is_write: �Ƿ���Ҫд����,Ĭ��Ϊֻ������
	  //
	  // ����:
	  //             out_idx: ���ز�ѯ�������������Ϣ
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     �˺������û�Է��ص��ڴ��Լ����̸�����Ϣ���м�������,�ڸ�����Ϣ��ʹ���ڼ䲻�ܱ��޸�;ʹ����ɺ�������query_mix_unlock�����ڴ��Լ����̸�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 query_lock(const var_4 in_num, const T_Key* in_key, const var_1** out_idx, const var_4 is_write = 0)
	  {
		  if(in_num < 1 || in_key == NULL || out_idx == NULL)
			  return -1;

		  if(in_num >m_max_level)
			  return -2;

		  var_4 ret = 0, i = 0, err_num = 0, lock_suf = (var_4)(in_key[0] % m_hash_tln_size);

		  tree_node_lock(lock_suf, is_write);

		  AVL_TREE_NODE<T_Key> *pst_root = m_psta_hash_table[lock_suf].st_stn.pst_sub, *pst_ret_atn = NULL;

		  if(pst_root == NULL)
		  {
			  tree_node_unlock(lock_suf);
			  return -3;
		  }

		  try
		  {
			  for(i = 0; i < in_num; i++)
			  {
				  ret = search_tree_node(pst_root, in_key[i], pst_ret_atn);
				  if(ret != 0)
				  {
					  err_num = i;
					  throw -4;
				  }

				  if(pst_ret_atn->st_stn.del_flg == 1)
				  {
					  err_num = i;
					  throw -5;
				  }

				  data_node_lock(&pst_ret_atn->st_dcln, is_write);

				  pst_root = pst_ret_atn->st_stn.pst_sub;

				  VERIFY_DATA_NODE(i, pst_ret_atn->key, pst_ret_atn, pst_ret_atn->ptr_data);
				  out_idx[i] = (var_1*)pst_ret_atn;

			  }

			  ret = 0;
		  }
		  catch (const var_4 err)
		  {
			  for(i = 0; i < err_num; i++)
			  {
				  pst_ret_atn = (AVL_TREE_NODE<T_Key> *)out_idx[i];
				  VERIFY_DATA_NODE(i, pst_ret_atn->key, pst_ret_atn, pst_ret_atn->ptr_data);
				  data_node_unlock(&pst_ret_atn->st_dcln);
			  }

			  tree_node_unlock(lock_suf);
			  ret = err;
		  }

		  return ret;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      query_unlock
	  // ����:      �Բ�ѯ���ĸ�����Ϣ���н�������
	  // ���:
	  //              in_num: ��Ҫ�����ĸ�����Ϣ����
	  //              in_idx: ��Ҫ�����ĸ�����Ϣ����
	  //            is_write: �Ƿ���Ҫд����,Ĭ��Ϊֻ������
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����query_mix_lock�����������ô˺������н�������
	  /////////////////////////////////////////////////////////////////////
	  const var_4 query_unlock(const var_4 in_num, const var_1** in_idx, const var_4 is_write = 0)
	  {
		  if(in_num < 1 || in_idx == NULL)
			  return -1;

		  AVL_TREE_NODE<T_Key> *pst_atn = NULL;

		  var_4 lock_suf = -1;

		  for(var_4 i = 0; i < in_num; i++)
		  {
			  if(in_idx[i] == NULL)
				  continue;

			  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];
			  VERIFY_DATA_NODE(pst_atn->st_stn.level, pst_atn->key, pst_atn, pst_atn->ptr_data);

			  if(lock_suf >= 0)
			  {
				  assert(lock_suf == pst_atn->lock_suf);
			  }
			  lock_suf = pst_atn->lock_suf;
			  data_node_unlock(&pst_atn->st_dcln);
		  }

		  assert(lock_suf >= 0);

		  tree_node_unlock(lock_suf);

		  return 0;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      query_single_lock
	  // ����:      ��ѯָ��key·������ĩ��key��������Ϣ����
	  // ���:
	  //              in_num: ��Ҫ��ѯ��key·�����
	  //              in_key: ��Ҫ��ѯ��key·���б�
	  //            is_write: �Ƿ���Ҫд����,Ĭ��Ϊֻ������
	  //
	  // ����:
	  //             out_idx: ���ض�Ӧ������Ϣ����
	  //
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     �˺������û�Է��ص��ڴ��Լ����̸�����Ϣ���м�������,�ڸ�����Ϣ��ʹ���ڼ䲻�ܱ��޸�;ʹ����ɺ�������query_mix_single_unlock�����ڴ��Լ����̸�����Ϣ
	  //////////////////////////////////////////////////////////////////////
	  const var_4 query_single_lock(const var_4 in_num, const T_Key* in_key, var_1*& out_idx, const var_4 is_write = 0)
	  {
		  if(in_num < 1 || in_key == NULL)
			  return -1;

		  if(in_num >m_max_level)
			  return -2;

		  var_4 ret = 0, i = 0, lock_suf = (var_4)(in_key[0] % m_hash_tln_size);

		  AVL_TREE_NODE<T_Key> *pst_ret_atn = NULL, *pst_root = NULL;

		  tree_node_lock(lock_suf, is_write);

		  pst_root = m_psta_hash_table[lock_suf].st_stn.pst_sub;

		  if(pst_root == NULL)
		  {
			  tree_node_unlock(lock_suf);
			  return -3;
		  }

		  for(i = 0; i < in_num; i++)
		  {
			  ret = search_tree_node(pst_root, in_key[i], pst_ret_atn);
			  if(ret != 0)
			  {
				  tree_node_unlock(lock_suf);
				  return -4;
			  }

			  if(pst_ret_atn->st_stn.del_flg == 1)
			  {
				  tree_node_unlock(lock_suf);
				  return -5;
			  }

			  pst_root = pst_ret_atn->st_stn.pst_sub;
		  }

		  VERIFY_DATA_NODE(in_num - 1, in_key[in_num - 1], pst_ret_atn, pst_ret_atn->ptr_data);
		  out_idx = (var_1*)pst_ret_atn;

		  data_node_lock(&pst_ret_atn->st_dcln, is_write);

		  return ret;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      query_single_unlock
	  // ����:      �Բ�ѯ������Ϣ��Ϣ���н�������
	  // ���:
	  //              in_idx: ��Ҫ������������Ϣ
	  //              is_write: �Ƿ���Ҫд����,Ĭ��Ϊֻ������
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ����query_mix_single_lock�����������ô˺������н�������
	  //////////////////////////////////////////////////////////////////////
	  const var_4 query_single_unlock(const var_1* in_idx, var_4 is_write = 0)
	  {
		  var_1 *ptra_idx[1];

		  ptra_idx[0] = (var_1*)in_idx;

		  return query_unlock(1, (const var_1**)ptra_idx, is_write);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      is_exist
	  // ����:      ��ѯָ��key·���ϵĽ���Ƿ񶼴���
	  // ���:
	  //              in_num: ��Ҫ��ѯ��key·�����
	  //              in_key: ��Ҫ��ѯ��key·���б�
	  //
	  // ����:
	  // ����ֵ:    �������ܲ��ҵ����������key�ļ���,��������0,���� key·��ΪABC,�洢�������ֻ����AB���򷵻�ֵΪ1��
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 is_exist(const var_4 in_num, const T_Key* in_key)
	  {
		  if(in_num <= 0 || in_key == NULL)
			  return -2;

		  var_4 ret = 0, i = -2, lock_suf = (var_4)(in_key[0] % m_hash_tln_size);

		  tree_node_lock(lock_suf);

		  AVL_TREE_NODE<T_Key> *pst_root = m_psta_hash_table[lock_suf].st_stn.pst_sub, *pst_ret_atn = NULL;
		  if(pst_root == NULL)
		  {
			  tree_node_unlock(lock_suf);
			  return -3;
		  }

		  for(i = 0; i < in_num; i++)
		  {
			  ret = search_tree_node(pst_root, in_key[i], pst_ret_atn);
			  if(ret != 0)
			  {
				  break;
			  }

			  if(pst_ret_atn->st_stn.del_flg == 1)
				  break;

			  pst_root = pst_ret_atn->st_stn.pst_sub;
		  }

		  tree_node_unlock(lock_suf);

		  return i - 1;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      modify_mem_single
	  // ����:      �Բ�ѯ�����ڴ渽����Ϣ�����޸Ĳ���
	  // ���:
	  //              in_idx: ��Ҫԭʼ���ڴ渽����Ϣ�б�
	  //       in_modify_buf: ���ĺ���ڴ渽����Ϣ�б�
	  //       in_modify_len: ���ĺ���ڴ渽����Ϣ�����б�
	  //           in_offset: �޸ĵ�ƫ��λ��,����˲���Ϊ0,��ʹ��in_modify_buf�������鸲��,����ʹ��in_modify_len��in_offsetλ�ø���
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ֻ�ܶ�query_mem_lock, query_dsk_lock, query_mix_lock, query_mem_single_lock, query_dsk_single_lock, query_mix_single_lock
	  //          �������صĽ�����в���,�ҵ������Ϻ�����ʱ�����ʹis_write=1,ͬʱ���뱣֤�ڵ������Ϻ�����Ӧ��unlock����ǰʹ�ô˺���
	  //////////////////////////////////////////////////////////////////////
	  const var_4 modify_mem_single(const var_4 in_num, T_Key *in_key, const var_1* in_idx, const var_1*in_modify_buf, const var_4 in_modify_len, const var_4 in_offset = 0)
	  {
		  if(in_idx == NULL || in_modify_buf == NULL || in_modify_len <= 0 || in_offset < 0)
			  return -1;

		  var_1 *ptra_mem[I_SYS_MAX_LEVEL], *ptra_idx[I_SYS_MAX_LEVEL];
		  var_4 arr_mem_len[I_SYS_MAX_LEVEL], arr_offset[I_SYS_MAX_LEVEL];
		  for(var_4 i = 0; i < in_num - 1; i++)
		  {
			  ptra_mem[i] = NULL;
			  ptra_idx[i] = NULL;
			  arr_mem_len[i] = 0;
			  arr_offset[i] = 0;
		  }

		  ptra_idx[in_num - 1] = (var_1*)in_idx;
		  ptra_mem[in_num - 1] = (var_1*)in_modify_buf;
		  arr_mem_len[in_num - 1] = in_modify_len;
		  arr_offset[in_num - 1] = in_offset;

		  return modify_mem(in_num, in_key, (const var_1**)ptra_idx, (const var_1**)ptra_mem, (const var_4*)arr_mem_len, (const var_4*)arr_offset);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      modify_mem
	  // ����:      �Բ�ѯ�����ڴ渽����Ϣ�����޸Ĳ���
	  // ���:
	  //              in_num: ��Ҫ�޸ĵĸ�����Ϣ����
	  //              in_idx: ��Ҫ�޸����ݵ�ԭʼ��������Ϣ
	  //       in_modify_buf: ���ĺ���ڴ渽����Ϣ�б�
	  //       in_modify_len: ���ĺ���ڴ渽����Ϣ�����б�
	  //           in_offset: �޸ĵ�ƫ��λ��,����˲���ΪNULL,��ʹ��in_modify_buf�������鸲��,����ʹ��in_modify_len��in_offsetλ�ø���
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ֻ�ܶ�query_mem_lock, query_dsk_lock, query_mix_lock, query_mem_single_lock, query_dsk_single_lock, query_mix_single_lock
	  //          �������صĽ�����в���,�ҵ������Ϻ�����ʱ�����ʹis_write=1,ͬʱ���뱣֤�ڵ������Ϻ�����Ӧ��unlock����ǰʹ�ô˺���
	  //////////////////////////////////////////////////////////////////////
	  const var_4 modify_mem(const var_4 in_num, T_Key *in_key, const var_1** in_idx, const var_1** in_modify_buf, const var_4* in_modify_len, const var_4* in_offset = NULL)
	  {
		  if(in_num < 1 || in_key == NULL || in_idx == NULL || in_modify_buf == NULL || in_modify_len == NULL)
			  return -1;

		  var_4 i = 0, ret = 0, err_num = 0;

		  MEM_APPENDIX_NODE    *pst_man = NULL;
		  AVL_TREE_NODE<T_Key> *pst_atn = NULL;

		  for(i = 0; i < in_num; i++)
		  {
			  if(in_idx[i] == NULL)
			  {
				  err_num++;
				  continue;
			  }
			  if(in_modify_buf[i] == NULL || in_modify_len[i] <= 0)
			  {
				  err_num++;
				  continue;
			  }

			  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];
			  if(pst_atn->ptr_data == NULL)
				  continue;

			  VERIFY_DATA_NODE(i, in_key[i], pst_atn, pst_atn->ptr_data);

			  GET_MAN_HEAD<T_Key>(pst_atn->ptr_data, pst_man);
			  if(in_offset && in_offset[i] > (var_4)pst_man->mem_len)
			  {
				  return -2;
			  }
		  }

		  if(err_num >= in_num)
			  return -3;

		  ret = write_inc_file(3, in_num, in_key, in_modify_buf, in_modify_len, NULL, in_offset);
		  if(ret != 0)
			  return ret;

		  return modify_mem_data(in_num, in_idx, in_modify_buf, in_modify_len, in_offset);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      modify_mem_data
	  // ����:      �Բ�ѯ�����ڴ渽����Ϣ�����޸Ĳ������˲������ᱣ�浽���̣��������ٴ�����ʱ�ָ���ǰ������
	  // ���:
	  //              in_num: ��Ҫ�޸ĵĸ�����Ϣ����
	  //              in_idx: ��Ҫ�޸����ݵ�ԭʼ��������Ϣ
	  //       in_modify_buf: ���ĺ���ڴ渽����Ϣ�б�
	  //       in_modify_len: ���ĺ���ڴ渽����Ϣ�����б�
	  //           in_offset: �޸ĵ�ƫ��λ��,����˲���ΪNULL,��ʹ��in_modify_buf�������鸲��,����ʹ��in_modify_len��in_offsetλ�ø���
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ֻ�ܶ�query_mem_lock, query_dsk_lock, query_mix_lock, query_mem_single_lock, query_dsk_single_lock, query_mix_single_lock
	  //          �������صĽ�����в���,�ҵ������Ϻ�����ʱ�����ʹis_write=1,ͬʱ���뱣֤�ڵ������Ϻ�����Ӧ��unlock����ǰʹ�ô˺���
	  const var_4 modify_mem_data(const var_4 in_num, const var_1** in_idx, const var_1** in_modify_buf, const var_4* in_modify_len, const var_4* in_offset)
	  {
		  if(in_num < 1 || in_idx == NULL || in_modify_buf == NULL || in_modify_len == NULL)
			  return -71;

		  var_4 i = 0;
		  AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;
		  MEM_APPENDIX_NODE         *pst_man = NULL;
		  AVL_TREE_NODE<T_Key>      *pst_atn = NULL;

		  bool     arr_flg[I_SYS_MAX_LEVEL];
		  char *ptra_aoc_data[I_SYS_MAX_LEVEL], *ptr_tmp_mem = NULL;

		  for(i = 0; i < in_num; i++)
		  {
			  ptra_aoc_data[i] = NULL;
			  arr_flg[i] = false;
			  if(in_idx[i] == NULL || in_modify_buf[i] == NULL || in_modify_len[i] <= 0)
				  continue;

			  if(in_modify_len[i] > I_MEMORY_APPENDIX_SIZE)
				  return -72;

			  if(in_offset && in_offset[i] < 0)
			  {
				  return -73;
			  }

			  arr_flg[i] = true;
			  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];
			  if(pst_atn->ptr_data == NULL)
			  {
				  if(in_offset && in_offset[i] != 0)
				  {
					  return -74;
				  }
				  continue;
			  }

			  VERIFY_DATA_NODE(i, pst_atn->key, pst_atn, pst_atn->ptr_data);

			  GET_AMBN_HEAD(pst_atn->ptr_data, pst_ambn);
			  if(pst_ambn->mem_flg != 1)
			  {
				  if(in_offset && in_offset[i] != 0)
				  {
					  return -75;
				  }

				  continue;
			  }

			  GET_MAN_HEAD<T_Key>(pst_atn->ptr_data, pst_man);
			  if(in_offset && in_offset[i] > 0)
			  {
				  if(in_offset[i] + in_modify_len[i] > (var_4)pst_man->mem_len)
					  return -76;
			  }
		  }

		  var_4 len = 0, off = 0, ret = 0, new_mem_size = 0;
		  DISK_INDEX_NODE *pst_din = NULL;

		  try
		  {
			  for(i = 0; i < in_num; i++)
			  {
				  if(!arr_flg[i])
					  continue;

				  if(in_offset && in_offset[i] > 0)
				  {
					  off = in_offset[i];

					  GET_MAN_HEAD<T_Key>(pst_atn->ptr_data, pst_man);

					  len = pst_man->mem_len;
					  ptr_tmp_mem = (char*)(pst_man + 1);
				  }
				  else
				  {
					  off = 0;
					  len = in_modify_len[i];
					  ptr_tmp_mem = (char*)in_modify_buf[i];
				  }

				  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];
				  pst_din = NULL;
				  if(pst_atn->ptr_data)
				  {  
					  GET_DISK_IDX_HEAD<T_Key>(pst_atn->ptr_data, pst_din);

					  GET_ALLOC_DATA_SIZE<T_Key>(ptr_tmp_mem, len, pst_din, new_mem_size);
					  if((var_4)pst_ambn->size >= new_mem_size)
						  continue;
				  }

				  ptra_aoc_data[i] = (char*)alloc_data(pst_atn->key, pst_atn, ptr_tmp_mem, len, pst_din);
				  if(ptra_aoc_data[i] == NULL)
				  {
					  throw -77;
				  }

				  if(off > 0)
				  {
					  memcpy(ptra_aoc_data[i] + off, in_modify_buf[i], in_modify_len[i]);
				  }
			  }

			  for(i = 0; i < in_num; i++)
			  {
				  if(!arr_flg[i])
					  continue;

				  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];

				  if(ptra_aoc_data[i] != NULL)
				  {
					  char *ptr_old = pst_atn->ptr_data;
					  pst_atn->ptr_data = ptra_aoc_data[i];

					  free_data(ptr_old);
					  continue;
				  }

				  GET_MAN_HEAD<T_Key>(pst_atn->ptr_data, pst_man);
				  char *ptr = (char*)(pst_man + 1);

				  memcpy((void*)(ptr + off), in_modify_buf[i], in_modify_len[i]);
			  }
		  }
		  catch (const var_4 err)
		  {
			  ret = err;

			  for(i = 0; i < in_num; i++)
			  {
				  if(ptra_aoc_data[i])
				  {
					  free_data(ptra_aoc_data[i]);
				  }
			  }
		  }

		  return ret;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      get_single_mem_data
	  // ����:      ʹ�ò�ѯ����������Ϣȡ���������ڴ�����
	  // ���:
	  //              in_num: �Ѿ���ȡ�Ĵ�����Ϣ����
	  //              in_idx: �Ѿ���ȡ�Ĵ�����Ϣ�б�
	  //
	  // ����:
	  //         out_mem_buf: ���ص�������Ϣָ�����ڴ渽����Ϣ�ĵ�ַ
	  //         out_mem_len: ���ص��ڴ渽����Ϣ�����ݳ����б�
	  //
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 get_single_mem_data(const var_1* in_idx, var_1 *&out_mem_buf, var_4& out_mem_len)
	  {
		  var_1 *ptra_idx[1], *ptra_mem[1];
		  var_4 arr_mem_len[1];

		  ptra_idx[0] = (var_1*)in_idx;

		  var_4 ret = get_mem_data(1, (const var_1**)ptra_idx, (const var_1**)ptra_mem, arr_mem_len);
		  if(ret != 0)
			  return ret;

		  out_mem_buf = ptra_mem[0];
		  out_mem_len = arr_mem_len[0];
		  return 0;
	  }


	  //////////////////////////////////////////////////////////////////////
	  // ����:      get_mem_data
	  // ����:      ʹ�ò�ѯ����������Ϣȡ���������ڴ�����
	  // ���:
	  //              in_num: �Ѿ���ȡ�Ĵ�����Ϣ����
	  //              in_idx: �Ѿ���ȡ�Ĵ�����Ϣ�б�
	  //
	  // ����:
	  //         out_mem_buf: ���ص�������Ϣָ�����ڴ渽����Ϣ�ĵ�ַ
	  //         out_mem_len: ���ص��ڴ渽����Ϣ�����ݳ����б�
	  //
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 get_mem_data(const var_4 in_num, const var_1** in_idx, const var_1** out_mem_buf, var_4* out_mem_len)
	  {
		  if(in_num < 1 || in_idx == NULL || out_mem_buf == NULL || out_mem_len == NULL)
			  return -1;

		  var_4 i = 0, err_num = 0;

		  MEM_APPENDIX_NODE *pst_man = NULL;
		  AVL_TREE_NODE<T_Key> *pst_atn = NULL;

		  for(i = 0; i < in_num; i++)
		  {
			  out_mem_buf[i] = NULL;
			  out_mem_len[i] = 0;
			  if(in_idx[i] == NULL)
			  {
				  err_num++;
				  continue;
			  }

			  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];
			  if(pst_atn->ptr_data == NULL)
			  {
				  err_num++;
				  STORE_PRINTF_LOG("get_mem_data NULL!\n");
				  continue;
			  }

			  GET_MAN_HEAD<T_Key>(pst_atn->ptr_data, pst_man);
			  if(pst_man == NULL)
			  {
				  err_num++;
				  STORE_PRINTF_LOG("get_mem_data failed! mem:%p\n", pst_man);
				  continue;
			  }

			  out_mem_buf[i] =(var_1*)(pst_man + 1);
			  out_mem_len[i] = pst_man->mem_len;
		  }

		  if(err_num >= in_num)
			  return -2;

		  if(out_mem_buf[in_num - 1] == NULL)
		  {
			  STORE_PRINTF_LOG("out_mem_buf NULL\n");
		  }
		  return 0;
	  }

	  /////////////////////////////////////////////////////////////////////
	  // ����:      modify_dsk_single
	  // ����:      �Բ�ѯ�����ڴ渽����Ϣ�����޸Ĳ���
	  // ���:
	  //              in_num: ��Ҫ�޸ĵĸ�����Ϣ����
	  //              in_idx: ��Ҫԭʼ�Ĵ��̸�����Ϣ�б�
	  //       in_modify_buf: ���ĺ�Ĵ��̸�����Ϣ�б�
	  //       in_modify_len: ���ĺ�Ĵ��̸�����Ϣ�����б�
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ֻ�ܶ�query_mem_lock, query_dsk_lock, query_mix_lock, query_mem_single_lock, query_dsk_single_lock, query_mix_single_lock
	  //          �������صĽ�����в���,�ҵ������Ϻ�����ʱ�����ʹis_write=1,ͬʱ���뱣֤�ڵ������Ϻ�����Ӧ��unlock����ǰʹ�ô˺���
	  //////////////////////////////////////////////////////////////////////
	  const var_4 modify_dsk_single(const var_4 in_num, T_Key *in_key, const var_1* in_idx, const var_1*in_modify_buf, const var_4 in_modify_len)
	  {
		  if(in_idx == NULL || in_modify_buf == NULL || in_modify_len <= 0)
			  return -1;

		  var_1 *ptra_dsk[I_SYS_MAX_LEVEL], *ptra_idx[I_SYS_MAX_LEVEL];
		  var_4 arr_dsk_len[I_SYS_MAX_LEVEL];

		  for(var_4 i = 0; i < in_num - 1; i++)
		  {
			  ptra_dsk[i] = NULL;
			  ptra_idx[i] = NULL;
			  arr_dsk_len[i] = 0;
		  }

		  ptra_idx[in_num - 1] =(var_1*) in_idx;
		  ptra_dsk[in_num - 1] = (var_1*)in_modify_buf;
		  arr_dsk_len[in_num - 1] = in_modify_len;

		  return modify_dsk(in_num, in_key, (const var_1**)ptra_idx, (const var_1**)ptra_dsk, (const var_4*)arr_dsk_len);
	  }

	  /////////////////////////////////////////////////////////////////////
	  // ����:      modify_dsk
	  // ����:      �Բ�ѯ�����ڴ渽����Ϣ�����޸Ĳ���
	  // ���:
	  //              in_num: ��Ҫ�޸ĵĸ�����Ϣ����
	  //              in_idx: ��Ҫԭʼ�Ĵ��̸�����Ϣ�б�
	  //       in_modify_buf: ���ĺ�Ĵ��̸�����Ϣ�б�
	  //       in_modify_len: ���ĺ�Ĵ��̸�����Ϣ�����б�
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:     ֻ�ܶ�query_mem_lock, query_dsk_lock, query_mix_lock, query_mem_single_lock, query_dsk_single_lock, query_mix_single_lock
	  //          �������صĽ�����в���,�ҵ������Ϻ�����ʱ�����ʹis_write=1,ͬʱ���뱣֤�ڵ������Ϻ�����Ӧ��unlock����ǰʹ�ô˺���
	  //////////////////////////////////////////////////////////////////////
	  const var_4 modify_dsk(const var_4 in_num, T_Key *in_key, const var_1** in_idx, const var_1** in_modify_buf, const var_4* in_modify_len)
	  {
		  if(in_num < 1 || in_key == NULL || in_idx == NULL || in_modify_buf == NULL || in_modify_len == NULL)
			  return -1;

		  DISK_INDEX_NODE *psta_din[I_SYS_MAX_LEVEL], sta_din[I_SYS_MAX_LEVEL];

		  var_4 ret = 0, i = 0, err_num = 0;

		  for(i = 0; i < in_num; i++)
		  {
			  psta_din[i] = NULL;
			  if(in_idx[i] == NULL)
			  {
				  err_num++;
				  continue;
			  }

			  if(in_modify_buf[i] == NULL || in_modify_len[i] <= 0)
			  {
				  err_num++;
				  continue;
			  }

			  if(in_modify_len[i] >= m_one_inc_data_size)
				  return -2;

			  ret = m_disk_quota_man.dqm_write_data(sta_din[i].file_no, sta_din[i].off, (I_TKEY_SIZE * (i + 1)),
				  (var_1*)in_key, in_modify_len[i], (var_1*)in_modify_buf[i]);
			  if(ret != 0)
			  {
				  break;
			  }

			  psta_din[i] = sta_din + i;
		  }

		  if(err_num >= in_num)
			  return -3;

		  ret = write_inc_file(4, in_num, in_key, NULL, NULL, (const DISK_INDEX_NODE**)psta_din, NULL);
		  if(ret != 0)
			  return ret;

		  return modify_dsk_data(in_num, in_idx, (const DISK_INDEX_NODE**)psta_din);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      get_single_dsk_data
	  // ����:      ʹ�ò�ѯ���Ĵ��̸�����Ϣȡ�������Ĵ�������
	  // ���:
	  //              in_num: �Ѿ���ȡ�Ĵ�����Ϣ����
	  //              in_idx: �Ѿ���ȡ�Ĵ�����Ϣ�б�
	  //
	  // ����:
	  //         out_dsk_buf: ���صĴ��������б�
	  //         out_dsk_len: ���صĴ������ݳ����б�
	  //
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 get_single_dsk_data(const var_1* in_idx, var_1* out_dsk_buf, var_4& out_dsk_len)
	  {
		  var_1 *ptra_idx[1], *ptra_dsk[1];
		  var_4 arr_dsk_len[1];

		  ptra_idx[0] = (var_1*)in_idx;
		  ptra_dsk[0] = out_dsk_buf;

		  var_4 ret = get_dsk_data(1, (const var_1**)ptra_idx, (const var_1**)ptra_dsk, arr_dsk_len);
		  if(ret != 0)
			  return ret;

		  out_dsk_len = arr_dsk_len[0];
		  return 0;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      get_dsk_dat
	  // ����:      ʹ�ò�ѯ���Ĵ��̸�����Ϣȡ�������Ĵ�������
	  // ���:
	  //              in_num: �Ѿ���ȡ�Ĵ�����Ϣ����
	  //              in_idx: �Ѿ���ȡ�Ĵ�����Ϣ�б�
	  //
	  // ����:
	  //         out_dsk_buf: ���صĴ��������б�
	  //         out_dsk_len: ���صĴ������ݳ����б�
	  //
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 get_dsk_data(const var_4 in_num, const var_1** in_idx, const var_1** out_dsk_buf, var_4* out_dsk_len)
	  {
		  if(in_num < 1 || in_idx == NULL || out_dsk_buf == NULL || out_dsk_len == NULL)
			  return -1;

		  var_4 ret = 0, i = 0, key_len = 0, *pa_dsk_len = (var_4*)out_dsk_len, err_num = 0;
		  T_Key arr_key[I_SYS_MAX_LEVEL];

		  for(i = 0; i < in_num; i++)
		  {
			  out_dsk_len[i] = 0;

			  if(in_idx[i] == NULL)
			  {
				  err_num++;
				  continue;
			  }
			  if(out_dsk_buf[i] == NULL)
			  {
				  err_num++;
				  continue;
			  }
		  }

		  if(err_num >= in_num)
			  return -2;

		  AVL_TREE_NODE<T_Key> *pst_atn = NULL;
		  DISK_INDEX_NODE      *pst_din = NULL;

		  for(i = 0; i < in_num; i++)
		  {
			  pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];
			  if(pst_atn == NULL)
				  continue;

			  if(pst_atn->ptr_data == NULL)
			  {
				  err_num++;
				  continue;
			  }

			  GET_DISK_IDX_HEAD<T_Key>(pst_atn->ptr_data, pst_din);
			  if(pst_din == NULL)
			  {
				  err_num++;
				  continue;
			  }

			  ret = m_disk_quota_man.dqm_read_data(pst_din->file_no, pst_din->off, key_len, (var_1*)arr_key, pa_dsk_len[i], (var_1*)out_dsk_buf[i]);
			  if(ret != 0)
				  return -3;
		  }

		  return 0;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      travel_prepare
	  // ����:      Ϊ׼������ָ��key�µ���������ȡ�ñ������
	  // ���:
	  //              in_num: ָ��key·�������
	  //              in_key: ָ��key·�����б�
	  //        in_max_depth: ���ؾ�������������,Ĭ��Ϊ-1,��0����������洢������
	  //        is_write:     �Ƿ���Ľ�� 0:������1:д
	  // ����:
	  //          out_handle: ���غ�������ʱ��Ҫ�ľ��
	  //
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:      ������Ȼ���0; ����������������travel_key
	  /////////////////////////////////////////////////////////////////////
	  const var_4 travel_prepare(const var_4 in_num, const T_Key* in_key, var_vd*& out_handle, var_4 in_max_depth = -1, var_4 is_write = 0)
	  {
		  out_handle = NULL;

		  if(in_max_depth > m_max_level)
			  return -1;

		  if(in_max_depth != -1 && in_max_depth > 0)
		  {
			  if(in_num < 1 || in_key == NULL)
				  return -2;
		  }

		  var_4 ret = 0, i = 0;

		  TRAVEL_HEAD_INFO<T_Key>   *pst_thi = NULL;

		  if(m_travel_que.PopData_NB(pst_thi) != 0)
		  {
			  return -3;
		  }

		  pst_thi->is_write = is_write;
		  pst_thi->hash_cur = 0;
		  pst_thi->num = 0;
		  pst_thi->pst_cur = NULL;
		  pst_thi->pst_old_dcln = NULL;

		  pst_thi->old_lock_suf = -1;

		  pst_thi->type = -1;

		  if(in_max_depth == 0 && (in_num == 0 || in_key == NULL))
		  {
			  pst_thi->max_depth = 0;
			  assert(in_num == 0);
			  pst_thi->type = 1;
		  }
		  else if(in_max_depth >= 0)
		  {
			  pst_thi->old_lock_suf = (var_4)(in_key[0] % m_hash_tln_size);

			  tree_node_lock(pst_thi->old_lock_suf, is_write);
			  AVL_TREE_NODE<T_Key>  *pst_root = m_psta_hash_table[pst_thi->old_lock_suf].st_stn.pst_sub, *pst_ret_atn = NULL;

			  if(pst_root == NULL)
			  {
				  tree_node_unlock(pst_thi->old_lock_suf);

				  m_travel_que.PushData(pst_thi);
				  return -4;
			  }

			  for(i = 0; i < in_num; i++)
			  {
				  ret = search_tree_node(pst_root, in_key[i], pst_ret_atn);
				  if(ret != 0)
				  {
					  tree_node_unlock(pst_thi->old_lock_suf);
					  m_travel_que.PushData(pst_thi);
					  return -5;
				  }

				  pst_thi->arr_key[i] = in_key[i];

				  pst_root = pst_ret_atn->st_stn.pst_sub;
				  //pst_thi->pst_parent_tln = &pst_ret_atn->st_tln;
				  pst_thi->pst_cur = pst_ret_atn->st_stn.pst_sub;
			  }

			  pst_thi->num = in_num;

			  pst_thi->max_depth = in_max_depth + in_num;

			  assert(pst_thi->max_depth > 0);
			  pst_thi->type = 2;
		  }
		  else
		  {
			  pst_thi->max_depth = I_SYS_MAX_LEVEL;
			  assert(in_num == 0);
			  pst_thi->type = 3;
		  }

		  out_handle = (void*)pst_thi;

		  return 0;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      travel_key
	  // ����:      ʹ��ָ����������洢������
	  // ���:
	  //           in_handle: ָ��key·�������
	  //
	  // ����:
	  //             out_num: ��ǰ����key·�����
	  //             out_key: ��ǰ����key·��
	  //             out_mem: ��ǰ�����ڴ渽����Ϣ
	  //         out_mem_len: ��ǰ�����ڴ渽����Ϣ����
	  //             out_dsk: ��ǰ���Ĵ��̸�����Ϣ
	  //         out_dsk_len: ��ǰ���Ĵ��̸�����Ϣ����
	  //
	  // ����ֵ:    ����0 - ˵�����к������,��������ô˺�������; ���ش���0 - �Ѿ���ɱ���; ����С��0 - ��������
	  // ��ע:      ������Ȼ���0; ����������������travel_key
	  //////////////////////////////////////////////////////////////////////
	  const var_4 travel_key(const var_vd* in_handle, var_4& out_num, T_Key* out_key, var_1 *&out_idx)
	  {
		  if(in_handle == NULL)
			  return -1;

		  TRAVEL_HEAD_INFO<T_Key> *pst_thi = (TRAVEL_HEAD_INFO<T_Key>*)in_handle;

		  CStoreStack<SUBTREE_SATACK_NODE<T_Key> >  &subtree_stack = pst_thi->subtree_stack;
		  CStoreStack<AVL_TREE_NODE<T_Key>* >  &tree_stack = pst_thi->tree_stack;

		  SUBTREE_SATACK_NODE<T_Key>  st_ssn;
		  AVL_TREE_NODE<T_Key>   *pst_atn = pst_thi->pst_cur;

		  if(pst_thi->pst_old_dcln)
		  {
			  data_node_unlock(pst_thi->pst_old_dcln);
			  pst_thi->pst_old_dcln = NULL;
		  }

		  if(pst_atn || !tree_stack.IsEmpty())
		  {
			  while(pst_atn != NULL)
			  {
				  if(pst_atn->st_stn.level < (var_u4)pst_thi->max_depth && pst_atn->st_stn.pst_sub)
				  {
					  st_ssn.pst_cur = pst_atn->st_stn.pst_sub;
					  st_ssn.pst_parent = pst_atn;

					  if(subtree_stack.Push(st_ssn) != 0)
					  {
						  return -3;
					  }
				  }

				  data_node_lock(&pst_atn->st_dcln, pst_thi->is_write);

				  if(tree_stack.Push(pst_atn) != 0)
				  {
					  return -4;
				  }

				  pst_atn = pst_atn->pst_left;
			  }

			  pst_atn = tree_stack.Pop();

			  pst_thi->pst_old_dcln = &pst_atn->st_dcln;

			  VERIFY_DATA_NODE(pst_atn->st_stn.level, pst_atn->key, pst_atn, pst_atn->ptr_data);

			  pst_thi->pst_cur = pst_atn->pst_right;

			  if(pst_atn->st_stn.del_flg == 0)
			  {
				  out_idx = (var_1*)pst_atn;

				  for(var_4 i = 0; i < pst_thi->num; i++)
				  {
					  out_key[i] = pst_thi->arr_key[i];
				  }

				  assert(pst_thi->num < m_max_level);

				  out_num = pst_thi->num;
				  out_key[out_num] = pst_atn->key;
				  out_num++;

				  return 0;
			  }
			  else
			  {
				  out_num = 0;
			  }
		  }
		  else if(!subtree_stack.IsEmpty())
		  {
			  st_ssn = subtree_stack.Pop();

			  pst_thi->arr_key[st_ssn.pst_parent->st_stn.level] = st_ssn.pst_parent->key;
			  pst_thi->num = st_ssn.pst_parent->st_stn.level + 1;

			  assert((var_4)(st_ssn.pst_parent->st_stn.level) < m_max_level - 1);
			  pst_thi->pst_cur = st_ssn.pst_cur;

		  }
		  else
		  {
			  if(pst_thi->max_depth > 0 && pst_thi->max_depth != I_SYS_MAX_LEVEL)
				  return 1;

			  pst_thi->num = 0;

			  while(pst_thi->hash_cur < m_hash_tln_size)
			  {
				  if(pst_thi->old_lock_suf >= 0)
				  {
					  assert(pst_thi->old_lock_suf != pst_thi->hash_cur);
					  tree_node_unlock(pst_thi->old_lock_suf);
					  pst_thi->old_lock_suf = -1;
				  }

				  //pst_thi->pst_hash_tln = m_psta_hash_tln + pst_thi->hash_cur;

				  tree_node_lock(pst_thi->hash_cur);

				  if(m_psta_hash_table[pst_thi->hash_cur].st_stn.pst_sub)
				  {
					  pst_thi->pst_cur = m_psta_hash_table[pst_thi->hash_cur].st_stn.pst_sub;

					  pst_thi->old_lock_suf = pst_thi->hash_cur;
					  pst_thi->hash_cur ++;

					  break;
				  }

				  tree_node_unlock(pst_thi->hash_cur);
				  pst_thi->hash_cur ++;
			  }

			  if(pst_thi->hash_cur >= m_hash_tln_size)
				  return 2;
		  }

		  return travel_key((void*)pst_thi, out_num, out_key, out_idx);
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      travel_finish
	  // ����:      ������ɺ��ͷű������
	  // ���:
	  //           in_handle: ��Ҫ�ͷŵľ��
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:      ������Ȼ���0; ����������������travel_key
	  //////////////////////////////////////////////////////////////////////
	  const var_4 travel_finish(const var_vd* in_handle)
	  {
		  if(in_handle == NULL)
			  return -1;

		  TRAVEL_HEAD_INFO<T_Key> *pst_thi = (TRAVEL_HEAD_INFO<T_Key>*)in_handle;

		  SUBTREE_SATACK_NODE<T_Key>  st_ssn;
		  while(!pst_thi->subtree_stack.IsEmpty())
		  {
			  st_ssn = pst_thi->subtree_stack.Pop();
			  //tree_node_unlock(&st_ssn.pst_parent->st_tln);
		  }

		  AVL_TREE_NODE<T_Key>   *pst_tmp_atn = NULL;
		  while(!pst_thi->tree_stack.IsEmpty())
		  {
			  pst_tmp_atn = pst_thi->tree_stack.Pop();
			  data_node_unlock(&pst_tmp_atn->st_dcln);
		  }

		  if(pst_thi->pst_old_dcln)
		  {
			  data_node_unlock(pst_thi->pst_old_dcln);
			  pst_thi->pst_old_dcln = NULL;
		  }       

		  if(pst_thi->old_lock_suf >= 0)
		  {
			  tree_node_unlock(pst_thi->old_lock_suf);
			  pst_thi->old_lock_suf = -1;
		  }

		  pst_thi->is_write = -1;
		  m_travel_que.PushData(pst_thi);

		  return 0;
	  }

	  //////////////////////////////////////////////////////////////////////
	  // ����:      clear
	  // ����:      ���
	  // ���:
	  //
	  // ����:
	  // ����ֵ:    �ɹ�����0,���򷵻ش�����
	  // ��ע:
	  //////////////////////////////////////////////////////////////////////
	  const var_4 clear()
	  {
		  var_4 ret = 0, hash_cur = 0;

		  AVL_TREE_NODE<T_Key> *pst_del_atn = NULL;

		  for(hash_cur = 0; hash_cur < m_hash_tln_size; hash_cur++)
		  {
			  tree_node_lock(hash_cur, 1);

			  if(m_psta_hash_table[hash_cur].st_stn.pst_sub == NULL)
			  {
				  tree_node_unlock(hash_cur);
				  continue;
			  }

			  pst_del_atn = m_psta_hash_table[hash_cur].st_stn.pst_sub;
			  m_psta_hash_table[hash_cur].st_stn.pst_sub = NULL;
			  m_psta_hash_table[hash_cur].st_stn.tree_num = 0;
			  m_psta_hash_table[hash_cur].st_stn.del_flg = 0;

			  // wait_lock(&pst_hash_tln->tree_cln);

			  ret = del_tree(pst_del_atn);

			  tree_node_unlock(hash_cur);

			  if(ret != 0)
			  {
				  for(;;)
				  {
					  printf("del_tree failed, ret:%d\n", ret);
					  cp_sleep(10000);
				  }
			  }
		  }

		  m_inc_file_lock.lock();

		  if(m_fp_inc)
		  { 
			  while(fseek(m_fp_inc, 0, SEEK_SET) != 0)
			  {
				  cp_sleep(10000);
				  printf("fseek failed!\n");
			  }

			  while(cp_change_file_size(m_fp_inc, 0) != 0)
			  {
				  cp_sleep(10000);
				  printf("cp_change_file_size!\n");
			  }			 
		  }

		  while(access(m_str_sto_lib_idx, 00) == 0 && remove(m_str_sto_lib_idx) != 0)
		  {
			  cp_sleep(10000);
			  printf("remove :%s failed!\n", m_str_sto_lib_idx);
		  }
		  m_inc_file_lock.unlock();
		  return ret;
	  }

private:  
	const var_4 add(const var_1 type, const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len, const var_1** in_dsk, const var_4* in_dsk_len)
	{
		if(in_num < 1 || in_key == NULL)
			return -40;

		if(in_num < 1 || in_num > m_max_level)
			return -41;

		var_4 ret = 0, i = 0, start = 0;
		DISK_INDEX_NODE sta_din[I_SYS_MAX_LEVEL], *psta_din[I_SYS_MAX_LEVEL];
		var_4 lock_suf = (var_4)(in_key[0] % m_hash_tln_size);

		AVL_TREE_NODE<T_Key> *pst_ret_atn = NULL, *pst_head = NULL, *pst_aoc = NULL, *pst_parent_atn = NULL;

		char *ptra_aoc_data[I_SYS_MAX_LEVEL];

		for(i = 0; i < in_num; i++)
		{
			psta_din[i] = NULL;
			ptra_aoc_data[i] = NULL;
			if(in_mem_len && (in_mem_len[i] < 0 || in_mem_len[i] > I_MEMORY_APPENDIX_SIZE))
				return -42;

			if(in_dsk_len && in_dsk_len[i] >= m_one_inc_data_size)
				return -43;
		}

		tree_node_lock(lock_suf, 1);

		SUBTREE_NODE<T_Key> *pst_parent_sn = &m_psta_hash_table[lock_suf].st_stn;

		VERIFY_TREE_NUM(pst_parent_sn->pst_sub, pst_parent_sn->tree_num);

		try
		{
			for(start = 0; start < in_num; start++)
			{
				ret = search_tree_node(pst_parent_sn->pst_sub, in_key[start], pst_ret_atn);
				if(ret < 0)
				{
					if(pst_parent_sn->tree_num + 1 >= m_arr_tree_key_max[start])
					{
						throw -45;
					}

					break;
				}

				VERIFY_DATA_NODE(start, in_key[start], pst_ret_atn, (pst_ret_atn->ptr_data));

				VERIFY_TREE_NUM(pst_ret_atn->st_stn.pst_sub, pst_ret_atn->st_stn.tree_num);

				//pst_parent_tln = &pst_ret_atn->st_tln;
				pst_parent_sn = &pst_ret_atn->st_stn;
				pst_parent_atn = pst_ret_atn;
				if(pst_parent_sn->del_flg == 1)
				{
					if(pst_parent_sn->tree_num + 1 >= m_arr_tree_key_max[start])
					{
						throw -45;
					}

					break;
				}
			}

			if(start >= in_num)
			{
				throw 1;
			}

			for(i = in_num - 1; i >= start ; i--)
			{
				if(in_dsk && in_dsk_len && in_dsk[i] && in_dsk_len[i] > 0)
				{
					if(type > 0)
					{
						ret = m_disk_quota_man.dqm_write_data(sta_din[i].file_no, sta_din[i].off, I_TKEY_SIZE * (i + 1), (var_1*)in_key, in_dsk_len[i], (var_1*)in_dsk[i]);
						if(ret != 0)
						{
							printf("dqm_write_data failed!\n");
							throw -45;
						}

						psta_din[i] = sta_din + i;
					}
					else
					{
						psta_din[i] = (DISK_INDEX_NODE*)in_dsk[i];
					}
				}

				if(in_mem && in_mem_len && in_mem[i] && in_mem_len[i] > 0)
				{
					ptra_aoc_data[i] = (char*)alloc_data(in_key[i], NULL, (char*)in_mem[i], in_mem_len[i], psta_din[i]);
					if(ptra_aoc_data[i] == NULL)
					{
						throw -46;
					}
				}
				else if(psta_din[i] != NULL)
				{
					ptra_aoc_data[i] = (char*)alloc_data(in_key[i], NULL, NULL, 0, psta_din[i]);
					if(ptra_aoc_data[i] == NULL)
					{
						throw -47;
					}
				}
			}

			AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;

			var_4 aoc_start = start;
			if(pst_parent_sn->del_flg == 1)
			{
				aoc_start++;
			}

			for(i = in_num - 1; i >= aoc_start; i--)
			{
				pst_aoc = alloc_tree_node(i, in_key[i], ptra_aoc_data[i], NULL, lock_suf);
				if(pst_aoc == NULL)
				{
					throw -48;
				}

				if(pst_head == NULL)
				{
					pst_head = pst_aoc;
				}
				else
				{
					pst_aoc->st_stn.pst_sub = pst_head;
					pst_aoc->st_stn.tree_num++;

					pst_head = pst_aoc;
				}

				VERIFY_TREE_NUM(pst_head->st_stn.pst_sub, pst_head->st_stn.tree_num);
				GET_AMBN_HEAD(ptra_aoc_data[i], pst_ambn);
				pst_ambn->pst_atn = pst_aoc;
				VERIFY_DATA_NODE(i, pst_aoc->key, pst_aoc, ptra_aoc_data[i]);
			}

			//�����ļ�
			if(type > 0)
			{
				ret = write_inc_file(1, in_num, in_key, in_mem, in_mem_len, (const DISK_INDEX_NODE**)psta_din, NULL);
				if(ret != 0)
					throw ret;
			}

			if(pst_parent_sn->del_flg == 0)
			{
				ret = insert_tree_node(pst_parent_sn->pst_sub, pst_head, pst_parent_sn->pst_sub);
				assert(ret == 0);
			}
			else
			{
				GET_AMBN_HEAD(ptra_aoc_data[start], pst_ambn);
				pst_ambn->pst_atn = pst_parent_atn;

				VERIFY_DATA_NODE(start, in_key[start], pst_parent_atn, pst_parent_atn->ptr_data);

				data_node_lock(&pst_parent_atn->st_dcln, 1);
				pst_parent_atn->ptr_data = ptra_aoc_data[start];
				data_node_unlock(&pst_parent_atn->st_dcln);

				pst_parent_sn->pst_sub = pst_head;

				pst_parent_sn->del_flg = 0;
			}

			if(pst_parent_sn->pst_sub)
			{
				pst_parent_sn->tree_num++;
			}

			VERIFY_TREE_NUM(pst_parent_sn->pst_sub, pst_parent_sn->tree_num);

			ret = 0;
		}
		catch (const var_4 err)
		{
			while(pst_head)
			{
#ifdef DEBUG_ASSERT_ERROR
				assert(0);
#endif
				AVL_TREE_NODE<T_Key> *pst_free = pst_head;

				pst_head = pst_head->st_stn.pst_sub;

				free_tree_node(pst_free);
			}

			for(i = 0; i < in_num; i++)
			{
				if(ptra_aoc_data[i])
				{
					free_data(ptra_aoc_data[i]);
					ptra_aoc_data[i] = NULL;
				}
			}
			ret = err;
		}

		tree_node_unlock(lock_suf);
		return ret;
	}

	const var_4 update(const var_4 type, const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len, const var_1** in_dsk, const var_4* in_dsk_len)
	{
		if(in_num < 1 || in_key == NULL)
			return -51;

		if(in_num < 1 || in_num > m_max_level)
			return -52;

		var_4 lock_suf = (var_4)(in_key[0] % m_hash_tln_size), ret = 0, i = 0, err_num = 0;

		DISK_INDEX_NODE  sta_din[I_SYS_MAX_LEVEL], *psta_din[I_SYS_MAX_LEVEL];
		AVL_TREE_NODE<T_Key>  *pst_root = NULL, *psta_ret_atn[I_SYS_MAX_LEVEL];
		bool                       arr_flg[I_SYS_MAX_LEVEL];

		char *ptra_aoc_mem[I_SYS_MAX_LEVEL], *ptr_tmp_mem = NULL;

		for(i = 0; i < in_num; i++)
		{
			psta_din[i] = NULL;
			arr_flg[i] = false;
			psta_ret_atn[i] = NULL;
			ptra_aoc_mem[i] = NULL;

			if(in_mem_len && (in_mem_len[i] < 0 || in_mem_len[i] > I_MEMORY_APPENDIX_SIZE))
				return -53;

			if(in_dsk_len && in_dsk_len[i] >= m_one_inc_data_size)
				return -54;

			if(in_dsk && in_dsk_len && in_dsk[i] && in_dsk_len[i] > 0)
			{
				if(type > 0)
				{
					ret = m_disk_quota_man.dqm_write_data(sta_din[i].file_no, sta_din[i].off, (I_TKEY_SIZE * (i + 1)), (var_1*)in_key, in_dsk_len[i], (var_1*)in_dsk[i]);
					if(ret != 0)
					{
						return -55;
					}

					psta_din[i] = sta_din + i;
				}
				else
				{
					psta_din[i] = (DISK_INDEX_NODE*)in_dsk[i];
				}
			}

			if((in_mem == NULL || in_mem_len == NULL || in_mem[i] == NULL || in_mem_len[0] <= 0) &&	psta_din[i] == NULL)
			{
				err_num++;
			}		
		}

		if(err_num >= in_num)
			return -56;

		var_4 mem_size = 0;

		AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;


		tree_node_lock(lock_suf, 1);
		pst_root = m_psta_hash_table[lock_suf].st_stn.pst_sub;

		try
		{
			if(pst_root == NULL)
			{
				throw -57;
			}

			for(i = 0; i < in_num; i++)
			{
				ret = search_tree_node(pst_root, in_key[i], psta_ret_atn[i]);
				if(ret != 0)
				{
					throw -58;
				}

				VERIFY_DATA_NODE(i, in_key[i], psta_ret_atn[i], psta_ret_atn[i]->ptr_data);

				if(psta_ret_atn[i]->st_stn.del_flg == 1)
				{
					throw -59;
				}

				pst_root = psta_ret_atn[i]->st_stn.pst_sub;

				ptr_tmp_mem = NULL;
				if((in_mem && in_mem_len && in_mem[i] && in_mem_len[i] > 0) )
				{
					ptr_tmp_mem = (char*)in_mem[i];
				}

				if(ptr_tmp_mem || psta_din[i])
				{
					arr_flg[i] = true;
					data_node_lock(&psta_ret_atn[i]->st_dcln);

					GET_AMBN_HEAD(psta_ret_atn[i]->ptr_data, pst_ambn);

					GET_ALLOC_DATA_SIZE<T_Key>(ptr_tmp_mem, in_mem_len[i], psta_din[i], mem_size);

					if(pst_ambn == NULL || (var_4)pst_ambn->size < mem_size)
					{
						ptra_aoc_mem[i] = (char*)alloc_data(in_key[i], psta_ret_atn[i], in_mem[i], in_mem_len[i], psta_din[i]);
						if(ptra_aoc_mem[i] == NULL)
						{
							throw -60;
						}
					}
				}

				//tree_node_unlock(pst_parent_tln);

				//pst_parent_tln = &psta_ret_atn[i]->st_tln;
			}

			//tree_node_unlock(pst_parent_tln);

			if(type > 0)
			{
				ret = write_inc_file(2, in_num, in_key, in_mem, in_mem_len, (const DISK_INDEX_NODE**)psta_din, NULL);
				if(ret != 0)
				{
					throw ret;
				}
			}

			for(i = 0; i < in_num; i++)
			{
				if(ptra_aoc_mem[i] != NULL)
				{
					var_1 *ptr_old = psta_ret_atn[i]->ptr_data;
					psta_ret_atn[i]->ptr_data = ptra_aoc_mem[i];

					free_data(ptr_old);
					continue;
				}
				else if(!arr_flg[i])
				{
					continue;
				}

				GET_AMBN_HEAD(psta_ret_atn[i]->ptr_data, pst_ambn);

				if(psta_din[i])
				{
					if(in_mem_len[i] > 0 && in_mem[i])
					{
						SET_DISK_IDX_DATA<T_Key>(psta_ret_atn[i]->ptr_data, psta_din[i]);
						SET_MEM_ADX_DATA<T_Key>(psta_ret_atn[i]->ptr_data, (var_1*)in_mem[i], in_mem_len[i]);
					}
					else
					{
						SET_DISK_IDX_DATA<T_Key>(psta_ret_atn[i]->ptr_data, psta_din[i]);
					}
				}
				else if(in_mem_len[i] > 0 && in_mem[i])
				{
					SET_MEM_ADX_DATA<T_Key>(psta_ret_atn[i]->ptr_data, (var_1*)in_mem[i], in_mem_len[i]);
				}
			}

			ret = 0;
		}
		catch (const var_4 err)
		{
			for(i = 0; i < in_num; i++)
			{
				if(ptra_aoc_mem[i])
				{
					free_data(ptra_aoc_mem[i]);
				}
			}

			ret = err;
		}

		for(i = 0; i < in_num; i++)
		{
			if(arr_flg[i])
			{
				data_node_unlock(&psta_ret_atn[i]->st_dcln);
			}
		}

		tree_node_unlock(lock_suf);

		return 0;
	}

	const var_4 del_key(const var_4 in_num, T_Key* in_key, const var_4 type)
	{
		if(in_key == NULL || in_num < 1)
			return -61;

		if(in_num < 1 || in_num > m_max_level)
			return -62;

		var_4 ret = 0, i = 0, lock_suf = (var_u4)(in_key[0] % m_hash_tln_size);

		AVL_TREE_NODE<T_Key> *pst_root = NULL, *pst_ret_atn = NULL, *pst_parent_atn = NULL;

		tree_node_lock(lock_suf, 1);

		try
		{
			pst_root = m_psta_hash_table[lock_suf].st_stn.pst_sub;

			if(pst_root == NULL)
			{
				throw -63;
			}

			for(i = 0; i < in_num; i++)
			{
				ret = search_tree_node(pst_root, in_key[i], pst_ret_atn);
				if(ret != 0)
				{
					throw -64;
				}

				VERIFY_DATA_NODE(i, in_key[i], pst_ret_atn, pst_ret_atn->ptr_data);

				//pst_parent_tln = &pst_ret_atn->st_tln;
				pst_root = pst_ret_atn->st_stn.pst_sub;
				pst_parent_atn = pst_ret_atn;
			}

			if(pst_parent_atn->st_stn.del_flg == 1)
			{
				throw -65;
			}

			if(type > 0)
			{
				ret = write_inc_file(-5, in_num, in_key, NULL, NULL, NULL, NULL);
				if(ret != 0)
				{
					throw ret;
				}
			}

			//pst_parent_tln->tree_num--;
			pst_parent_atn->st_stn.del_flg = 1;

			VERIFY_DATA_NODE(in_num - 1, in_key[in_num - 1], pst_ret_atn, pst_ret_atn->ptr_data);

			VERIFY_TREE_NUM(pst_root, pst_parent_atn->st_stn.tree_num);

			data_wait_lock(&pst_ret_atn->st_dcln);
			if(pst_ret_atn->ptr_data)
			{
				free_data(pst_ret_atn->ptr_data);
				pst_ret_atn->ptr_data = NULL;
			}

			pst_ret_atn = pst_parent_atn->st_stn.pst_sub;
			pst_parent_atn->st_stn.pst_sub = NULL;
			pst_parent_atn->st_stn.tree_num = 0;

			ret = del_tree(pst_ret_atn);

		}		
		catch (const var_4 err)
		{
			ret = err;
		}

		tree_node_unlock(lock_suf);

		return ret;
	}

	//ɾ��������Ϣ
	//type        1:ɾ���ڴ���Ϣ   2:ɾ��������Ϣ  3��ɾ�����и�����Ϣ
	const var_4 del_appendix(const var_4 oper_flg, const var_4 in_num, T_Key* in_key, const var_4 type)
	{
		if(in_key == NULL)
			return -111;

		if(in_num > m_max_level)
			return -112;

		var_4 ret = 0, i = 0, lock_suf = (var_4)(in_key[0] % m_hash_tln_size);

		tree_node_lock(lock_suf, 1);

		try
		{
			AVL_TREE_NODE<T_Key> *pst_root = m_psta_hash_table[lock_suf].st_stn.pst_sub, *pst_ret_atn = NULL, *pst_parent_atn = NULL;

			if(pst_root == NULL)
			{
				throw -113;
			}

			for(i = 0; i < in_num; i++)
			{
				ret = search_tree_node(pst_root, in_key[i], pst_ret_atn);
				if(ret != 0)
				{
					throw -114;
				}
				VERIFY_DATA_NODE(i, in_key[i], pst_ret_atn, pst_ret_atn->ptr_data);

				pst_root = pst_ret_atn->st_stn.pst_sub;
				pst_parent_atn = pst_ret_atn;
			}

			data_node_lock(&pst_parent_atn->st_dcln, 1);

			AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;

			GET_AMBN_HEAD(pst_ret_atn->ptr_data, pst_ambn);

			if(pst_ambn == NULL)
			{
				data_node_unlock(&pst_ret_atn->st_dcln);
				throw 0;
			}

			if(oper_flg > 0)
			{
				ret = write_inc_file(-1 * type, in_num, in_key, NULL, NULL, NULL, NULL);
				if(ret != 0)
				{
					data_node_unlock(&pst_ret_atn->st_dcln);
					throw ret;
				}
			}

			if(type == 2 || type == 3) //ɾ��������Ϣ
			{
				if(pst_ambn->dsk_flg == 1)
				{
					if(pst_ambn->mem_flg == 1)
					{
						MEM_APPENDIX_NODE *pst_man = NULL;
						DISK_INDEX_NODE *pst_din = NULL;
						GET_DISK_IDX_HEAD<T_Key>(pst_ret_atn->ptr_data, pst_din);

						GET_MAN_HEAD<T_Key>(pst_ret_atn->ptr_data, pst_man);
						//ptr_man->dsk_flg = 0;
						memmove(pst_din, pst_man, pst_man->mem_len + I_MAN_SIZE);
					}

					pst_ambn->dsk_flg = 0;
				}
			}

			if(type == 1 || type == 3)//ɾ���ڴ���Ϣ
			{
				pst_ambn->mem_flg = 0;
			}

			if(pst_ambn->dsk_flg == 0 && pst_ambn->mem_flg == 0)
			{
				free_data(pst_ret_atn->ptr_data);
				pst_ret_atn->ptr_data = NULL;
			}

			data_node_unlock(&pst_ret_atn->st_dcln);

			ret = 0;
		}		
		catch (const var_4 err)
		{
			ret = err;
		}

		tree_node_unlock(lock_suf);
		return ret;
	}

	const var_4 del_tree(AVL_TREE_NODE<T_Key> *&pst_root)
	{
		if(pst_root == NULL)
			return 0;

		var_4 ret = 0;

		AVL_TREE_NODE<T_Key> *pst_del = NULL;

		pst_del = pst_root;

		pst_root = NULL;

		if(pst_del->st_stn.pst_sub)
		{
			ret = del_tree(pst_del->st_stn.pst_sub);
		}

		data_wait_lock(&pst_del->st_dcln);

		if(pst_del->pst_left)
		{
			ret = del_tree(pst_del->pst_left);
		}

		if(pst_del->pst_right)
		{
			ret = del_tree(pst_del->pst_right);
		}

		free_tree_node(pst_del);

		return ret;
	}

	const var_4 modify_dsk_data(const var_4 in_num, const var_1** in_idx, const DISK_INDEX_NODE** in_dsk_idx)
	{
		if(in_num < 1 || in_idx == NULL || in_dsk_idx == NULL)
			return -81;

		var_4 i = 0, tmp_mem_len = 0;

		AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;
		AVL_TREE_NODE<T_Key>      *pst_atn = NULL;

		var_1 *ptra_aoc_data[I_SYS_MAX_LEVEL], *ptr_tmp_data = NULL;

		for(i = 0; i < in_num; i++)
		{
			ptra_aoc_data[i] = NULL;
			if(in_idx[i] == NULL || in_dsk_idx[i] == NULL)
				continue;

			pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];

			tmp_mem_len = 0;
			ptr_tmp_data = NULL;
			if(pst_atn->ptr_data)
			{
				VERIFY_DATA_NODE(i, pst_atn->key, pst_atn, pst_atn->ptr_data);

				GET_AMBN_HEAD(pst_atn->ptr_data, pst_ambn);
				if(pst_ambn->dsk_flg == 1)
				{
					continue;
				}

				if(pst_ambn->mem_flg == 1)
				{
					MEM_APPENDIX_NODE *pst_man = NULL;
					GET_MAN_HEAD<T_Key>(pst_atn->ptr_data, pst_man);

					if(pst_ambn->size - (I_AMBN_SIZE + I_MAN_SIZE + pst_man->mem_len) >= I_DIN_SIZE)
						continue;

					ptr_tmp_data = (char*)(pst_man + 1);
					tmp_mem_len = pst_man->mem_len;
				}
				else if((var_4)pst_ambn->size >= (I_AMBN_SIZE + I_DIN_SIZE))
				{
					continue;
				}
			}

			ptra_aoc_data[i] = (char*)alloc_data(pst_atn->key, pst_atn, ptr_tmp_data, tmp_mem_len, ((DISK_INDEX_NODE *)in_dsk_idx[i]));

			if(ptra_aoc_data[i] == NULL)
			{
				for(--i; i >= 0; i--)
				{
					free_data(ptra_aoc_data[i]);
				}
				return -84;
			}            
		}

		for(i = 0; i < in_num; i++)
		{
			if(in_idx[i] == NULL || in_dsk_idx[i] == NULL)
				continue;

			pst_atn = (AVL_TREE_NODE<T_Key> *)in_idx[i];

			if(ptra_aoc_data[i])
			{
				char *ptr_old = pst_atn->ptr_data;
				pst_atn->ptr_data = ptra_aoc_data[i];
				free_data(ptr_old);
				continue;
			}

			SET_DISK_IDX_DATA<T_Key>(pst_atn->ptr_data, (DISK_INDEX_NODE*)in_dsk_idx[i]);
		}

		return 0;
	}

	const var_4 load_inc_file(const var_1 max_level, const char *dat_file)
	{
		if(dat_file == NULL || max_level < 1)
			return -201;

		var_4 ret = 0;
		var_8 file_size = cp_get_file_size((char*)dat_file), file_len = 0;

		if(file_size < 0)
		{
			return -202;
		}

		if(file_size == 0)
			return 0;

		var_4 buf_size = 2 * m_one_inc_data_size, i = 0;

		if(file_size < (var_8)buf_size)
			buf_size = (var_4)file_size;

		char *ptr_buf = new char[buf_size];
		if(ptr_buf == NULL)
			return -203;

		char *ptra_mem_buf[I_SYS_MAX_LEVEL];

		for(i = 0; i < max_level; i++)
		{
			ptra_mem_buf[i] = new char[m_one_inc_data_size];
			if(ptra_mem_buf[i] == NULL)
			{
				for(--i; i >= 0; i--)
				{
					delete[] ptra_mem_buf[i];
				}

				delete[] ptr_buf;
				return -204;
			}
		}

		FILE *fp_r = fopen(dat_file, "rb+");

		try
		{
			if(fp_r == NULL)
			{
				throw -205;
			}

			if(fseek(fp_r, file_size - buf_size, SEEK_SET) != 0)
			{
				throw -206;
			}

			if(fread(ptr_buf, buf_size, 1, fp_r) != 1)
			{
				throw -207;
			}

			char *ptr_end =  ptr_buf + buf_size, *ptr = ptr_end - 1;

			while(ptr > ptr_buf)
			{
				if(*ptr == '@' && ptr_end - ptr >= 8 && *(var_u8*)ptr == I_STORE_FILE_MARK)
				{
					ptr += 8;
					break;
				}

				ptr--;
			}

			if(ptr >= ptr_buf)
			{
				file_size -= ptr_end - ptr;
			}
			else
			{
				throw - 208;
			}

			if(cp_change_file_size(fp_r, file_size) != 0)
			{
				throw -209;
			}

			if(fseek(fp_r, 0, SEEK_SET) != 0)
			{
				throw -210;
			}

			WRITE_INC_FILE_NODE st_wifn;

			T_Key arr_key[I_SYS_MAX_LEVEL];
			var_4 arr_mem_len[I_SYS_MAX_LEVEL], arr_dsk_len[I_SYS_MAX_LEVEL] ,arr_off[I_SYS_MAX_LEVEL];

			DISK_INDEX_NODE sta_din[I_SYS_MAX_LEVEL], *psta_dsk_idx[I_SYS_MAX_LEVEL];
			var_u8 end_mark = 0;

			while(file_len < file_size)
			{
				if(fread(&st_wifn, I_WIFN_SIZE, 1, fp_r) != 1)
				{
					throw -211;
				}

				if(st_wifn.len < 0 || st_wifn.len > m_one_inc_data_size)
					throw -212;

				if(fread(arr_key, st_wifn.num * I_TKEY_SIZE, 1, fp_r) != 1)
				{
					throw -213;
				}

				file_len += (I_WIFN_SIZE + st_wifn.num * I_TKEY_SIZE);

				if(fread(arr_mem_len, st_wifn.num * sizeof(var_4), 1, fp_r) != 1)
				{
					throw -214;
				}

				file_len += st_wifn.num * sizeof(var_4);
				if(fread(arr_dsk_len, st_wifn.num * sizeof(var_4), 1, fp_r) != 1)
				{
					throw -215;
				}

				file_len += st_wifn.num * sizeof(var_4);

				for(var_4 i = 0; i < st_wifn.num; i++)
				{
					psta_dsk_idx[i] = NULL;
					if(arr_dsk_len[i] > 0)
					{
						if(fread(sta_din + i, arr_dsk_len[i], 1, fp_r) != 1)
						{
							throw -216;
						}

						psta_dsk_idx[i] = sta_din + i;
					}

					if(arr_mem_len[i] > 0 && fread(ptra_mem_buf[i], arr_mem_len[i], 1, fp_r) != 1)
					{
						throw -217;
					}

					if(fread(arr_off + i, sizeof(var_4), 1, fp_r) != 1)
					{
						throw -218;
					}

					file_len += (arr_dsk_len[i] + arr_mem_len[i] + sizeof(var_4));
				}

				ret = -111;
				if(st_wifn.oper_flg == 1)
				{
					ret = add(0, st_wifn.num, arr_key, (const char**)ptra_mem_buf, arr_mem_len, (const char**)psta_dsk_idx, arr_dsk_len);
					if(ret < 0)
						throw ret;
				}
				else if(st_wifn.oper_flg == 2)
				{
					ret = update(0, st_wifn.num, arr_key, (const char**)ptra_mem_buf, arr_mem_len, (const char**)psta_dsk_idx, arr_dsk_len);
					if(ret != 0)
						throw ret;
				}
				else if(st_wifn.oper_flg == 3 || st_wifn.oper_flg == 4)
				{
					var_4 suf = (var_4)(arr_key[0] % m_hash_tln_size), i = 0;
					AVL_TREE_NODE<T_Key> *pst_root = NULL, *psta_ret_atn[I_SYS_MAX_LEVEL];

					pst_root = m_psta_hash_table[suf].st_stn.pst_sub;

					for(i = 0; i < st_wifn.num; i++)
					{
						ret = search_tree_node(pst_root, arr_key[i], psta_ret_atn[i]);
						if(ret != 0)
						{
							throw -220;
						}

						pst_root =psta_ret_atn[i]->st_stn.pst_sub;
					}

					if(st_wifn.oper_flg == 3)
					{
						ret = modify_mem_data(st_wifn.num, (const var_1**)psta_ret_atn, (const char**)ptra_mem_buf, arr_mem_len, arr_off);
					}
					else
					{
						ret = modify_dsk_data(st_wifn.num, (const var_1**)psta_ret_atn, (const DISK_INDEX_NODE**)psta_dsk_idx);
					}

					if(ret != 0)
						throw ret;

				}
				else if(st_wifn.oper_flg < 0 )
				{
					if(st_wifn.oper_flg == -5)
					{
						ret = del_key(st_wifn.num, arr_key, 0);
					}
					else
					{
						ret = del_appendix(0, st_wifn.num, arr_key, st_wifn.oper_flg * (-1));
					}

					if(ret != 0)
						throw ret;
				}

				if(fread(&end_mark, sizeof(var_u8), 1, fp_r) != 1)
				{
					throw -225;
				}

				if(end_mark != I_STORE_FILE_MARK)
					throw -226;

				file_len += 8;
			}

			ret = 1;
		}
		catch (const var_4 err)
		{
			ret = err;
		}

		if(fp_r)
		{
			fclose(fp_r);
			fp_r = NULL;
		}

		for(i = 0; i < max_level; i++)
		{
			delete[] ptra_mem_buf[i];
		}

		delete[] ptr_buf;
		ptr_buf = NULL;

		return ret;
	}


	//write_inc_file
	//oper_flg �������ͱ�ʶ��1:���  2:���� 3:�޸Ĳ����ֶ� -1:ɾ���ڴ���Ϣ -2:ɾ��������Ϣ -3:ɾ�����и�����Ϣ -5:ɾ�������
	const var_4 write_inc_file(const var_4 oper_flg, const var_4 in_num, const T_Key* in_key, const var_1** in_mem, const var_4* in_mem_len,
		const DISK_INDEX_NODE** in_dsk, const var_4 *in_mem_off)
	{
		if(in_num < 1 || in_key == NULL)
			return -130;

		if(in_num > m_max_level)
			return -131;

		if(m_fp_inc == NULL)
			return 0;

		var_4 ret = 0, i = 0;

#ifdef _PRINTF_LOG_FILE_
		char str_tmp[100];
		var_4 tmp_len = 0;
		for(i = 0;i < in_num;i++)
		{
			tmp_len += _snprintf(str_tmp + tmp_len, 100 - tmp_len, CP_PU64",", in_key[i]);
		}

		m_log.Log(false, "oper_flg:%d,in_num:%d,%.*s\n", oper_flg, in_num, tmp_len, str_tmp);
#endif 

		WRITE_INC_FILE_NODE st_wifn;

		st_wifn.len = 0;
		st_wifn.num = in_num;
		st_wifn.oper_flg = oper_flg;

		var_4 arr_dsk_len[I_SYS_MAX_LEVEL], arr_mem_len[I_SYS_MAX_LEVEL], arr_mem_off[I_SYS_MAX_LEVEL];

		for(i = 0; i < in_num; i++)
		{
			arr_dsk_len[i] = 0;

			if(in_dsk && in_dsk[i])
			{
				arr_dsk_len[i] = I_DIN_SIZE;
			}

			arr_mem_len[i] = 0;
			if(in_mem_len && in_mem && in_mem_len[i] > 0 && in_mem[i])
			{
				arr_mem_len[i] = in_mem_len[i];
			}
			arr_mem_off[i] = 0;
			if(in_mem_off)
			{
				arr_mem_off[i] = in_mem_off[i];
			}

			st_wifn.len += (sizeof(var_4) * 3);
			st_wifn.len += arr_dsk_len[i];
			st_wifn.len += arr_mem_len[i];
		}

		m_inc_file_lock.lock();

		var_8 off = ftell(m_fp_inc);
		try
		{
			if(fwrite(&st_wifn, I_WIFN_SIZE, 1, m_fp_inc) != 1)
			{
				throw -132;
			}

			if(fwrite(in_key, in_num * I_TKEY_SIZE, 1, m_fp_inc) != 1)
			{
				throw -133;
			}

			if(fwrite(arr_mem_len, in_num * sizeof(var_4), 1, m_fp_inc) != 1)
			{
				throw -134;
			}

			if(fwrite(arr_dsk_len, in_num * sizeof(var_4), 1, m_fp_inc) != 1)
			{
				throw -135;
			}

			for(i = 0; i < in_num; i++)
			{	
				if(arr_dsk_len[i] > 0 && fwrite(in_dsk[i], arr_dsk_len[i], 1, m_fp_inc) != 1)
				{
					throw -136;
				}

				if(arr_mem_len[i] > 0 && fwrite(in_mem[i], arr_mem_len[i], 1, m_fp_inc) != 1)
				{
					throw -137;
				}

				if(fwrite(arr_mem_off + i, sizeof(var_4), 1, m_fp_inc) != 1)
				{
					throw -138;
				}	
			}

			if(fwrite(&I_STORE_FILE_MARK, 8, 1, m_fp_inc) != 1)
			{
				throw -139;
			}	

			while(fflush(m_fp_inc) != 0)
			{
				cp_sleep(10000);
				printf("fflush failed!\n");
			}
		}		
		catch (const var_4 err)
		{
			ret = err;

			while(fseek(m_fp_inc, off, SEEK_SET) != 0)
			{
				cp_sleep(10000);
				printf("fseek %ld failed!\n", off);
			}

			while(cp_change_file_size(m_fp_inc, off) != 0)
			{
				cp_sleep(10000);
				printf("cp_change_file_size %ld failed!\n", off);
			}
		}

		m_inc_file_lock.unlock();

		return ret;
	}

	//�����ڴ�
	const var_4 arrange_memory(const var_4 min_free)
	{
		var_4 num = 0, max = 0;

		AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;

		const var_4 I_MEM_MAX = 30000;

		char *ptra_mem[I_MEM_MAX], *ptr_mem = NULL;

		while(m_use_mem_list.Get(ptr_mem) == 0 && max < I_MEM_MAX)
		{
			ptra_mem[max] = ptr_mem;
			max++;
		}

		char *ptr_new = NULL;

		for(var_4 i = 0; i < max; i++)
		{	
			char *ptr = ptra_mem[i], *ptr_end = ptra_mem[i] + m_one_aoc_mem_size;

			while(ptr < ptr_end)
			{
				if(ptr + I_AMBN_SIZE >ptr_end)
					break;

				pst_ambn = (AOC_MEM_BLOCK_NODE<T_Key> *)ptr;

				ptr += pst_ambn->size;

				if(pst_ambn->pst_atn == NULL)
				{
					num++;
				}
			}

			if(num <= min_free)
			{
				m_use_mem_list.Add(ptra_mem[i]);
				continue;
			}

			ptr = ptra_mem[i];
			ptr_end = ptra_mem[i] + m_one_aoc_mem_size;

			while(ptr < ptr_end)
			{
				if(ptr + I_AMBN_SIZE >ptr_end)
					break;

				pst_ambn = (AOC_MEM_BLOCK_NODE<T_Key> *)ptr;

				//if(ptr_ambn->dsk_flg == 2)
				//	break;

				if(pst_ambn->pst_atn)
				{
					ptr_new = (char*)alloc_block(pst_ambn->size);
					if(ptr_new)
						break;

					memcpy(ptr_new, pst_ambn, pst_ambn->size);

					data_node_lock(&pst_ambn->pst_atn->st_dcln, 1);
					//wait_lock(&ptr_ambn->ptr_tree->tln.data_cln);
					pst_ambn->pst_atn->ptr_data = ptr_new;

					data_node_unlock(&pst_ambn->pst_atn->st_dcln);
				}

				ptr += pst_ambn->size;				
			}

			if(ptra_mem[i])
			{
				delete[] ptra_mem[i];
				ptra_mem[i] = NULL;
			}
		}

		return 0;
	}

	void *alloc_block(const var_u4 size)
	{
		m_aoc_mem_lock.lock();

		if(m_aoc_mem_off + size >= m_one_aoc_mem_size || m_ptr_aoc_mem == NULL)
		{
			if(m_ptr_aoc_mem)
			{
				var_4 len = (var_4)(m_one_aoc_mem_size - m_aoc_mem_off);
				if(len > I_AMBN_SIZE)
				{
					char *ptr = m_ptr_aoc_mem + m_aoc_mem_off;	
					AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = (AOC_MEM_BLOCK_NODE<T_Key>*)ptr;

					pst_ambn->pst_atn = NULL;
					pst_ambn->dsk_flg = 0;
					pst_ambn->mem_flg = 0;
					pst_ambn->size = len;
				}

				m_use_mem_list.Add(m_ptr_aoc_mem);
			}

			m_ptr_aoc_mem = new char[m_one_aoc_mem_size] ;
			if(m_ptr_aoc_mem == NULL)
			{
				m_aoc_mem_lock.unlock();

				for(;;)
				{
					cp_sleep(10000);
					printf("alloc_block "CP_PU64" failed!\n", m_one_aoc_mem_size);					
				} 	

				return NULL;
			}

			m_aoc_mem_off = 0;
		}

		char *ptr_mem = m_ptr_aoc_mem + m_aoc_mem_off;		
		m_aoc_mem_off += size;
		m_aoc_mem_lock.unlock();

		return ptr_mem;
	}

	const var_4 free_data(char* ptr_data)
	{
		if(ptr_data == NULL)
			return -1;

		AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;

		GET_AMBN_HEAD(ptr_data, pst_ambn);

		VERIFY_DATA_NODE(pst_ambn->pst_atn->st_stn.level, pst_ambn->pst_atn->key, pst_ambn->pst_atn, ptr_data);

		pst_ambn->pst_atn = NULL;

		return 0;
	}

	void *alloc_data(T_Key key, AVL_TREE_NODE<T_Key> *pst_atn, const var_1 *ptr_in_mem, const var_4 in_mem_len, DISK_INDEX_NODE *pst_in_din)
	{
		var_4 alloc_size = 0;
		GET_ALLOC_DATA_SIZE<T_Key>(ptr_in_mem, in_mem_len, pst_in_din, alloc_size);
#ifdef DEBUG_ASSERT_ERROR
		alloc_size += sizeof(T_Key);
#endif
		char *ptr_data = (char*)alloc_block(alloc_size);
		if(ptr_data == NULL)
		{
			return NULL;
		}

		AOC_MEM_BLOCK_NODE<T_Key> *pst_ambn = NULL;
		GET_AMBN_HEAD(ptr_data, pst_ambn);

#ifdef DEBUG_ASSERT_ERROR
		pst_ambn->key = key;
#endif

		pst_ambn->dsk_flg = 0;
		pst_ambn->mem_flg = 0;

		pst_ambn->size = alloc_size;
		pst_ambn->pst_atn = pst_atn;

		if(pst_in_din)
		{
			SET_DISK_IDX_DATA<T_Key>(ptr_data, pst_in_din);
		}

		if(in_mem_len > 0 && ptr_in_mem)
		{
			SET_MEM_ADX_DATA<T_Key>(ptr_data, (var_1*)ptr_in_mem, in_mem_len);
		}

		return ptr_data;
	}

	const var_4 free_tree_node(AVL_TREE_NODE<T_Key> *pst_del_atn)
	{
		if(pst_del_atn->ptr_data)
		{
			data_wait_lock(&pst_del_atn->st_dcln);
			free_data(pst_del_atn->ptr_data);
			pst_del_atn->ptr_data = NULL;
		}

		pst_del_atn->st_stn.del_flg = 1;

		pst_del_atn->pst_left = NULL;
		pst_del_atn->pst_right = NULL;
		pst_del_atn->st_stn.pst_sub = NULL;
		//pst_del_atn->height = 0;
		//pst_del_atn->key = 0;

		m_free_tnode_list.Add(pst_del_atn);
		//m_aoc_tn_recycle.FreeMem((var_1*)pst_del_atn);
		return 0;
	}

	void tree_node_lock(const var_4 lock_suf, const var_4 is_write = 0)
	{
		HASH_TABLE_NODE<T_Key> *pst_htn = m_psta_hash_table + lock_suf;

		var_u4 suf = lock_suf % m_oper_mtx_max;

		m_pta_oper_mtx[suf].lock();
		//pst_cln->oper_count++;

#ifdef _TEST_LOCK_COUNT_
		if(is_write == 1)
		{
			//pst_tln->write++;
		}
		else
		{
			//pst_tln->read++;
		}	

#ifdef _PRINTF_LOG_FILE_
		//	m_log.Log(false, "%p:flg:%d,oc:%d,rc:%d[%d,%d]\n", pst_tln, is_write, pst_tln->oper_count, pst_tln->r_count,pst_tln->write, pst_tln->read);
#endif 
#endif
		m_pta_oper_mtx[suf].unlock();

		if(is_write == 1)
		{
			for(;;)
			{
				m_pta_oper_mtx[suf].lock();
				if(pst_htn->r_count <= 0 && pst_htn->flg == 0)
				{
					pst_htn->r_count = 0;
					pst_htn->flg = 1;
					m_pta_oper_mtx[suf].unlock();
					break;
				}
				m_pta_oper_mtx[suf].unlock();
				cp_sleep(1);
			}
		}
		else
		{
			for(;;)
			{
				m_pta_oper_mtx[suf].lock();
				if(pst_htn->flg == 0 && pst_htn->r_count < I_CHILE_TREE_MAX)
				{
					pst_htn->r_count++;
					m_pta_oper_mtx[suf].unlock();
					break;
				}
				m_pta_oper_mtx[suf].unlock();
				cp_sleep(1);
			}
		}
	}

	void tree_node_unlock(const var_4 lock_suf)
	{
		HASH_TABLE_NODE<T_Key> *pst_htn = m_psta_hash_table + lock_suf;

		var_u4 suf = lock_suf % m_oper_mtx_max;

		m_pta_oper_mtx[suf].lock();
		//pst_tln->oper_count--;

#ifdef _TEST_LOCK_COUNT_
		if(pst_htn->flg == 1)
		{
			//pst_tln->write--;
		}
		else
		{
			//pst_tln->read--;
		}	

#ifdef _PRINTF_LOG_FILE_
		//m_log.Log(false, "%p:flg:%d,oc:%d,rc:%d[%d,%d]-un\n", pst_tln, pst_tln->flg,pst_tln->oper_count,pst_tln->r_count, pst_tln->write,pst_tln->read);
#endif 
#endif
		if(pst_htn->flg != 0)
		{
			pst_htn->flg = 0;
		}
		else
		{
			if(pst_htn->r_count <= 0)
			{
				printf("lock error! count:%d\n", pst_htn->r_count);
				assert(0);
			}
			pst_htn->r_count--;
		}


		m_pta_oper_mtx[suf].unlock();
	}

	void data_wait_lock(DATA_COUNT_LOCK_NODE *pst_dcln)
	{
		var_u8 *pt_key = *(var_u8**)(&pst_dcln);
		var_u4 suf = (var_u4)((*pt_key) % m_oper_mtx_max);
		suf=0;

		for(;;)
		{
			m_pta_oper_mtx[suf].lock();
			if(pst_dcln->oper_count == 0)
			{
				m_pta_oper_mtx[suf].unlock();
				break;
			}

			if(pst_dcln->oper_count <= 0)
			{
				printf("oper_count error! count:%d\n", pst_dcln->oper_count);
				assert(0);
			}
			m_pta_oper_mtx[suf].unlock();
		}
	}

	void data_node_lock(DATA_COUNT_LOCK_NODE *pst_dcln, const var_4 is_write = 0)
	{
		var_u8 *pt_key = *(var_u8**)(&pst_dcln);
		var_u4 suf = (var_u4)((*pt_key) % m_oper_mtx_max);
		suf = 0;

		m_pta_oper_mtx[suf].lock();
		pst_dcln->oper_count++;

#ifdef _TEST_LOCK_COUNT_
		if(is_write == 1)
		{
			pst_dcln->write++;
		}
		else
		{
			pst_dcln->read++;
		}

#ifdef _PRINTF_LOG_FILE_
		//	m_log.Log(false, "%p:flg:%d,oc:%d,rc:%d[%d,%d]\n", pst_dcln, is_write, v->oper_count, pst_dcln->r_count,pst_dcln->write, pst_dcln->read);
#endif
#endif
		m_pta_oper_mtx[suf].unlock();

		if(is_write == 1)
		{
			for(;;)
			{
				m_pta_oper_mtx[suf].lock();
				if(pst_dcln->r_count <= 0 && pst_dcln->flg == 0)
				{
					pst_dcln->r_count = 0;
					pst_dcln->flg = 1;
					m_pta_oper_mtx[suf].unlock();
					break;
				}
				m_pta_oper_mtx[suf].unlock();
				cp_sleep(1);
			}
		}
		else
		{
			for(;;)
			{
				m_pta_oper_mtx[suf].lock();
				if(pst_dcln->flg == 0 && pst_dcln->r_count < I_DCLN_MAX)
				{
					pst_dcln->r_count++;
					m_pta_oper_mtx[suf].unlock();
					break;
				}
				m_pta_oper_mtx[suf].unlock();
				cp_sleep(1);
			}
		}
	}

	void data_node_unlock(DATA_COUNT_LOCK_NODE *pst_dcln)
	{
		var_u8 *pt_key = *(var_u8**)(&pst_dcln);
		var_u4 suf = (var_u4)((*pt_key) % m_oper_mtx_max);
		suf = 0;
		m_pta_oper_mtx[suf].lock();
		pst_dcln->oper_count--;

#ifdef _TEST_LOCK_COUNT_
		if(pst_dcln->flg == 1)
		{
			pst_dcln->write--;
		}
		else
		{
			pst_dcln->read--;
		}

#ifdef _PRINTF_LOG_FILE_
		//m_log.Log(false, "%p:flg:%d,oc:%d,rc:%d[%d,%d]-un\n", pst_dcln, pst_dcln->flg,pst_dcln->oper_count,pst_dcln->r_count, pst_dcln->write,pst_dcln->read);
#endif
#endif
		if(pst_dcln->flg != 0)
		{
			pst_dcln->flg = 0;
		}
		else
		{
			if(pst_dcln->r_count <= 0)
			{
				printf("lock error! count:%d\n", pst_dcln->r_count);
				assert(0);
			}
			pst_dcln->r_count--;
		}

		m_pta_oper_mtx[suf].unlock();
	}

private:  //tree node
	//////////////////////////////////////////////////////////////////////
	//���ܣ�	  �洢�ṹʹ�õ�AVL����������
	//��ע��
	//����:      qinfei
	//�������:   20121129
	//////////////////////////////////////////////////////////////////////
	const var_4 search_tree_node(AVL_TREE_NODE<T_Key> *spTree ,const T_Key tKey, AVL_TREE_NODE<T_Key> *&spOutTree)
	{
		spOutTree = NULL;
		while(spTree)
		{
			if(tKey == spTree->key)
			{
				spOutTree = spTree;
				return 0;
			}
			else if(tKey < spTree->key)
			{
				spTree = spTree->pst_left;
			}
			else
			{
				spTree = spTree->pst_right;
			}
		}

		return -1;
	}

	const var_4 insert_tree_node(AVL_TREE_NODE<T_Key> *pst_in_root, AVL_TREE_NODE<T_Key> *pst_new, AVL_TREE_NODE<T_Key> *&pst_out_root)
	{
		pst_out_root = NULL;

		if(pst_in_root == NULL)
		{
			pst_in_root = pst_new;

			pst_out_root = pst_in_root;
			return 0;
		}

		if(pst_new->key == pst_in_root->key)
		{
			return 1;
		}
		else if(pst_new->key < pst_in_root->key)
		{
			var_4 ret = insert_tree_node(pst_in_root->pst_left, pst_new, pst_in_root->pst_left);
			if( ret < 0)
			{
				return ret - 10;
			}
			else if(ret != 0)
			{
				return ret;
			}

			if(HEIGHT_TREE(pst_in_root->pst_left) - HEIGHT_TREE(pst_in_root->pst_right) == 2)
			{
				if(pst_new->key < pst_in_root->pst_left->key)
				{
					pst_in_root = SL_Rot(pst_in_root);
				}
				else
				{
					pst_in_root = DL_Rot(pst_in_root);
				}
			}
		}
		else
		{
			var_4 ret = insert_tree_node(pst_in_root->pst_right, pst_new, pst_in_root->pst_right);
			if( ret < 0)
				return ret - 20;
			else if(ret != 0)
			{
				return ret;
			}

			if(HEIGHT_TREE(pst_in_root->pst_right) - HEIGHT_TREE(pst_in_root->pst_left) == 2)
			{
				if(pst_new->key > pst_in_root->pst_right->key)
				{
					pst_in_root = SR_Rot(pst_in_root);
				}
				else
				{
					pst_in_root = DR_Rot(pst_in_root);
				}
			}
		}

		pst_in_root->height = MAX_VAL(HEIGHT_TREE(pst_in_root->pst_left), HEIGHT_TREE(pst_in_root->pst_right)) + 1;
		pst_out_root = pst_in_root;

		return 0;
	}

	const var_4 arrange_tree(const var_4 parent_level, AVL_TREE_NODE<T_Key> *pst_root, AVL_TREE_NODE<T_Key> *&pst_new_root)
	{

		if(pst_root->pst_left)
		{
			arrange_tree(parent_level, pst_root->pst_left, pst_new_root);
		}

		if(pst_root->pst_right)
		{
			arrange_tree(parent_level, pst_root->pst_right, pst_new_root);
		}

		if(pst_root->st_stn.del_flg == 1)
		{
			free_tree_node(pst_root);
			return 0;
		}

		pst_root->pst_left = NULL;
		pst_root->pst_right = NULL;
		pst_root->height = 0;
		//pst_root->st_tln.tree_num = 0;

		assert(parent_level + 1 == pst_root->st_stn.level);
		insert_tree_node(pst_new_root, pst_root, pst_new_root);

#ifdef DEBUG_PRINTF
		printf(CP_PU64"[%d, %p],", pst_root->key, pst_root->st_stn.del_flg, pst_root->st_stn.pst_sub);
#endif
		VERIFY_DATA_NODE(pst_root->st_stn.level, pst_root->key, pst_root, pst_root->ptr_data);

		if(pst_root->st_stn.pst_sub)
		{

			AVL_TREE_NODE<T_Key> *pst_tmp_root = NULL;
			arrange_tree(pst_root->st_stn.level, pst_root->st_stn.pst_sub, pst_tmp_root);

			pst_root->st_stn.tree_num = get_tree_num(pst_tmp_root);
			pst_root->st_stn.pst_sub = pst_tmp_root;
		}

#ifdef DEBUG_PRINTF
		if(pst_root->st_stn.level == 0)
			printf("\n");
#endif
		return 0;
	}


	const var_4 get_tree_num(AVL_TREE_NODE<T_Key> *pst_atn)
	{
		var_4 num = 0;

		if(pst_atn)
		{
			num++;

			if(pst_atn->pst_left)
				num += get_tree_num(pst_atn->pst_left);

			if(pst_atn->pst_right)
				num += get_tree_num(pst_atn->pst_right);
		}

		return num;
	}

	const var_4 get_tree_del_num(AVL_TREE_NODE<T_Key> *pst_atn)
	{
		var_4 num = 0;

		if(pst_atn)
		{
			if(pst_atn->st_stn.del_flg == 1)
				num++;

			if(pst_atn->pst_left)
				num += get_tree_del_num(pst_atn->pst_left);

			if(pst_atn->pst_right)
				num += get_tree_del_num(pst_atn->pst_right);

			if(pst_atn->st_stn.pst_sub)
				num += get_tree_del_num(pst_atn->st_stn.pst_sub);
		}

		return num;
	}

private:  
	AVL_TREE_NODE<T_Key> * SL_Rot(AVL_TREE_NODE<T_Key> *pst_atn)
	{
		AVL_TREE_NODE<T_Key> *pst_tmp = pst_atn->pst_left;
		pst_atn->pst_left = pst_tmp->pst_right;
		pst_tmp->pst_right = pst_atn;

		pst_atn->height = MAX_VAL(HEIGHT_TREE(pst_atn->pst_left), HEIGHT_TREE(pst_atn->pst_right)) + 1;
		pst_tmp->height = MAX_VAL(HEIGHT_TREE(pst_tmp->pst_left), HEIGHT_TREE(pst_tmp->pst_right)) + 1;

		return pst_tmp;
	}

	AVL_TREE_NODE<T_Key> * SR_Rot(AVL_TREE_NODE<T_Key> *pst_atn)
	{
		AVL_TREE_NODE<T_Key> *pst_tmp = pst_atn->pst_right;
		pst_atn->pst_right = pst_tmp->pst_left;
		pst_tmp->pst_left = pst_atn;

		pst_atn->height = MAX_VAL(HEIGHT_TREE(pst_atn->pst_left), HEIGHT_TREE(pst_atn->pst_right)) + 1;
		pst_tmp->height = MAX_VAL(HEIGHT_TREE(pst_tmp->pst_left), HEIGHT_TREE(pst_tmp->pst_right)) + 1;

		return pst_tmp;
	}

	AVL_TREE_NODE<T_Key> * DL_Rot(AVL_TREE_NODE<T_Key> *pst_atn)
	{
		pst_atn->pst_left = SR_Rot(pst_atn->pst_left);
		return SL_Rot(pst_atn);
	}

	AVL_TREE_NODE<T_Key> * DR_Rot(AVL_TREE_NODE<T_Key> *pst_atn)
	{
		pst_atn->pst_right = SL_Rot(pst_atn->pst_right);
		return SR_Rot(pst_atn);
	}

	AVL_TREE_NODE<T_Key>* alloc_tree_node(const var_u1 level, const T_Key key, char *ptr_data, AVL_TREE_NODE<T_Key> *pst_sub, const var_4 lock_suf)
	{
		AVL_TREE_NODE<T_Key>* pst_atn = (AVL_TREE_NODE<T_Key>*)m_aoc_tn_recycle.AllocMem();

		if(pst_atn == NULL)
			return NULL;


		pst_atn->pst_left = NULL;
		pst_atn->pst_right = NULL;
		pst_atn->height = 0;

		pst_atn->key = key;
		pst_atn->st_stn.level = level;
		pst_atn->ptr_data = ptr_data;
		pst_atn->st_stn.pst_sub = pst_sub;
		pst_atn->lock_suf = lock_suf;

		pst_atn->st_dcln.r_count = 0;
		pst_atn->st_dcln.flg = 0;
		pst_atn->st_dcln.oper_count = 0;

		//pst_atn->st_tln.r_count = 0;
		//pst_atn->st_tln.flg = 0;
		pst_atn->st_stn.del_flg = 0;
		//			spTree->st_tln.tree_cln.oper_count = 0;

#ifdef _TEST_LOCK_COUNT_
		//pst_atn->st_tln.tree_cln.write = 0;
		//pst_atn->st_tln.tree_cln.read = 0;

		pst_atn->st_dcln.write = 0;
		pst_atn->st_dcln.read = 0;
#endif

		pst_atn->st_stn.tree_num = 0;

		if(pst_atn->ptr_data)
		{
			AOC_MEM_BLOCK_NODE<T_Key> *ptr_ambn = NULL;
			GET_AMBN_HEAD(pst_atn->ptr_data, ptr_ambn);
			ptr_ambn->pst_atn = pst_atn;
		}

		return pst_atn;
	}

private:
	static var_4 fun_judge(var_4 file_no, var_8 file_offset, var_4& one_buf_len, var_1*& one_buf, void *vp_info)
	{
		UC_StoreManager *p_this = (UC_StoreManager*)vp_info;

		var_4 num = one_buf_len / sizeof(T_Key), ret = 0;
		T_Key *pta_key = (T_Key*)one_buf;

		var_1 *ptr_idx = NULL;


		ret = p_this->query_single_lock(num, pta_key, ptr_idx);
		if(ret != 0)
			return ret;

		DISK_INDEX_NODE *ptr_din = NULL;

		AVL_TREE_NODE<T_Key> *pst_atn = (AVL_TREE_NODE<T_Key> *)ptr_idx;

		ret = -10;	
		GET_DISK_IDX_HEAD<T_Key>(pst_atn->ptr_data, ptr_din);
		if(ptr_din)
		{
			if(ptr_din->off == file_offset && ptr_din->file_no == file_no)
			{
				ret = 0;
			}
			else
			{
				ret = -11;
			}
		}	
		p_this->query_single_unlock(ptr_idx);

		return ret;
	}

	static var_4 fun_update(var_4 file_no, var_8 file_offset, var_4& one_buf_len, var_1*& one_buf, void *vp_info)
	{
		UC_StoreManager *p_this = (UC_StoreManager*)vp_info;

		var_4 num = one_buf_len / sizeof(T_Key), ret = 0;
		T_Key *pta_key = (T_Key*)one_buf;

		var_1 *pta_ret_idx[1];

		DISK_INDEX_NODE *psa_din[1], st_din;
		st_din.file_no = file_no;
		st_din.off = file_offset;
		psa_din[0] = &st_din;

		ret = p_this->query_single_lock(num, pta_key, pta_ret_idx[0], 1);
		if(ret != 0)
			return ret;

		ret = p_this->modify_dsk_data(1, (const var_1 **)pta_ret_idx, (const DISK_INDEX_NODE**)psa_din);	

		p_this->query_single_unlock(pta_ret_idx[0], 1);

		return ret;
	}

	static CP_THREAD_T arrange_thread(void *vp_info)
	{
		UC_StoreManager *p_this = (UC_StoreManager*)vp_info;

		var_4 ret = 0;

		var_u4 am_count = 0, save_count = 0, cur_day = 0, recy_count = 0;

		const var_4 I_MAX_TIME = 1800;

		time_t cur_time = time(NULL);
		struct tm *ptr_tm_cur = localtime(&cur_time); 
		cur_day = ptr_tm_cur->tm_mday;

		p_this->m_thread_flg = 1;

		AVL_TREE_NODE<T_Key> *pst_tmp = NULL;

		while(p_this->m_exit_flg)
		{
			am_count ++;			
			if(am_count > I_MAX_TIME)
			{
				ret = p_this->arrange_memory(MIN_FREE_BLOCK);
				printf("arrange_memory ret:%d\n", ret);
				if(ret == 0)
				{
					am_count = 0;
				}
			}

			recy_count++;
			if(recy_count >I_MAX_TIME)
			{
				recy_count = 0;
				while(p_this->m_free_tnode_list.Get(pst_tmp) == 0)
				{
					p_this->m_aoc_tn_recycle.FreeMem((var_1*)pst_tmp);
				}
			}

			save_count++;
			if(save_count > 1000)
			{
				cur_time = time(NULL);
				localtime(&cur_time);

				//if(cur_day != ptr_tm_cur->tm_mday && ptr_tm_cur->tm_hour > 1 && ptr_tm_cur->tm_hour < 6)
				{					
					ret = p_this->save();
					if(ret == 0)
					{
						cur_day =  ptr_tm_cur->tm_mday;		
						save_count = 0;
					}

					printf("%02d:%02d:%02d save index! ret = %d\n", ptr_tm_cur->tm_hour, ptr_tm_cur->tm_min, ptr_tm_cur->tm_sec, ret);
				}				
			}

			cp_sleep(1000);
		}

		p_this->m_thread_flg = -1;

		return 0;
	}

private:
	const var_2                            I_AMBN_SIZE;
	const var_2                            I_IFN_SIZE;
	const var_2                            I_ATN_SIZE;
	const var_2                            I_HTLN_SIZE;
	const var_2                            I_TKEY_SIZE;

	const var_4                            I_MEMORY_APPENDIX_SIZE;

	const var_u8                           I_STORE_FILE_MARK;     //����������ݵı�ʶ

	CStoreList<var_1*>                     m_use_mem_list;

	CStoreList<AVL_TREE_NODE<T_Key>* >     m_free_tnode_list;

	CP_MUTEXLOCK                           m_inc_file_lock;      //�����ļ���

	CP_MUTEXLOCK                           m_aoc_mem_lock;       //�ڴ������

	UT_Queue<TRAVEL_HEAD_INFO<T_Key>* >    m_travel_que;

	UC_Allocator_Recycle                   m_aoc_tn_recycle;     //�ڴ�������

	var_u8                                 m_aoc_mem_off;      //��ǰ������ڴ�ƫ��
	char                                  *m_ptr_aoc_mem;      //��ǰ������ڴ���ʼ��ַ

	var_u8                                 m_one_aoc_mem_size;  //���η�����ڴ����󳤶�
	var_4                                  m_one_inc_data_size;   //�����������ݵ�ռ�����ռ�

	var_4                                  m_max_level;     //������

	CP_MUTEXLOCK                          *m_pta_oper_mtx;
	var_4                                  m_oper_mtx_max;

	HASH_TABLE_NODE<T_Key>               *m_psta_hash_table;    //HASH �����
	var_4                                 m_hash_tln_size;   //HASH����

	char                                   m_str_sto_lib_idx[256];

	FILE                                  *m_fp_inc;         //�����������

	var_u4                                 m_arr_tree_key_max[I_SYS_MAX_LEVEL];

	UC_DiskQuotaManager                    m_disk_quota_man;
	bool                                   m_exit_flg;
	var_1                                  m_thread_flg;

#ifdef _PRINTF_LOG_FILE_
	CStoreLog                              m_log;
#endif

};

//////////////////////////////////////////////////////////////////////
//�ࣺ      CStoreStack
//���ܣ�	   �洢�ṹʹ�õ�STACK������
//��ע��
//����:      qinfei
//�������:   20121129
//////////////////////////////////////////////////////////////////////
template<class TYPE>
class CStoreStack
{
public:
	CStoreStack()
	{
		m_cur_pos = 0;
	}

	const var_4 Push(const TYPE t_data)
	{
		if(m_cur_pos >= I_STACK_SIZE)
		{
			STORE_PRINTF_LOG("stack full!\n");

			for(;;)
			{
				printf("push stack failed!,stack_size:%d\n", I_STACK_SIZE);
				cp_sleep(10000);
			}

			return -1;
		}

		m_arr_data[m_cur_pos] = t_data;
		m_cur_pos++;
		return 0;
	}

	const TYPE Pop()
	{
		TYPE t_data;

		if(m_cur_pos>0)
		{
			m_cur_pos--;
			t_data = m_arr_data[m_cur_pos];
		}

		return t_data;
	}

	const bool IsEmpty()
	{
		return m_cur_pos <= 0;
	}
	const bool IsFull()
	{
		return m_cur_pos >= I_STACK_SIZE;
	}
private:
	var_u4                m_cur_pos;
	TYPE                  m_arr_data[I_STACK_SIZE];
};


#endif // _UC_STORE_MANAGER_H_

