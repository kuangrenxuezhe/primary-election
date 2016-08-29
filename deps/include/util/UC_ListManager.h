//
//  UC_ListManager.h
//  code_library
//
//  Created by zhanghl on 12-11-1.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_LIST_MANAGER_H_
#define _UC_LIST_MANAGER_H_
#include "UH_Define.h"
#include "UT_HashSearch.h"
#include "UC_Communication.h"
#include "UC_MD5.h"

#define  MAX_URL_NUM 100000
#define  MAX_URL_LEN 256
#define  MAX_TRAVEL_NUM 1000
#define	 MAX_LISTEN_NUM 10
#define	 SERVER_TIME_OUT 120
#define	 CLIENT_TIME_OUT 120
//#define _TEST_START_SERVEICE
#define _TEST_NOTIFY_SERVER

typedef struct _UT_LISTMGR_HANDEL_
{
	CP_MutexLock_RW_LCK query_rw_lck;
	CP_MutexLock_RW_FUN query_rw_func;
	
}UT_LISTMGR_HANDEL;

typedef struct _UT_LISTMGR_TRAVEL_HANDEL_
{
	UT_LISTMGR_HANDEL *query_handle;

	var_vd *hash_trave_handel;
	
	UT_HashSearch<var_u8> *trave_list;

}UT_LISTMGR_TRAVEL_HANDEL;

//一个当前列表 及一个差集列表，句柄0 代表当前列表，1代表差集列表
class UC_ListManager
{
public:
	var_4 init_list()
	{
		m_pCurLst = new UT_HashSearch<var_u8>();
		if(m_pCurLst->InitHashSearch(MAX_URL_NUM,MAX_URL_LEN))
			return -1;
		m_pDifSet = new UT_HashSearch<var_u8>();
		if(m_pDifSet->InitHashSearch(MAX_URL_NUM,MAX_URL_LEN))
			return -1;
		m_pNewLst = new UT_HashSearch<var_u8>();
		if(m_pNewLst->InitHashSearch(MAX_URL_NUM,MAX_URL_LEN))
			return -1;
		return 0;
		
	}
	//////////////////////////////////////////////////////////////////////
	// 函数:      query_set
	// 功能:      查询指定key是否在名单中存在
	// 入参:
	//                    key_md5: 要查询的key
	//                 set_handle: 列表句柄,如果为NULL,则查询当前列表,否则查询句柄指定列表
	// 
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 query_set(var_u8 key_md5, var_vd* set_handle = NULL)
	{
		UT_HashSearch<var_u8> *cur_lst;
		UT_LISTMGR_HANDEL *cur_handle = NULL;
		var_4 list_id = (set_handle == NULL)?0:*(var_4*)set_handle;
		
		alloc_list_handle(list_id,cur_lst,cur_handle);

		cur_handle->query_rw_func.lock_r(&cur_handle->query_rw_lck);
		if(set_handle)
		{
			cur_lst = (set_handle == NULL)?m_pCurLst:m_pDifSet;//暂时支持两个列表选择
			if(cur_lst->SearchKey_FL(key_md5))
			{
				cur_handle->query_rw_func.unlock(&cur_handle->query_rw_lck);
				return -1;
			}
			cur_handle->query_rw_func.unlock(&cur_handle->query_rw_lck);
		}
		else
		{
			if(m_pCurLst->SearchKey_FL(key_md5))
			{
				m_pCurSetHandle.query_rw_func.unlock(&m_pCurSetHandle.query_rw_lck);
				return -1;
			}
			m_pCurSetHandle.query_rw_func.unlock(&m_pCurSetHandle.query_rw_lck);
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      query_set
	// 功能:      查询指定key是否在名单中存在
	// 入参:
	//                    key_buf: 要查询的key
	//					  key_buf_len: 要查询的key 长度
	//                 set_handle: 列表句柄,如果为NULL,则查询当前列表,否则查询句柄指定列表
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 query_set(var_1* key_buf, var_4 key_buf_len,var_vd* set_handle = NULL)
	{
		
		var_u8 key_md5 = m_md5Con.MD5Bits64((unsigned char *)key_buf,key_buf_len);

		return query_set(key_md5,set_handle);
	}

	//得到key_md5下所附带的数据内容
	var_4 get_keybuf(var_u8 key_md5, var_1 *key_buf = NULL)
	{
		var_vd *vpBuf =(var_vd *) key_buf;

		m_pCurSetHandle.query_rw_func.lock_r(&m_pCurSetHandle.query_rw_lck);
		if(m_pCurLst->SearchKey_FL(key_md5,&vpBuf))
		{
			m_pCurSetHandle.query_rw_func.unlock(&m_pCurSetHandle.query_rw_lck);
			return -1;
		}
		key_buf = (var_1 *) vpBuf;
		m_pCurSetHandle.query_rw_func.unlock(&m_pCurSetHandle.query_rw_lck);
		return 0;
	}
	//////////////////////////////////////////////////////////////////////
	// 函数:      travel_prepare
	// 功能:      取得遍历句柄
	// 入参:
	//              travel_handle: 返回的遍历句柄
	//                 set_handle: 列表句柄,如果为NULL,则遍历当前列表,否则遍历句柄指定列表
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 travel_prepare(var_vd*& travel_handle, var_vd* set_handle = NULL)
	{
		var_4 cur_list_id = 0;
		if(set_handle != NULL)
		 cur_list_id = *(var_4 *)set_handle;
		cur_list_id = (set_handle == NULL)?0:*(var_4 *)set_handle;

		UT_LISTMGR_TRAVEL_HANDEL *cur_handle = new UT_LISTMGR_TRAVEL_HANDEL;
		alloc_list_handle(cur_list_id,cur_handle->trave_list,cur_handle->query_handle);
		
		cur_handle->query_handle->query_rw_func.lock_r(&cur_handle->query_handle->query_rw_lck);

		cur_handle->trave_list->travel_prepare(cur_handle->hash_trave_handel);
		travel_handle = (var_vd*)cur_handle;
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      travel_key
	// 功能:      遍历列表
	// 入参:
	//              travel_handle: 遍历句柄
	//                    key_md5: 当前结点的key值
	//                    key_str: 当前结点的key串
	//
	// 出参:
	// 返回值:    如果返回0则当前结点遍历成功，返回非0值遍历结束
	// 备注:      循环调用此函数
	//////////////////////////////////////////////////////////////////////
	var_4 travel_key(var_vd* travel_handle, var_u8& key_md5, var_1**key_str = NULL )
	{
		var_4 key_str_len; // 
		var_vd *str_addr ;//= (var_vd *)key_str;
		
		UT_LISTMGR_TRAVEL_HANDEL *cur_trave_handle = (UT_LISTMGR_TRAVEL_HANDEL *)travel_handle;

		UT_HashSearch<var_u8> *cur_set = (UT_HashSearch<var_u8> *)cur_trave_handle->trave_list;

		var_4 ret = cur_set->travel_key(cur_trave_handle->hash_trave_handel,key_md5,str_addr,key_str_len);

		*key_str = (var_1*)str_addr;

		return ret;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      travel_finsih
	// 功能:      释放遍历句柄
	// 入参:
	//              travel_handle: 遍历句柄
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 travel_finsih(var_vd* travel_handle)
	{
		UT_LISTMGR_TRAVEL_HANDEL *cur_trave_handle = (UT_LISTMGR_TRAVEL_HANDEL *)travel_handle;
		cur_trave_handle->trave_list->travel_finish(cur_trave_handle->hash_trave_handel);

		cur_trave_handle->query_handle->query_rw_func.unlock(&cur_trave_handle->query_handle->query_rw_lck);

		delete cur_trave_handle;
		return 0;
	}
	//自文件中得到当前列表
	var_4 load_new_lst(const var_1 *file_path,var_4 rep_flg = -1)
	{
		var_1 url[256];
		var_4 url_len = -1;
		UC_MD5 con_md5;
		var_u8 key_md5 = 0;
		var_8 file_size = 0;
		FILE *fp = NULL;
		m_pNewSetHandel.query_rw_func.lock_w(&m_pNewSetHandel.query_rw_lck);
		if(rep_flg ==0 )
		{
			if(m_pNewLst->ClearHashSearch())
			{
				m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
				return -4;
			}
		}
		file_size = cp_get_file_size((var_1 *)file_path);

		fp = fopen(file_path,"rb");
		if(!fp)
		{
			m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
			return -1;
		}
		fseek(fp,8,SEEK_SET);//跳过版本号

		file_size -= 8;

		while (file_size > 0)
		{
			if(fread(&key_md5,8,1,fp) != 1)
			{
				fclose(fp);
				m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
				return -2;
			}
			if(fread(&url_len,4,1,fp) != 1)
			{
				fclose(fp);
				m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
				return -2;
			}
			file_size -= 12;
			if(url_len != -1 && url_len != 0)
			{
				if(fread(url,url_len,1,fp) != 1)
				{
					fclose(fp);
					m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
					return -2;
				}
				file_size -= url_len;
			}
			if(url_len > 0)
			{
				if(m_pNewLst->AddKey_FL(key_md5,url))
				{
					fclose(fp);
					m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
					return -3;
				}
			}
			else
			{
				if(m_pCurLst->AddKey_FL(key_md5))
				{
					fclose(fp);
					m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
					return -3;
				}
			}
			
		}
		fclose(fp);
		m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
		return 0;
	}

	var_4 swap_cur_list()
	{
		UT_HashSearch<var_u8> *temp_set = NULL;;
		m_pCurSetHandle.query_rw_func.lock_w(&m_pCurSetHandle.query_rw_lck);
		m_pNewSetHandel.query_rw_func.lock_w(&m_pNewSetHandel.query_rw_lck);
		if(m_pNewLst->GetKeyNum() == 0)
		{
			printf("uc listmanger; replace post list file verison failed. New list does not exsit.\n");
			m_pCurSetHandle.query_rw_func.unlock(&m_pCurSetHandle.query_rw_lck);
			return -1;
		}
		m_pCurLst->ClearHashSearch();
		//交换指针
		temp_set = m_pCurLst;
		m_pCurLst = m_pNewLst;
		m_pNewLst = temp_set;
		m_pCurSetHandle.query_rw_func.unlock(&m_pCurSetHandle.query_rw_lck);
		m_pNewSetHandel.query_rw_func.unlock(&m_pNewSetHandel.query_rw_lck);
		return 0;
	}
	
	var_4 gen_dif_set()
	{
		if(m_pNewLst->GetKeyNum() == 0)
		{
			printf("uc list manager. new list version does not exsit. \n");
			return -1;
		}
		var_vd *tra_handle = NULL;
		var_4 cur_set = 2;
		var_4 travel_ret = 0;
		var_u8 key_md5;
		var_vd *vpBuf;
		m_pDifSetHandel.query_rw_func.lock_w(&m_pDifSetHandel.query_rw_lck);
		if (travel_prepare(tra_handle,&cur_set))
		{
			printf("uc list manager. set travel handle failed.\n");
			travel_finsih(tra_handle);
			return -2;
		}
		while(!travel_ret)
		{
			travel_ret = travel_key(tra_handle,key_md5);
			if (m_pCurLst->SearchKey_FL(key_md5,&vpBuf))
			{
				if(m_pDifSet->AddKey_FL(key_md5,vpBuf))
				{
					printf("uc list manager. add new record to diffrent set failed.\n");
					travel_finsih(tra_handle);
					return -3;
				}
			}
		}
		travel_finsih(tra_handle);
		m_pDifSetHandel.query_rw_func.unlock(&m_pDifSetHandel.query_rw_lck);
		return 0;
	}

	var_4 add_key(var_u8 key_md5, var_1 *url = NULL, var_4 url_len = -1,var_vd *set_handle = NULL, var_4 need_loc = 0)
	{
		var_4 cur_list;// = *(var_4 *)set_handle;
		var_4 ret = -1;
		UT_HashSearch<var_u8> *add_set = NULL;
		UT_LISTMGR_HANDEL *cur_handel = NULL;
		if(set_handle != NULL)
		{
			cur_list = *(var_4 *)set_handle;
			if(alloc_list_handle(cur_list , add_set, cur_handel))
			{
				return -1;
			}
		}
		else
		{
			cur_handel = &m_pCurSetHandle;
			add_set = m_pCurLst;
		}
		if(need_loc == 0)
			cur_handel->query_rw_func.lock_w(&cur_handel->query_rw_lck);
		ret = add_set->AddKey_FL(key_md5,(var_vd *)url);

		if(need_loc == 0)
			cur_handel->query_rw_func.unlock(&cur_handel->query_rw_lck);
		return ret;
	}
	var_4 add_key(var_1 *url = NULL, var_4 url_len = -1,var_vd *set_handle = NULL)
	{
		var_u8 key_md5 = m_md5Con.MD5Bits64((var_u1 *)url, url_len);

		return add_key(key_md5,url,url_len,set_handle);
	}
	var_4 del_key(var_u8 key_md5,var_vd *set_handle = NULL, var_4 need_loc = 0)
	{
		var_4 cur_list ;
		var_4 ret = -1;
		UT_HashSearch<var_u8> *cur_set = NULL;
		UT_LISTMGR_HANDEL *cur_handel = NULL;
		if(set_handle != NULL)
		{
			cur_list = *(var_4*)set_handle;
			if(alloc_list_handle(cur_list , cur_set, cur_handel))
			{
				return -1;
			}
		}
		else
		{
			cur_handel = &m_pCurSetHandle;
			cur_set = m_pCurLst;
		}
		if(need_loc == 0)
			cur_handel->query_rw_func.lock_w(&cur_handel->query_rw_lck);
		
		ret = cur_set->DeleteKey_FL(key_md5);
		
		if(need_loc == 0)
			cur_handel->query_rw_func.unlock(&cur_handel->query_rw_lck);

		return ret;
	}
	var_4 del_key(var_1 *key_buf, var_4 key_buf_len, var_vd *set_handle = NULL)
	{
		var_u8 key_md5 = m_md5Con.MD5Bits64((var_u1 *)key_buf, key_buf_len);

		return del_key(key_md5,set_handle);
	}
	var_4 clear_all(var_vd *set_handle = NULL, var_4 need_loc = -1)
	{
		var_4 cur_list = *(var_4*)set_handle;
		var_4 ret = -1;
		UT_HashSearch<var_u8> *cur_set = NULL;
		UT_LISTMGR_HANDEL *cur_handel = NULL;

		if(alloc_list_handle(cur_list , cur_set, cur_handel))
		{
			return -1;
		}
		if(need_loc == 0)
			cur_handel->query_rw_func.lock_w(&cur_handel->query_rw_lck);

		cur_set->ClearHashSearch();

		if(need_loc == 0)
			cur_handel->query_rw_func.unlock(&cur_handel->query_rw_lck);

		return 0;
	}

private:
	UT_HashSearch<var_u8>		*m_pCurLst;
	UT_HashSearch<var_u8>		*m_pDifSet;
	UT_HashSearch<var_u8>		*m_pNewLst;
	UT_LISTMGR_HANDEL			 m_pCurSetHandle;// 当前列表读写锁
	UT_LISTMGR_HANDEL			 m_pDifSetHandel;  //差集列表 读写锁
	UT_LISTMGR_HANDEL			 m_pNewSetHandel; //新接收列表读写锁
	UC_MD5						 m_md5Con;

	var_4 alloc_list_handle(var_4 list_id ,UT_HashSearch<var_u8> *&cur_set, UT_LISTMGR_HANDEL *&cur_handel)
	{
		switch(list_id)
		{
		case 0:
			cur_set = m_pCurLst;
			cur_handel = &m_pCurSetHandle;
			break;
		case 1:
			cur_set = m_pDifSet;
			cur_handel = &m_pCurSetHandle;
			break;
		case 2:
			cur_set = m_pNewLst;
			cur_handel = &m_pCurSetHandle;
			break;
		default:
			break;
		}
		return 0;
	}
};


class UC_ListManager_Client : public UC_ListManager,public UC_Communication_Server
{
public:
	//////////////////////////////////////////////////////////////////////
	// 函数:      init
	// 功能:      初始化列表管理器客户端
	// 入参:
	//                  save_dir: 列表保存路径目录，以/结尾
	//                  server_ip: 服务端ip
	//                server_port: 服务端port
	//                  push_flag: 是否接收服务器的更新通知
	//                  push_port: 接收服务器更新通知的port
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 init(var_1* save_dir, var_1* server_ip, var_u2 server_port, var_4 push_flag = 0, var_u2 push_port = 0)
	{
		//m_recv_notify_ser = new UC_ListComMgrServer();
		if(cp_create_dir(save_dir))
			return -1;
		var_1 cur_path[256];
		sprintf(cur_path,"%scurlst.dat",save_dir);

		//sprintf(m_lst_savedir,"%s_curlist.dat",save_dir);
		strcpy(m_lst_savedir,save_dir);

		m_com_cli = new UC_Communication_Client();

		if(m_com_cli->init(server_ip,server_port,120000))//init_client(0,10,&server_ip,&server_port))
		{
			return -1;
		}
		if(push_flag)
		{
			//m_recv_notify_ser = new UC_ListComMgrServer();
			if(UC_Communication_Server::init(push_port,1,120000,1,22))
				return -1;
		}
		if(init_list())
		{
			printf("uc list manager client. init list failed.\n");
			return -1;
		}

		if(load_new_lst(cur_path) == 0)
		{
			if(swap_cur_list())
				return -1;
		}

		m_ser_upt_flg = -1;

#ifdef _TEST_START_SERVEICE
		return 0;
#endif
		if(start())
		{
			printf("start service failed.\n");
			return -1;
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      get_update_lst
	// 功能:      从服务器更新本地列表
	// 入参:
	// 出参:
	// 返回值:    列表有更新返回0,如果没有更新返回>0,失败返回<0
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 get_update_lst()//不允许有其它线程同时请求
	{
		var_u8 cur_version;
		var_u8 recv_version;
		var_1 save_path[256];
		var_1 bak_path[256];
		var_4 bak_counter= 0;
		m_lst_fileloc.lock();
		if(get_fileversion(cur_version))
		{
			cur_version = 0;
		}
		sprintf(save_path,"%snewlst.dat",m_lst_savedir);
		sprintf(bak_path,"%s%d.bak",save_path,bak_counter);
		if(req_ser_lstversion(recv_version))
		{
			m_lst_fileloc.unlock();
			return -1;
		}
		if(cur_version != recv_version)
		{
			if(access(save_path,0) == 0)
			{
				//sprintf(bak_path,"%s%d.bak",save_path);
				//检测所有未使用的历史文件
				while(access(bak_path,0) ==0)
				{
					sprintf(bak_path,"%s%d.bak",save_path,bak_counter);
					bak_counter++;
				}
				if(rename(save_path,bak_path))
				{
					printf("uc list manager client. backup history file failed.\n");
					m_lst_fileloc.unlock();
					return -5;
				}
			}
			if(req_lst_file(save_path))
			{
				m_lst_fileloc.unlock();
				return -2;
			}
			if(load_new_lst(save_path))
			{
				m_lst_fileloc.unlock();
				if(remove(save_path))
				{
					printf("uc list manager client. load new list from file failed && remove new file failed.\n");
				}
				printf("uc list manager. load new list from file failed \n");
				m_lst_fileloc.unlock();
				return -3;
			}
		}
		else
		{
			m_lst_fileloc.unlock();
			return 1;//无最新版本
		}
		m_lst_fileloc.unlock();
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      set_update_fun
	// 功能:      设置更新通知回调函数指针
	// 入参:
	//                        fun: 回调函数指针
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 set_update_fun(var_4 (*fun)(void*, void*), void* argv)
	{
		m_upt_fuc = fun;
		m_uptfuc_agrv = argv;
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      set_reserver_flg
	// 功能:      设置当前列表保存标记
	// 入参:
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 set_reserve_flg()
	{
		var_1 cur_path[256],new_path[256];
		var_4 find_ret = -1, load_ret = -1;
		//m_set_uptflg_loc.lock();
		var_1 bak_path[256];
		//find_ret = find_latest_version(cur_path) ;
		sprintf(cur_path,"%scurlst.dat",m_lst_savedir);
		sprintf(new_path,"%snewlst.dat",m_lst_savedir);
		sprintf(bak_path,"%scurlst.dat.bak",m_lst_savedir);
		var_4 bak_counter = 0;
		//先换位文件，再换位列表
		if(access(cur_path,0) == 0)
		{
			if(access(bak_path,0) == 0)
				remove(bak_path);
			if(rename(cur_path,bak_path))
			{
				printf("uc list manager client. remove cur file: %s after swapping list due to : %s\n", strerror(errno));
				return -1;
			}
		}
		if(access(new_path,0) == 0)
		{
			if(rename(new_path,cur_path))
			{
				printf("uc list manager client. rename new file 2 cur file failed due to : %s\n",strerror(errno));
				return -2;
			}
		}
		else
			return 1;
		if(swap_cur_list() == 0)//交换新列表成功,删除所有历史文件
		{
			sprintf(bak_path,"%snewlst.dat%d.bak",m_lst_savedir,bak_counter);
			while(access(bak_path,0) ==0)
			{
				if(remove(bak_path))
				{
					printf("uc list manager client. remove history list file failed.\n");
				}
				sprintf(bak_path,"%snewlst.dat%d.bak",m_lst_savedir,bak_counter);
				
				bak_counter++;
			}
			
			return load_ret;
		}
		else//交换不成功，重命名回文件并不做改动
		{
			if(rename(cur_path,new_path))
			{
				printf("uc list manager client. Swap list failed: rename file %s failed due to : %s\n",cur_path, strerror(errno));
				return -3;
			}
			sprintf(bak_path,"%scurlst.dat.bak",m_lst_savedir);
			if(rename(bak_path,cur_path))
			{
				printf("uc list manager client. Swap list failed: rename file %s failed due to : %s\n",bak_path,strerror(errno));
				return -4;
			}
		}
		//m_set_uptflg_loc.unlock();
		return 0;
	}


	/////////////////////////////////////////////////////////////////
	// 函数:      make_difference_set
	// 功能:      生成当前列表于历史列表的差集
	// 入参:
	// 出参:
	// 返回值:    成功返回0,错误返回<0,没有历史文件返回>0且差集就是当时的全集
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 make_difference_set()
	{
		if(gen_dif_set())
		{
			printf("uc list manager. genration of diffrent set failed.\n");
			return -1;
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      pop_difference_set
	// 功能:      取得差集列表句柄
	// 入参:
	//                 set_handle: 差集句柄
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 pop_difference_set(var_vd*& set_handle)
	{
		var_4 dif_set = 1;

 		if(travel_prepare(set_handle,&dif_set))
		{
			printf("list manager. prepare dif set travel handle failed.\n");
			
			return -1;
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      push_difference_set
	// 功能:      释放差集列表句柄
	// 入参:
	//                 set_handle: 差集句柄
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 push_difference_set(var_vd*& set_handle)
	{
		if(travel_finsih(set_handle))
		{
			printf("list manager. termination of diff set travel failed.\n ");
			return -1;
		}
		return 0;
	}

public:
	var_4 start_notify_ser()
	{
		return start();
	}
	var_4 process_getuptlst()
	{
		bool suc_flg = true;
		time_t past_time = time(NULL);
		time_t cur_time = past_time;
		var_4 ret; 
		m_getupt_inv = 120000;
		while(suc_flg)
		{
			cp_sleep(10);
			cur_time = time(NULL);
			if(cur_time - past_time > m_getupt_inv)
			{
				ret = get_update_lst();
				if (ret < 0)
				{
					printf("list manager. activated req list thr. get update list from server faild.errno : %d\n",ret);
				}
			}
		}
		return 0;
	}

	var_4 process_upt_notify()
	{
		bool finsih_flag = true;
		while(finsih_flag)
		{
			m_set_uptflg_loc.lock();

			if(m_ser_upt_flg == 0)
			{
				m_ser_upt_flg = 1;
#ifdef _TEST_NOTIFY_SERVER
				printf("start to request file from server after recv upt notification. \n");
#endif
				if(get_update_lst() < 0)
				{
					printf("list manager client. get upt list failed after notification of upt.\n");
				}
				else if(m_upt_fuc(this,m_uptfuc_agrv))//调用回调处理
				{
					printf("list manager. call back function process failed.\n");
				}
			}
			m_set_uptflg_loc.unlock();
			cp_sleep(500);
		}
		return 0;
	}
	
private:
	var_4 get_fileversion(var_u8 &cur_verson)
	{
		var_1 save_path[256];
		sprintf(save_path,"%scurlst.dat",m_lst_savedir);
		FILE *fp = fopen(save_path,"rb");
		if(!fp)
		{
			return -1;
		}
		if(fread(&cur_verson,8,1,fp)!=1)
		{
			fclose(fp);
			return -1;
		}
		fclose(fp);
		return 0;
	}
	
	var_4 start()
	{
		if(cp_create_thread(thr_active_req,this))
		{
			printf("uc list manager client. start manual req list service failed.\n");
			return -1;
		}
#ifndef _TEST_NOTIFY_SERVER
		if(cp_create_thread(thr_req_newlst_file,this))
		{
			printf("uc list manager client. start upt notify service failed.\n");
			return -1;
		}
#endif
		
		if(UC_Communication_Server::start())
		{
			printf("uc list manager client. start upt notify recv server failed.\n");
			return -1;
		}
		return 0;
	}

	typedef var_4	 (*upt_fuc)(void *,void*);

	upt_fuc					m_upt_fuc;
	var_vd *				m_uptfuc_agrv;
	
	CP_MutexLock			m_set_uptflg_loc;//接收通知后设置升级标识加锁
	time_t					m_getupt_inv; //主动取数据间隔时间
private:
	var_4				    m_ser_upt_flg;// 服务端升级标识
	var_1				    m_lst_savedir[256];
	CP_MutexLock		    m_lst_fileloc;// 主动更新与手动更新请求文件互斥锁
	UC_Allocator_Recycle    m_mem_alloc;

	var_4					m_stop_load_flg; //如果不停更新列表，停止载入当前列表
	CP_MutexLock			m_stop_load_loc;

	UC_Communication_Client *m_com_cli;//文件请求客户端

	var_4 cs_fun_package(const var_1* in_buf, const var_4 in_buf_size, const var_vd* handle = NULL) //接收更新通知
	{
		if(strncmp(in_buf,"LISTMGR_SERVER_NEWDATA",22))
		{
			m_set_uptflg_loc.lock();
			m_ser_upt_flg = -1;
			m_set_uptflg_loc.unlock();
			return -1;
		}
		else
		{
			m_set_uptflg_loc.lock();
			m_ser_upt_flg  = 0;
			m_set_uptflg_loc.unlock();
#ifdef _TEST_NOTIFY_SERVER
			printf("client set upt flg to 0.\n");
#endif
		}
		return 0;
	}

	var_4 cs_fun_process(const var_1* in_buf, const var_4 in_buf_size, const var_1* out_buf, const var_4 out_buf_size, var_vd* handle = NULL)
	{
		return 0;
	}


	var_4 recv_file(var_8 file_len,var_vd *handle, var_1 *save_path)
	{
		FILE *fp = fopen(save_path, "wb");
		if(!fp)
			return -1;
		var_8 remain_len = file_len;
		var_4 buf_len = 1024*1024;
		var_4 recv_len = 0;
		var_4 recv_buf_len = buf_len;
		var_1 *recv_buf = new var_1[buf_len];
		while(remain_len > 0)
		{
			if(remain_len < buf_len)
				recv_buf_len = remain_len;
			recv_len = m_com_cli->recv(handle,recv_buf,recv_buf_len);
			if(recv_len == -1)
			{
				//close(handle);
				fclose(fp);
				delete []recv_buf;
				return -1;
			}
			remain_len -= recv_buf_len;

			if(fwrite(recv_buf,recv_buf_len,1,fp) != 1)
			{
				fclose(fp);
				delete []recv_buf;
				return -1;
			}
		}
		fclose(fp);
		delete []recv_buf;
		return 0;
	}

	var_4 req_lst_file(var_1 *save_path)
	{
		var_vd *handle;
		var_1 com_buf[1024];
		var_8 file_len; 
		if(m_com_cli->open(handle))
		{
			return -1;
		}

		*(var_8*)com_buf = *(var_8 *)"LISTMGR1";
		*(var_4 *)(com_buf + 8) = 1;
		if(m_com_cli->send(handle,com_buf,12))
		{
			m_com_cli->close(handle);
			return -2;
		}
		if(m_com_cli->recv(handle,com_buf,16))
		{
			m_com_cli->close(handle);
			return -3;
		}
		if(*(var_8 *)com_buf !=  *(var_8*)"LISTMGR1")
		{
			m_com_cli->close(handle);
			return -6;
		}
		file_len = *(var_8 *)(com_buf+8);
		if(recv_file(file_len,handle,save_path))
		{
			m_com_cli->close(handle);
			return -4;
		}
		strcpy(com_buf,"LISTMGRCLRECVSUC");
		if(m_com_cli->send(handle,com_buf,16))
		{
			m_com_cli->close(handle);
			return -5;
		}
		m_com_cli->close(handle);
		return 0;
	}

	var_4 req_ser_lstversion(var_u8 &lst_version)
	{
		var_vd * handle;
		var_1 com_buf[1024];
		strcpy(com_buf,"LISTMGR1");
		*(var_4*)(com_buf + 8) = 0;
		if(m_com_cli->open(handle))
		{
			return -1;
		}
		if(m_com_cli->send(handle,com_buf,12))
		{
			m_com_cli->close(handle);
			return -2;
		}
		if(m_com_cli->recv(handle,com_buf,16))
		{
			m_com_cli->close(handle);
			return -3;
		}
		if(*(var_8*)(com_buf) !=*(var_8*)"LISTMGR1")
			return -4;
		lst_version = *(var_u8 *)(com_buf+8);
		return 0;
	}

private:
	static CP_THREAD_T thr_req_newlst_file(void *argv)
	{
		UC_ListManager_Client *cur_client = (UC_ListManager_Client *) argv;
		cur_client->process_getuptlst();
		return 0;
	}
	static CP_THREAD_T thr_active_req(void *argv)
	{
		UC_ListManager_Client *cur_client = (UC_ListManager_Client *) argv;
		cur_client->process_upt_notify();
		return 0;
	}
};

class UC_ListManager_Server : public UC_ListManager,public UC_Communication_Server
{
public:
	//////////////////////////////////////////////////////////////////////
	// 函数:      init
	// 功能:      初始化列表管理器服务端
	// 入参:
	//                  save_path: 列表保存路径
	//                listen_port: 监听端口号
	//         client_update_push: 是否给客户端发送更新通知
	//                 client_num: 要接收更新通知的客户端数量
	//                  client_ip: 要接收更新通知的客户端ip列表
	//                client_port: 要接收更新通知的客户端port列表
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 init(var_1* save_path, var_u2 listen_port, var_4 client_update_push = 0, var_4 client_num = 0, var_1** client_ip = NULL, var_u2* client_port = NULL)
	{
		var_4 time_out = 120000;
	
		if(init_list())
		{
			printf("list manager. init list manager failed.\n");
			return -3;
		}
		cp_create_dir(save_path);
		sprintf(list_path,"%s_curlist.dat",save_path);
		m_fp_w = fopen(list_path,"wb");
		if(!m_fp_w)
		{
			printf("uc list manager server. can't open data file due to : %s \n",strerror(errno));
			return -5;
		}
		var_u8 init_version = cp_get_uuid();
		if(fwrite(&init_version,8,1,m_fp_w)!=1)
		{
			fclose(m_fp_w);
			m_fp_w = NULL;
			return -6;
		}
		fflush(m_fp_w);

		var_1 *alloc_ipbuf = new var_1[1<<20];
		m_notify_cli_ip = new var_1 *[client_num];
		m_notify_cli_port = new var_u2[client_num];
		for (int i =0; i<client_num; i++)
		{
			m_notify_cli_ip[i] = alloc_ipbuf + i*32;
			strcpy(m_notify_cli_ip[i],client_ip[i]);
			m_notify_cli_port[i] = client_port[i];
		}

		m_num_lstcli = client_num;
		m_notify_upt_cli = new UC_Communication_Client;
		m_notify_upt_cli->init(*client_ip,*client_port,time_out);

		if(UC_Communication_Server::init(listen_port,client_num,time_out,0,12))
		{
			printf("list manager server. init list data server failed.\n");
			return -1;
		}

		if(load_new_lst(list_path) == 0)
		{
			swap_cur_list();
		}
		if(start())
		{
			return -4;
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      add
	// 功能:      增加key到当前列表
	// 入参:
	//                    key_buf: 增加的key
	//                    key_len: 增加的key长度
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 add(var_1* key_buf, var_4 key_len)
	{
		//保持与文件同步
		var_u8 key_md5 = m_md5_con.MD5Bits64((var_u1 *)key_buf,key_len);
		var_8 cur_filesize = cp_get_file_size(list_path);
		var_4 ret = write_rec(key_md5,key_buf,key_len);
		if(ret)
		{
			printf("list manager. write new rec to file failed.\n");
			return -1;
		}
	//	fflush(m_fp_w);
		ret = add_key(key_buf,key_len);
		if(ret) 
		{
			printf("list manager. add new key failed.: errno : %d\n", ret);
			FILE *fp = fopen(list_path,"rb+");
			if(!fp)
			{
				return -2;
			}
			cp_change_file_size(fp,cur_filesize);//写文件错误时恢复原有数据
			return -1;
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      add
	// 功能:      增加key到当前列表
	// 入参:
	//                    key_val: 增加的key
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 add(var_u8 key_val)
	{
		var_8 cur_filesize = cp_get_file_size(list_path);
		var_4 ret = write_rec(key_val);
		if(ret)
		{
			printf("list manager. write new rec to file failed.\n");
			return -1;
		}
		ret = add_key(key_val);
		if(ret) 
		{
			printf("list manager. add new key failed.: errno : %d\n", ret);
			FILE *fp = fopen(list_path,"rb+");
			if(!fp)
			{
				return -2;
			}
			cp_change_file_size(fp,cur_filesize);
			return -1;
		}
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      del
	// 功能:      从当前列表删除key
	// 入参:
	//                    key_buf: 删除的key
	//                    key_len: 删除的key长度
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 del(var_1* key_buf, var_4 key_len)
	{
		var_4 ret = query_set(key_buf,key_len);
		var_1 delpath[256];
		if(ret)
			return -1;
		m_file_loc.lock();
		if(m_fp_w != NULL)
		{
			fclose(m_fp_w);
			m_fp_w = NULL;
		}
		ret = del_key(key_buf,key_len);
		if(ret)
		{
			m_file_loc.unlock();
			return -2;
		}
		sprintf(delpath,"%s.delbak",list_path);
		ret = save_curlist(delpath);
		if(ret)
		{
			printf("Uc list manager. del key failed.\n. errno : %d\n",ret);
			if(add_key(key_buf,key_len))
			{
				m_file_loc.unlock();
				return -3;
			}
			m_file_loc.unlock();
			return ret;
		}
		//cp_swap_file(delpath,list_path);
		if(remove(list_path))
		{
			printf("uc list manager. opt of removing old file after delation has been failed due to :%s\n", strerror(errno));
		}
		if(rename(delpath,list_path))
		{
			printf("uc list manager. replacment of old file after delation has been failed due to :%s\n", strerror(errno));
		}

		m_file_loc.unlock();
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 函数:      del
	// 功能:      从当前列表删除key
	// 入参:
	//                    key_val: 删除的key
	//
	// 出参:
	// 返回值:    成功返回0,否则返回错误码
	// 备注:
	//////////////////////////////////////////////////////////////////////
	var_4 del(var_u8 key_val)
	{
		var_1 delpath[256];
		var_1* vpBuf; 
		var_1 key_str[256];
		key_str[0] = 0;
		sprintf(delpath,"%s.delbak",list_path);
		
		if(get_keybuf(key_val,vpBuf))//查看是否存在并得到附带的数据
		{
			return -1;
		}
		if (vpBuf != NULL)
		{
			if(*vpBuf != 0)
				memcpy(key_str,vpBuf,256);
		}
		m_file_loc.lock();
		if(m_fp_w != NULL)
		{
			fclose(m_fp_w);
			m_fp_w = NULL; 
		}
		var_4 ret = del_key(key_val);
		if(ret)
		{
			printf("Uc list manager. del key failed.\n. errno: %d\n",ret);
			m_file_loc.unlock();
			return ret;
		}
		ret = save_curlist(delpath);
		if(ret)
		{
			printf("uc list manager. save file after delation failed.\n");
			if(*vpBuf != 0)
			{
				if(add_key(key_str,strlen(key_str)))
				{
					printf("uc list manager. recover list after a failed opt of delation has been failed.\n");
					m_file_loc.unlock();
					return -5;
				}
			}
			else
			{
				if(add_key(key_val))
				{
					printf("uc list manager. recover list after a failed opt of delation has been failed.\n");
					m_file_loc.unlock();
					return -5;
				}
			}
			m_file_loc.unlock();
			return -1;
		}
		if(remove(list_path))
		{
			printf("uc list manager. opt of removing old file after delation has been failed due to :%s\n", strerror(errno));
		}
		if(rename(delpath,list_path))
		{
			printf("uc list manager. replacment of old file after delation has been failed due to :%s\n", strerror(errno));
		}
		m_file_loc.unlock();
		return 0;
	}

	//手动设置发送更新标识
	var_4 set_notify()
	{
		m_upt_loc.lock();
		if(m_upt_flg != 1)
			m_upt_flg = 1;
		m_upt_loc.unlock();
		return 0;
	}

	var_4 func_snd_notify()
	{
		bool finish_flg = false;
		while(finish_flg == false)
		{
			m_upt_loc.lock();
			if(m_upt_flg == 1)
			{
				m_upt_flg = 0;
				m_upt_loc.unlock();
				if(notify_upt())
				{
					printf("uc list manager server. send upt notify to clients failed.\n");
					continue;
				}
			}
			else
				m_upt_loc.unlock();
			cp_sleep(200);

		}
		return 0;
	}
	var_4 start()
	{
		if (start_data_server())
		{
			printf("uc list manager. Start list manager data server failed.\n");
			return -1;
		}

		if (cp_create_thread(thr_notify_upt,this))
		{

			printf("uc list manager. Start notify client failed.\n");
			return -2;
		}
#ifdef _TEST_NOTIFY_SERVER
		printf("uc list manager server. notify service started.\n");
#endif
		if(cp_create_thread(thr_activate_notify,this))
		{
			printf("uc list manager. Start automatic notify client failed.\n");
			return -3;
		}
		return 0;
	}

	var_4 open_datfile()//打开数据文件写入
	{
		if(m_fp_w != NULL)
			return 0;
		m_file_loc.lock();
		m_fp_w = fopen(list_path,"ab");
		if(!m_fp_w)
		{
			m_file_loc.unlock();
			return -1;
		}
		m_file_loc.unlock();
		return 0;
	}

	var_4 close_datfile()
	{
		if(m_fp_w == NULL)
			return 0;
		m_file_loc.lock();
		fclose(m_fp_w);
		m_file_loc.unlock();
		return 0;
	}

private:

	var_1				list_path[256];
	UC_MD5				m_md5_con;
	
	var_4				m_upt_flg;
	CP_MutexLock		m_upt_loc;

	FILE				*m_fp_w;//数据文件写入指针
	CP_MutexLock		m_file_loc;//数据文件写入时互斥锁

	CP_MutexLock		m_delkey_loc;

	UC_Communication_Client *m_notify_upt_cli;

	var_1				**m_notify_cli_ip;
	var_u2				*m_notify_cli_port;

	var_4				m_num_lstcli;

	var_4 write_rec(var_u8 key_md5, var_1 *key_buf = NULL, var_4 key_buflen = -1)
	{
		m_file_loc.lock();
		var_u8 uuid = cp_get_uuid();
		if(m_fp_w == NULL)
		{
			m_fp_w = fopen(list_path,"ab");
			if(!m_fp_w)
			{
				m_file_loc.unlock();
				return -1;
			}
		}
		fseek(m_fp_w,0,SEEK_SET);
		if(fwrite(&uuid,8,1,m_fp_w) != 1)
		{
			printf("list manager server. update list version failed.\n");
			fclose(m_fp_w);
			m_fp_w = NULL;
			m_file_loc.unlock();
			return -2;
		}
		fseek(m_fp_w,0,SEEK_END);
		if(fwrite(&key_md5,8,1,m_fp_w) != 1)
		{
			printf("list manager server. Write new rec failed.\n");
			fclose(m_fp_w);
			m_fp_w = NULL;
			m_file_loc.unlock();
			return -3;
		}
		if(fwrite(&key_buflen,4,1,m_fp_w) != 1)
		{
			printf("list manager server. Write new rec failed.\n");
			fclose(m_fp_w);
			m_fp_w = NULL;
			m_file_loc.unlock();
			return -4;
		}
		if(key_buflen != -1)
		{
			if(fwrite(key_buf,key_buflen,1,m_fp_w) != 1)
			{
				printf("list manager server. Write new rec failed.\n");
				fclose(m_fp_w);
				m_fp_w = NULL;
				m_file_loc.unlock();
				return -5;
			}
		}
		fflush(m_fp_w);
		m_file_loc.unlock();
		return 0;
	}

	var_4  write_cur_rec( FILE *fp,var_u8 key_md5, var_1 *key_buf = NULL, var_4 key_buflen = -1)
	{
		if(fwrite(&key_md5,8,1,fp) != 1)
		{
			printf("list manager server. Write new rec failed.\n");
			return -3;
		}
		if(fwrite(&key_buflen,4,1,fp) != 1)
		{
			printf("list manager server. Write new rec failed.\n");
			return -4;
		}
		if(key_buflen != -1 && key_buflen != 0)
		{
			if(fwrite(key_buf,key_buflen,1,fp) != 1)
			{
				printf("list manager server. Write new rec failed.\n");
				return -5;
			}
		}
		fflush(fp);
		return 0;
	}
	var_4  save_curlist(var_1 *save_path)
	{
		var_vd *cur_handle ;
		var_4 ret = 0;
		var_u8 cur_md5;
		var_1 *vpBuf;
		//var_1 save_path[256];
		FILE *fp = fopen(save_path,"wb");
		if (!fp )
		{
			printf("uc list manager server. save curlist failed: open error with reason : %s\n",strerror(errno));
			return -1;
		}
		cur_md5 = cp_get_uuid();//更新版本信息
		if(fwrite(&cur_md5,8,1,fp)!=1)
		{
			printf("uc list manager server. write new version indentifier failed.\n ");
			fclose(fp);
			return -1;
		}
		if(travel_prepare(cur_handle))
		{
			printf("uc list manager server. save cur list failed.\n");
			fclose(fp);
			return -1;
		}
		while (ret ==0)
		{
			ret = travel_key(cur_handle,cur_md5,&vpBuf);
			if(ret == 0)
			{
				if (write_cur_rec(fp,cur_md5,vpBuf,strlen(vpBuf)))
				{
					fclose(fp);
					travel_finsih(cur_handle);
					return -2;
				}
			}
		}
		travel_finsih(cur_handle);
		fclose(fp);
		return 0;
	}

	var_4 snd_upt_notify(var_vd *handle)
	{
		var_1 notify_buf[1024];
		strcpy(notify_buf,"LISTMGR_SERVER_NEWDATA");
		if(m_notify_upt_cli->send(handle,notify_buf,22))
		{
			return -1;
		}
		return 0;
	}

	var_4 notify_upt()
	{
		var_vd *handle;
		for (int i =0; i<m_num_lstcli; i++)
		{
			if(m_notify_upt_cli->open(handle,m_notify_cli_ip[i],m_notify_cli_port[i]))
			{
				continue;
			}
			if(snd_upt_notify(handle))
			{
				m_notify_upt_cli->close(handle);
				printf("uc list manager server . notify upt to client : %s failed. \n",m_notify_cli_ip[i]);
				continue;
			}
			m_notify_upt_cli->close(handle);
		}
		return 0;
	}

	var_4 snd_file( CP_SOCKET_T snd_soc)
	{
		var_8 file_size;
		var_4 buf_len = 1<<20;
		var_8 snd_len = 0;
		m_file_loc.lock();//发送中不允许写入操作
		if(m_fp_w != NULL)
		{
			fclose(m_fp_w);
			m_fp_w = NULL;
		}
		 file_size = cp_get_file_size((var_1 *)list_path);
		FILE *fp = fopen(list_path,"rb");
		if(!fp)
		{
			printf("can't open snd file :%s due to : %s\n",list_path,strerror(errno));
			m_file_loc.unlock();
			return -2;
		}
		var_1 *snd_buf = new var_1[buf_len];
		*(var_8*)snd_buf = *(var_8*)"LISTMGR1";
		*(var_8*)(snd_buf+8) = file_size;
		if(cp_sendbuf(snd_soc,snd_buf,16))
		{
			delete []snd_buf;
			fclose(fp);
			m_file_loc.unlock();
			return -1;
		}
		while(file_size > 0)
		{
			snd_len = buf_len;
			if(file_size < buf_len)
				snd_len = file_size;
			if(fread(snd_buf,snd_len,1,fp) != 1)
			{
				fclose(fp);
				delete []snd_buf;
				m_file_loc.unlock();
				return -3;
			}
			if(cp_sendbuf(snd_soc,snd_buf,snd_len))
			{
				fclose(fp);
				delete []snd_buf;
				m_file_loc.unlock();
				return -1;
			}
			file_size -= snd_len;
		}
		delete []snd_buf;
		m_file_loc.unlock();
		return 0;
	}

	var_8 get_file_version()
	{
		FILE *fp = fopen(list_path,"rb");
		if(!fp)
		{
			return -1;
		}
		var_8 file_version = 0;
		m_file_loc.lock();
		if(fread(&file_version,8,1,fp) != 1)
		{
			fclose(fp);
			m_file_loc.unlock();
			return -1;
		}
		fclose(fp);
		m_file_loc.unlock();
		return file_version;
	}
	var_4 snd_file_version(CP_SOCKET_T snd_soc)
	{
		var_u8 file_version = get_file_version();
		var_1 snd_buf[1024];
		*(var_8*)snd_buf = *(var_8*)"LISTMGR1";
		*(var_8 *)(snd_buf+8) = (var_8)file_version;
		if(cp_sendbuf(snd_soc,snd_buf,16))
		{
			return -1;
		}
		return 0;
	}
	var_4 start_data_server()
	{
		for (var_4 i =0; i<m_listen_num;i++)
		{
			m_success_flag = 0;
			if(cp_create_thread(cs_file_transfer_ser,this))
				return -1;
#ifdef UC_LISTMGR_DEBUG
			printf("List data file server started.\n");
#endif
			while (m_success_flag)
			{
				cp_sleep(1);
			}
			if(m_success_flag)
				return -1;
		}
		return 0;
	}

	var_4 cs_fun_package(const var_1* in_buf, const var_4 in_buf_size, const var_vd* handle = NULL) 
	{
		return 0;
	}

	var_4 fun_package(const var_1* in_buf, const var_4 in_buf_size, var_4 &req_type, const var_vd* handle = NULL)
	{
		if(*(var_8*) in_buf != *(var_8*)"LISTMGR1")
			return -1;
		req_type = *(var_4 *)(in_buf + 8);
		return 16;
	}

	var_4 cs_fun_process(const var_1* in_buf, const var_4 in_buf_size, const var_1* out_buf, const var_4 out_buf_size, var_vd* handle = NULL)
	{
		if(strncmp(in_buf,"LISTMGRCLRECVSUC",16))
		{
			return -1;
		}
		return 0;
	}

	static CP_THREAD_T cs_file_transfer_ser(void *argv)
	{
		//var_1 recv_header[1024];
		UC_ListManager_Server* cs = (UC_ListManager_Server*)argv;

		var_1 recv_buf[1024];
		var_4  recv_len = 0;
		var_1 send_buf[1024];
		var_4  send_len = 0;
		var_4 req_type;
		//var_1* temp_buf = NULL;

		cs->m_success_flag = 1;

#ifdef UC_LISTMGR_DEBUG
		printf("data transfer ser started.\n");
#endif
		for(;;)
		{
			CP_SOCKET_T socket;

			if(cp_accept_socket(cs->m_listen, socket))
			{
				cp_sleep(1);
				continue;
			}

			if(cp_set_overtime(socket, cs->m_time_out))
			{
				cp_close_socket(socket);

				printf("UC_Communication_Server.cs_daemon_thread - set time out error, value = %d\n", cs->m_time_out);
				continue;
			}

			if(cp_recvbuf(socket, recv_buf, cs->m_head_size))
			{
				cp_close_socket(socket);

				printf("UC_Communication_Server.cs_daemon_thread - recv head error, size = %d\n", cs->m_head_size);
				continue;
			}

			recv_len = cs->fun_package(recv_buf, cs->m_head_size,req_type);
			if(recv_len < 0)
			{
				cp_close_socket(socket);
				printf("UC_Communication_Server.cs_daemon_thread - cs_fun_package error, code = %d\n", recv_len);
				continue;
			}

			else if(recv_len > 0)
			{
				if(req_type == 0)
				{
					if(cs->snd_file_version(socket))
					{
						cp_close_socket(socket);
						continue;
					}
					cp_close_socket(socket);
					continue;
				}
				else if(req_type == 1)
				{
					if(cs->snd_file(socket))
					{
						printf("UC_ListComMgr.thread_sndfile cs snd list file failed.\n");
						cp_close_socket(socket);
						continue;
					}
				}

				if(recv_len > cs->m_min_recv_size)
				{
					if(recv_len > cs->m_max_recv_size)
					{
						cp_close_socket(socket);

						printf("UC_Communication_Server.cs_daemon_thread - recv length(%d) > max recv length(%d)\n", recv_len, cs->m_max_recv_size);
						continue;
					}
					if(recv_buf == NULL)
					{
						cp_close_socket(socket);

						printf("UC_Communication_Server.cs_daemon_thread - alloc recv memory error, size = %d)\n", recv_len);
						continue;
					}
				}

				if(cp_recvbuf(socket, recv_buf, recv_len))
				{
					cp_close_socket(socket);

					printf("UC_Communication_Server.cs_daemon_thread - recv body error, size = %d)\n", recv_len);
					continue;
				}
			}

			send_len = cs->cs_fun_process(recv_buf, recv_len, send_buf, cs->m_max_send_size);
			if(send_len < 0)
			{
				cp_close_socket(socket);

				printf("UC_Communication_Server.cs_daemon_thread - cs_fun_process error, code = %d\n", send_len);
				continue;
			}

			cp_close_socket(socket);
		}

		return 0;
	}

	static CP_THREAD_T thr_notify_upt(void *argv)
	{
		UC_ListManager_Server *lst_ser = (UC_ListManager_Server *)argv;
		lst_ser->func_snd_notify();
		return 0;
	}
	
	static CP_THREAD_T thr_activate_notify(void *argv)
	{
		UC_ListManager_Server *lst_ser = (UC_ListManager_Server *)argv;
		time_t cur_time = time(NULL);
		time_t past_time = cur_time;
		bool finish_flg = false;
		time_t interval = 120000;
		var_u8 last_version = 0;
		var_8 new_version = 0;
		while(finish_flg == false)
		{
			if(cur_time - past_time > interval)
			{
				new_version = lst_ser->get_file_version();

				if(new_version > 0)
				{
					if((var_u8)new_version != last_version)
						lst_ser->set_notify();
				}
			}
			cp_sleep(10000);
		}
		return 0;
	}
};

class UC_ListManager_Manager : public UC_ListManager
{

};

#endif // _UC_LIST_MANAGER_H_
