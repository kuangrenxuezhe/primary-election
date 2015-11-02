// video_integration.h

#include "UH_Define.h"
#include "UT_HashTable_Lite.h"
#include "UC_Mem_Allocator_Recycle.h"

#define CLUSTER_NUM		4

template<class T_Type>
var_vd bubble_sort(T_Type* lst, var_4 num)
{
	for(var_4 i = 0; i <= num - 1; i++) 
	{
		var_4 flag = 0;

		for(var_4 j = num - 1; j > i; j--) 
		{
			if (lst[j] < lst[j - 1]) 
			{
				T_Type temp = lst[j]; 
				lst[j] = lst[j - 1]; 
				lst[j - 1] = temp;

				flag = 1;
			}
		}

		if(flag == 0)
			break;
	}
}

template <class T_Type>
class dynamic_array
{
public:
	dynamic_array()
	{
		m_cnum = 0;
		m_size = 8;
		m_base = 4;

		m_buff = NULL;
	}
	~dynamic_array()
	{
		if(m_buff)
			delete m_buff;
	}

	var_4 cpy_buf(dynamic_array<T_Type>* source)
	{
		var_8   src_num = source->get_num();
		T_Type* src_buf = source->get_buf();

		if(set_buf(src_buf, src_num))
			return -1;

		return 0;
	}

	var_4 cat_buf(T_Type* value, var_8 size = 1)
	{
		var_8 n_size = size + m_cnum;

		if(m_buff && n_size <= m_size)
		{
			memcpy(m_buff + m_cnum, value, sizeof(T_Type) * size);
			m_cnum += n_size;

			return 0;
		}

		var_8 t_size = m_size;

		if(n_size > m_size)
			t_size = (n_size + m_base - 1) / m_base;

		T_Type* n_buff = new T_Type[t_size];
		if(n_buff == NULL)
			return -1;

		m_size = t_size;

		if(m_buff)
		{
			memcpy(n_buff, m_buff, sizeof(T_Type) * m_cnum);
			delete m_buff;
		}

		memcpy(n_buff + m_cnum, value, sizeof(T_Type) * size);
		m_cnum += n_size;

		m_buff = n_buff;
		
		return 0;
	}

	var_4 set_buf(T_Type* value, var_8 size)
	{
		if(m_buff && size <= m_size)
		{
			memcpy(m_buff, value, sizeof(T_Type) * size);
			m_cnum = size;

			return 0;
		}

		var_8 t_size = m_size;

		if(size > m_size)
			t_size = (size + m_base - 1) / m_base;

		T_Type* n_buff = new T_Type[t_size];
		if(n_buff == NULL)
			return -1;

		if(m_buff)
			delete m_buff;

		memcpy(n_buff, value, sizeof(T_Type) * size);
		m_cnum = size;
		
		m_buff = n_buff;

		return 0;
	}

	inline T_Type* get_buf()
	{
		return m_buff;
	}

	inline var_8 get_num()
	{
		return m_cnum;
	}

	var_vd cls_buf()
	{
		m_cnum = 0;
	}

private:
	T_Type* m_buff;

	var_8   m_cnum;
	var_8   m_size;
	var_8   m_base;
};

typedef struct _xml_info_
{
	var_u8 id;
	var_u8 md5;
	var_1* pos;
	var_4  len;
} XML_INFO;

typedef struct _u_node_
{
	var_u8           id;
	struct _u_node_* next;
} U_NODE;

typedef struct _c_node_
{
	dynamic_array<var_u8> list;

    dynamic_array<var_u8> info;
	var_4				  size[CLUSTER_NUM];
	
	var_u8           id;
	struct _c_node_* next;
} C_NODE;

typedef struct _i_node_
{
    C_NODE* sid_lst;
    var_4   sid_num;
    
    C_NODE* lid_lst;
    var_4   lid_num;
    
	dynamic_array<var_u8> ofi_lst;
	dynamic_array<var_u8> ofi_md5;
    var_4				  ofi_num;
    
	dynamic_array<var_u8> obi_lst;
	dynamic_array<var_u8> obi_md5;
	var_4				  obi_num;
} I_NODE;

class video_integration
{
public:
	var_4 init()
	{
		if(m_hs_nid.init(1000000, sizeof(I_NODE*)))
			return -1;
		if(m_hs_sid.init(1000000, sizeof(U_NODE*)))
			return -1;
		if(m_hs_lid.init(1000000, sizeof(U_NODE*)))
			return -1;
				
		if(m_lib_INODE.init(sizeof(I_NODE)))
			return -1;
		if(m_lib_CNODE.init(sizeof(C_NODE)))
			return -1;
		if(m_lib_UNODE.init(sizeof(U_NODE)))
			return -1;

		return 0;
	}
 
	var_vd add_sid(var_u8 sid, var_u8 n_nid, var_u8 o_nid, var_u8** c_info, var_4* c_size)
	{
		m_global_lck.lock_r();

		m_hs_nid.key_lock(n_nid);

		if(n_nid != o_nid)
			m_hs_nid.key_lock(o_nid);

		add_sid(sid, n_nid, c_info, c_size, 0);

		if(n_nid != o_nid)
		{
			del_sid(sid, o_nid, 0);
			m_hs_nid.key_unlock(o_nid);
		}

		m_hs_nid.key_unlock(n_nid);

		m_global_lck.unlock();
	}
	var_vd add_sid(var_u8 sid, var_u8 nid, var_u8** c_info, var_4* c_size, var_4 is_lck = 1)
	{
		I_NODE* i_node = NULL;
		C_NODE* c_node = NULL;
		var_vd* r_buff = NULL;

		if(is_lck)
		{
			m_global_lck.lock_r();
			m_hs_nid.key_lock(nid);
		}

		if(m_hs_nid.query(nid, r_buff))
		{
			i_node = (I_NODE*)m_lib_INODE.get_mem();

			while(m_hs_nid.add(nid, &i_node))
			{
				printf("video_integration.add_sid m_hs_nid.add failure\n");
				cp_sleep(1000);
			}

			c_node = (C_NODE*)m_lib_CNODE.get_mem();

			c_node->id = sid;
			c_node->next = NULL;

			for(var_4 i = 0; i < CLUSTER_NUM; i++)
			{
				bubble_sort<var_u8>(c_info[i], c_size[i]);

				while(c_node->info.cat_buf(c_info[i], c_size[i]))
				{
					printf("video_integration.add_sid info.catbuf failure\n");
					cp_sleep(1000);
				}

				c_node->size[i] = c_size[i];
			}

			i_node->sid_lst = c_node;
			i_node->sid_num = 1;
		}
		else
		{
			i_node = *(I_NODE**)r_buff;

			c_node = i_node->sid_lst;
			for(var_4 i = 0; i < i_node->sid_num; i++)
			{
				if(c_node->id == sid)
					break;
				c_node = c_node->next;
			}

			if(c_node == NULL)
			{
				c_node = (C_NODE*)m_lib_CNODE.get_mem();

				c_node->id = sid;

				c_node->next = i_node->sid_lst;
				i_node->sid_lst = c_node;
				i_node->sid_num++;
			}
			else
				c_node->info.cls_buf();

			for(var_4 i = 0; i < CLUSTER_NUM; i++)
			{
				bubble_sort<var_u8>(c_info[i], c_size[i]);

				while(c_size[i] && c_node->info.cat_buf(c_info[i], c_size[i]))
				{
					printf("video_integration.add_sid info.catbuf failure\n");
					cp_sleep(1000);
				}

				c_node->size[i] = c_size[i];
			}
		}

		if(is_lck)
		{
			m_hs_nid.key_unlock(nid);
			m_global_lck.unlock();
		}
	}
	var_4 del_sid(var_u8 sid, var_u8 nid, var_4 is_lck = 1)
	{
		var_vd* r_buff = NULL;

		if(is_lck)
		{
			m_global_lck.lock_r();
			m_hs_nid.key_lock(nid);
		}

		if(m_hs_nid.query(nid, r_buff))
		{
			if(is_lck)
			{
				m_hs_nid.key_unlock(nid);
				m_global_lck.unlock();
			}

			return 1;
		}

		I_NODE* i_node = *(I_NODE**)r_buff;
		
		C_NODE* c_node = i_node->sid_lst;
		C_NODE** ptr = &(i_node->sid_lst);

		for(var_4 i = 0; i < i_node->sid_num; i++)
		{
			if(c_node->id == sid)
				break;
			ptr = &(c_node->next);
			c_node = c_node->next;
		}

		if(c_node == NULL)
		{
			if(is_lck)
			{
				m_hs_nid.key_unlock(nid);
				m_global_lck.unlock();
			}

			return 2;
		}

		*ptr = c_node->next;

		m_lib_CNODE.put_mem(c_node);

		if(is_lck)
		{
			m_hs_nid.key_unlock(nid);
			m_global_lck.unlock();
		}

		return 0;
	}

	var_vd add_lid(var_u8 lid, var_u8 n_nid, var_u8 o_nid, var_u8** c_info, var_4* c_size)
	{
		m_global_lck.lock_r();

		m_hs_nid.key_lock(n_nid);

		if(n_nid != o_nid)
			m_hs_nid.key_lock(o_nid);

		add_lid(lid, n_nid, c_info, c_size, 0);
		
		if(n_nid != o_nid)
		{
			del_lid(lid, o_nid, 0);
			m_hs_nid.key_unlock(o_nid);
		}

		m_hs_nid.key_unlock(n_nid);

		m_global_lck.unlock();
	}
	var_vd add_lid(var_u8 lid, var_u8 nid, var_u8** c_info, var_4* c_size, var_4 is_lck = 1)
	{
		I_NODE* i_node = NULL;
		C_NODE* c_node = NULL;
		var_vd* r_buff = NULL;

		if(is_lck)
		{
			m_global_lck.lock_r();
			m_hs_nid.key_lock(nid);
		}

		if(m_hs_nid.query(nid, r_buff))
		{
			i_node = (I_NODE*)m_lib_INODE.get_mem();

			while(m_hs_nid.add(nid, &i_node))
			{
				printf("video_integration.add_lid m_hs_nid.add failure\n");
				cp_sleep(1000);
			}

			c_node = (C_NODE*)m_lib_CNODE.get_mem();

			c_node->id = lid;
			c_node->next = NULL;

			for(var_4 i = 0; i < CLUSTER_NUM; i++)
			{
				bubble_sort<var_u8>(c_info[i], c_size[i]);

				while(c_node->info.cat_buf(c_info[i], c_size[i]))
				{
					printf("video_integration.add_lid info.catbuf failure\n");
					cp_sleep(1000);
				}

				c_node->size[i] = c_size[i];
			}

			i_node->lid_lst = c_node;
			i_node->lid_num = 1;
		}
		else
		{
			i_node = *(I_NODE**)r_buff;

			c_node = i_node->lid_lst;
			for(var_4 i = 0; i < i_node->lid_num; i++)
			{
				if(c_node->id == lid)
					break;
				c_node = c_node->next;
			}

			if(c_node == NULL)
			{
				c_node = (C_NODE*)m_lib_CNODE.get_mem();

				c_node->id = lid;

				c_node->next = i_node->lid_lst;
				i_node->lid_lst = c_node;
				i_node->lid_num++;
			}
			else
				c_node->info.cls_buf();

			for(var_4 i = 0; i < CLUSTER_NUM; i++)
			{
				bubble_sort<var_u8>(c_info[i], c_size[i]);

				while(c_size[i] && c_node->info.cat_buf(c_info[i], c_size[i]))
				{
					printf("video_integration.add_lid info.catbuf failure\n");
					cp_sleep(1000);
				}

				c_node->size[i] = c_size[i];
			}
		}

		if(is_lck)
		{
			m_hs_nid.key_unlock(nid);
			m_global_lck.unlock();
		}
	}
	var_4 del_lid(var_u8 lid, var_u8 nid, var_4 is_lck = 1)
	{
		var_vd* r_buff = NULL;

		if(is_lck)
		{
			m_global_lck.lock_r();
			m_hs_nid.key_lock(nid);
		}

		if(m_hs_nid.query(nid, r_buff))
		{
			if(is_lck)
			{
				m_hs_nid.key_unlock(nid);
				m_global_lck.unlock();
			}

			return 1;
		}

		I_NODE* i_node = *(I_NODE**)r_buff;

		C_NODE* c_node = i_node->lid_lst;
		C_NODE** ptr = &(i_node->lid_lst);

		for(var_4 i = 0; i < i_node->lid_num; i++)
		{
			if(c_node->id == lid)
				break;
			ptr = &(c_node->next);
			c_node = c_node->next;
		}

		if(c_node == NULL)
		{
			if(is_lck)
			{
				m_hs_nid.key_unlock(nid);
				m_global_lck.unlock();
			}

			return 2;
		}

		*ptr = c_node->next;

		m_lib_CNODE.put_mem(c_node);		

		if(is_lck)
		{
			m_hs_nid.key_unlock(nid);
			m_global_lck.unlock();
		}

		return 0;
	}

	var_vd add_uid(var_u8 sid, var_u8 lid)
	{
		m_global_lck.lock_r();
		m_user_lck.lock();

		add_sld(&m_hs_sid, sid, lid);
		add_sld(&m_hs_lid, lid, sid);

		m_user_lck.unlock();
		m_global_lck.unlock();
	}
	var_4  del_uid(var_u8 sid, var_u8 lid)
	{
		m_global_lck.lock_r();
		m_user_lck.lock();

		if(del_sld(&m_hs_sid, sid, lid))
		{
			m_user_lck.unlock();
			m_global_lck.unlock();

			return 1;
		}
		if(del_sld(&m_hs_lid, lid, sid))
		{
			m_user_lck.unlock();
			m_global_lck.unlock();

			return 1;
		}

		m_user_lck.unlock();
		m_global_lck.unlock();

		return 0;
	}

	var_4 output_beg()
	{
		m_global_lck.lock_w();
		return 0;
	}
	var_4 output_end()
	{
		m_global_lck.unlock();
		return 0;
	}

	var_4 output_nid(var_u8 nid, var_vd* global_info, var_1* tmp_use_buf, var_4 tmp_use_max,
					 var_u8* relation_buf, var_4 relation_max, var_4& relation_len,
					 var_u8* lst_fid_buf, var_4 lst_fid_max, var_4& lst_fid_len,
					 var_u8* lst_bid_buf, var_4 lst_bid_max, var_4& lst_bid_len,
					 var_1* fid_con_buf, var_1* fid_tmp_buf, var_4 fid_buf_max, var_4& fid_con_len,
					 var_1* bid_con_buf, var_1* bid_tmp_buf, var_4 bid_buf_max, var_4& bid_con_len)
	{
		var_vd* r_buff = NULL;

		if(m_hs_nid.query(nid, r_buff))
			return -1;

		relation_len = 0;

		lst_fid_len = 0;
		lst_bid_len = 0;
		fid_con_len = 0;
		bid_con_len = 0;

		// sid or lid is empty, delete all fid and bid
		I_NODE* i_node = *(I_NODE**)r_buff;
				
		if(i_node->sid_num == 0 || i_node->lid_num == 0)
		{			
			if(lst_fid_max < i_node->ofi_num)
				return 1;
			memcpy(lst_fid_buf, i_node->ofi_lst.get_buf(), i_node->ofi_num * sizeof(var_u8));
			lst_fid_len = i_node->ofi_num;

			i_node->ofi_lst.cls_buf();
			i_node->ofi_md5.cls_buf();
			i_node->ofi_num = 0;

			if(lst_bid_max < i_node->obi_num)
				return 1;
			memcpy(lst_bid_buf, i_node->obi_lst.get_buf(), i_node->obi_num * sizeof(var_u8));
			lst_bid_len = i_node->obi_num;

			i_node->obi_lst.cls_buf();
			i_node->obi_md5.cls_buf();
			i_node->obi_num = 0;

			return 0;
		}
		
		// cluster sid and lid
		C_NODE* sid = NULL;
		C_NODE* lid = i_node->lid_lst;

		for(var_4 i = 0; i < i_node->lid_num; i++, lid = lid->next)
		{
			var_4 relation_pos = relation_len;

			if(relation_len >= relation_max)
				return 2;			
			relation_buf[relation_len++] = lid->id;

			sid = i_node->sid_lst;

			for(var_4 j = 0; j < i_node->sid_num; j++, sid = sid->next)
			{
				if(i == 0)
					sid->list.cls_buf();

				if(cluster(sid, lid))
					continue;

				sid->list.cat_buf(&lid->id);

				if(relation_len >= relation_max)
					return 2;
				relation_buf[relation_len++] = sid->id;
			}

			var_vd* reference = NULL;
			if(m_hs_lid.query(lid->id, reference) == 0)
			{
				//???
			}

			if(relation_len - relation_pos == 1)
				relation_len = relation_pos;
			else
			{
				if(relation_len >= relation_max)
					return 2;
				relation_buf[relation_len++] = -1;
			}
		}
	
		// create output id list
		var_u8  xml_id_lst[256];
		var_u8* xml_id_pos = xml_id_lst;
		var_u8* xml_id_tmp = NULL;
		var_4   xml_id_num[128];
		var_4   xml_id_grp = 0;
				
		sid = i_node->sid_lst;

		for(var_4 i = 0; i < i_node->sid_num; i++, sid = sid->next)
		{
			var_8 num = sid->list.get_num();
			if(num <= 0)
				continue;
			
			if(256 - (xml_id_pos - xml_id_lst) < num + 1)
				return 3;
			
			*xml_id_pos++ = sid->id;

			memcpy(xml_id_pos, sid->list.get_buf(), num * sizeof(var_u8));
			xml_id_pos += num;

			//???
			if(xml_id_grp >= 128)
				return 2;
			xml_id_num[xml_id_grp++] = num + 1;
		}
		
		XML_INFO fid_lst[256];
		XML_INFO bid_lst[512];
		var_4 fid_num = 0;
		var_4 bid_num = 0;

		generate_xml(global_info, xml_id_lst, xml_id_num, xml_id_grp, tmp_use_buf, tmp_use_max,
					 fid_lst, 256, fid_num, fid_tmp_buf, fid_buf_max, bid_lst, 512, bid_num, bid_tmp_buf, bid_buf_max);
		
		// output fid_add_buf and fid_del_buf
		qs_xml_info(0, fid_num - 1, fid_lst);

		var_u8* ofi_lst = i_node->ofi_lst.get_buf();
		var_u8* ofi_md5 = i_node->ofi_md5.get_buf();
		var_u8  ofi_num = i_node->ofi_num;
						
		fid_con_len = 0;

		var_4 i = 0, j = 0;

		for(;;)
		{
			while(j < ofi_num && ofi_lst[j] < fid_lst[i].id)
			{
				if(lst_fid_len >= lst_fid_max)
					return 1;
				lst_fid_buf[lst_fid_len++] = ofi_lst[j++];
			}
			
			if(j >= ofi_num)
				break;

			while(i < fid_num && fid_lst[i].id < ofi_lst[j])
			{
				memcpy(fid_con_buf + fid_con_len, fid_lst[i].pos, fid_lst[i].len);
				fid_con_len += fid_lst[i++].len;
			}

			if(i >= fid_num)
				break;

			if(ofi_md5[j] != fid_lst[i].md5)
			{
				if(lst_fid_len >= lst_fid_max)
					return 1;
				lst_fid_buf[lst_fid_len++] = ofi_lst[j];

				memcpy(fid_con_buf + fid_con_len, fid_lst[i].pos, fid_lst[i].len);
				fid_con_len += fid_lst[i].len;
			}

			i++; j++;
		}

		while(j < ofi_num)
		{
			if(lst_fid_len >= lst_fid_max)
				return 1;
			lst_fid_buf[lst_fid_len++] = ofi_lst[j++];
		}

		while(i < fid_num)
		{
			memcpy(fid_con_buf + fid_con_len, fid_lst[i].pos, fid_lst[i].len);
			fid_con_len += fid_lst[i++].len;
		}
				
		// output fid_add_buf and fid_del_buf
		qs_xml_info(0, bid_num - 1, bid_lst);

		var_u8* obi_lst = i_node->obi_lst.get_buf();
		var_u8* obi_md5 = i_node->obi_md5.get_buf();
		var_u8  obi_num = i_node->obi_num;

		bid_con_len = 0;

		for(var_4 i = 0, j = 0;;)
		{
			while(j < obi_num && obi_lst[j] < bid_lst[i].id)
			{
				if(lst_bid_len >= lst_bid_max)
					return 1;
				lst_bid_buf[lst_bid_len++] = obi_lst[j++];
			}

			if(j >= obi_num)
				break;

			while(i < bid_num && bid_lst[i].id < obi_lst[j])
			{				
				memcpy(bid_con_buf + bid_con_len, bid_lst[i].pos, bid_lst[i].len);
				bid_con_len += bid_lst[i++].len;
			}

			if(i >= bid_num)
				break;

			if(obi_md5[j] != bid_lst[i].md5)
			{
				if(lst_bid_len >= lst_bid_max)
					return 1;
				lst_bid_buf[lst_bid_len++] = obi_lst[j];

				memcpy(bid_con_buf + bid_con_len, bid_lst[i].pos, bid_lst[i].len);
				bid_con_len += bid_lst[i].len;
			}

			i++; j++;
		}

		while(j < ofi_num)
		{
			if(lst_bid_len >= lst_bid_max)
				return 1;
			lst_bid_buf[lst_bid_len++] = obi_lst[j++];
		}

		while(i < fid_num)
		{
			memcpy(bid_con_buf + bid_con_len, bid_lst[i].pos, bid_lst[i].len);
			bid_con_len += bid_lst[i++].len;
		}

		// ??

		return 0;
	}

private:
	var_4 cluster(C_NODE* sid, C_NODE* lid)
	{
		return 0;
	}

	var_4 qs_xml_info(var_4 lBegin, var_4 lEnd, XML_INFO* tKey)
	{
		if(lBegin >= lEnd)
			return 0;

		XML_INFO tTmp;

		if(lBegin + 1 == lEnd)
		{
			if(tKey[lBegin].id > tKey[lEnd].id)
			{
				tTmp = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tTmp;
			}
			return 0;
		}

		var_4 lMid = (lBegin + lEnd)>>1;
		var_4 m = lBegin, n = lEnd;

		XML_INFO tMid = tKey[lMid];

		while(lBegin < lEnd)
		{
			while(lBegin < lEnd && tKey[lBegin].id < tMid.id) lBegin++;
			while(lBegin < lEnd && tKey[lEnd].id > tMid.id) lEnd--;
			if(lBegin < lEnd)
			{
				tTmp = tKey[lBegin];
				tKey[lBegin] = tKey[lEnd];
				tKey[lEnd] = tTmp;

				if(++lBegin < lEnd)
					lEnd--;
			}
		}

		if(tKey[lBegin].id < tMid.id)
			lBegin++;

		if(lBegin - 1 > m)
			qs_xml_info(m, lBegin - 1, tKey);
		if(lEnd < n)
			qs_xml_info(lEnd, n, tKey);

		return 0;
	}

	var_4 add_sld(UT_HashTable_Lite<var_u8>* hs, var_u8 m_id, var_u8 s_id)
	{
		U_NODE* u_node = NULL;
		U_NODE* t_node = NULL;
		var_vd* r_buff = NULL;

		if(hs->query(m_id, r_buff))
		{
			u_node = (U_NODE*)m_lib_UNODE.get_mem();

			while(hs->add(m_id, &u_node))
			{
				printf("video_integration.add_sld hs.add failure\n");
				cp_sleep(1000);
			}

			u_node->id = s_id;
			u_node->next = NULL;

			return;
		}

		u_node = *(U_NODE**)r_buff;
		t_node = u_node;

		while(t_node)
		{
			if(t_node->id == s_id)
				break;
			t_node = t_node->next;
		}

		if(t_node)
			return;

		t_node = (U_NODE*)m_lib_UNODE.get_mem();

		t_node->id = s_id;
		t_node->next = u_node->next;
		u_node->next = t_node;
	}
	var_4 del_sld(UT_HashTable_Lite<var_u8>* hs, var_u8 m_id, var_u8 s_id)
	{
		var_vd* r_buff = NULL;

		if(hs->query(m_id, r_buff))
			return 1;

		U_NODE* h_node = *(U_NODE**)r_buff;
		U_NODE* t_node = h_node;
		U_NODE* p_node = h_node;

		while(t_node)
		{
			if(t_node->id == s_id)
				break;

			p_node = t_node;
			t_node = t_node->next;
		}

		if(t_node == NULL)
			return 2;

		if(t_node == p_node)
			h_node = h_node->next;
		else
			p_node->next = t_node->next;

		if(h_node == NULL)
		{
			while(hs->del(m_id))
			{
				printf("video_integration.del_sld hs.del failure\n");
				cp_sleep(1000);
			}
		}
		else if(t_node == p_node)
		{
			while(hs->add(m_id, &h_node, NULL, 1) != 1)
			{
				printf("video_integration.del_sld hs.add failure\n");
				cp_sleep(1000);
			}
		}

		m_lib_UNODE.put_mem(t_node);

		return 0;
	}

	var_4 generate_xml(var_vd* vp_info, var_u8* in_id_lst, var_4* in_id_num, var_4 id_grp, var_1* tmp_use_buf, var_4 in_tmp_buf_size,
		XML_INFO* pst_sid_lst, var_4 in_sid_max, var_4& out_sid_fid, var_1* std_con_buf, var_4 in_sid_buf_size,
		XML_INFO* pst_tid_lst, var_4 in_tid_max, var_4& out_tid_num, var_1* tid_con_buf, var_4 in_tid_buf_size)
	{
		const var_4 I_DEF_MAX = 7;
		const var_4 I_PLAYER_MAX = 2;
#pragma pack(1)
		struct CLUSTER_INFO
		{
			var_4  pl_num;   //播放器条目数
			var_1 *ptr_pn;   //播放器名称指针
			var_2  pn_len;   //播放器名称长度
			struct TAG_INFO
			{
				var_4  def_num;        //清晰度条目数
				var_1 *ptr_def;   //清晰度名称指针
				var_2  def_len;   //清晰度名称长度
				struct LIST_INFO
				{
					OUTPUT_DETAIL_INFO *pst_di;
					OUTPUT_SOURCE_INFO *pst_si;
					LIST_INFO *pst_next;
				}*pst_head;
			}ast_tag[I_DEF_MAX];
		}ast_cluster[I_PLAYER_MAX]; 
		typedef CLUSTER_INFO::TAG_INFO::LIST_INFO CLUSTER_LIST_INFO;
#pragma pack()
		OUTPUT_STD_INFO    st_std;
		var_4 i = 0, num = 0, ret = 0, len = 0,t = 0;
		const var_4 I_TCI_SIZE = sizeof(CLUSTER_INFO::TAG_INFO); 
		const var_4 I_CLI_SIZE = sizeof(CLUSTER_LIST_INFO);
		const var_4 I_SORT_SIZE = sizeof(OUTPUT_SORT_INFO); 
		const var_4 I_SRC_SIZE = sizeof(OUTPUT_SOURCE_INFO);
		var_1 *ret_std_info = NULL, **arr_lid_info = NULL;
		XML_INFO* pst_fxi = pst_sid_lst, *pst_sxi_end = pst_sid_lst + in_sid_max;
		XML_INFO* pst_bxi = pst_tid_lst, *pst_txi_end = pst_tid_lst + in_tid_max;
		out_tid_num = 0;
		out_sid_fid = 0;
		var_1 *ptr_out = std_con_buf, *ptr_out_end = std_con_buf + in_sid_buf_size;
		var_1 *ptr_tn_out = tid_con_buf, *ptr_tn_out_end = tid_con_buf + in_tid_buf_size;
		var_u8* p_tmp_id_lst = in_id_lst;
		const var_4 I_NAME_SIZE = 100;
		char str_tmp[I_NAME_SIZE];
		UC_MD5 cMd5;
		VinfoLib* cp_lib = (VinfoLib*)vp_info;
		try
		{
			for(t = 0; t < id_grp;t++)
			{
				var_u8* p_id_lst = p_tmp_id_lst;
				p_tmp_id_lst += in_id_num[t];
				var_1  *ptr_aoc = tmp_use_buf, *ptr_aoc_end = tmp_use_buf + in_tmp_buf_size;
				memset(ast_cluster, 0, sizeof(CLUSTER_INFO) * I_PLAYER_MAX);
				ret = cp_lib->get_standard_info(p_id_lst[0], &st_std, ret_std_info);
				if(ret <= 0)
					throw -3;
				if(ptr_aoc + in_id_num[t] * sizeof(var_1*) > ptr_aoc_end)
					throw -4;
				arr_lid_info = (var_1**)ptr_aoc;
				memset(arr_lid_info, 0, in_id_num[t] * sizeof(var_1*));
				ptr_aoc += in_id_num[t] * sizeof(var_1*);
				for(i = 1; i < in_id_num[t]; i++)
				{
					if(ptr_aoc + I_SRC_SIZE > ptr_aoc_end)
						throw -4;
					OUTPUT_SOURCE_INFO *pst_si = (OUTPUT_SOURCE_INFO*)ptr_aoc;
					ptr_aoc += I_SRC_SIZE;
					if(ptr_aoc + I_SORT_SIZE > ptr_aoc_end)
						throw -5;
					OUTPUT_SORT_INFO *past_sort = (OUTPUT_SORT_INFO*)ptr_aoc;
					num = cp_lib->get_source_info(p_id_lst[i], pst_si, (ptr_aoc_end - ptr_aoc) / I_SORT_SIZE, past_sort, arr_lid_info[i - 1]);
					if(num <= 0)
						throw -6;
					ptr_aoc += num * I_SORT_SIZE;
					for(var_4 j = 0; j < num;j++)
					{
						if(ptr_aoc + I_CLI_SIZE > ptr_aoc_end)
							throw -7;
						if(past_sort[j].def < 0 || past_sort[j].def > I_DEF_MAX)
							throw -8;
						if(past_sort[j].player < 0 || past_sort[j].player > I_PLAYER_MAX)
							throw -9;
						CLUSTER_LIST_INFO *pst_cli = (CLUSTER_LIST_INFO*)ptr_aoc;
						ptr_aoc += I_CLI_SIZE;
						CLUSTER_INFO::TAG_INFO *pst_tci = ast_cluster[past_sort[j].player].ast_tag + past_sort[j].def;
						ast_cluster[past_sort[j].player].ptr_pn = past_sort[j].ptr_par;
						ast_cluster[past_sort[j].player].pn_len = past_sort[j].par_len;
						pst_cli->pst_si = pst_si;
						pst_cli->pst_di = &past_sort[j].st_di; 
						pst_cli->pst_next = pst_tci->pst_head;
						pst_tci->def_num++;
						pst_tci->ptr_def = past_sort[j].ptr_def;
						pst_tci->def_len = past_sort[j].def_len;
						if(pst_tci->pst_head == NULL)
							ast_cluster[past_sort[j].player].pl_num++;
						pst_tci->pst_head = pst_cli;
					}
				}
				pst_fxi->pos = ptr_out;
				ptr_out += 4;
				len = _snprintf(ptr_out, ptr_out_end - ptr_out, "<video>\n\
																<rel_id><![CDATA["CP_PU64"]]></rel_id>\n",p_id_lst[0]);
				if(len <= 0)
					throw -10;
				ptr_out += len;
				var_u8 tn_id = 0;
				for(i = 0; i < I_PLAYER_MAX; i++)
				{
					if(ast_cluster[i].pl_num <= 0)
						continue;
					len = _snprintf(ptr_out, ptr_out_end - ptr_out, "  <play_src>\n\
																	<player><![CDATA[%.*s]]></player>\n\
																	<def_num><![CDATA[%d]]></def_num>\n",
																	ast_cluster[i].pn_len, ast_cluster[i].ptr_pn, 
																	ast_cluster[i].pl_num);
					if(len <= 0)
						throw -11;
					ptr_out += len;
					for(var_4 j = 0; j < I_DEF_MAX;j++)
					{
						CLUSTER_INFO::TAG_INFO *pst_tci = ast_cluster[i].ast_tag + j;
						if(pst_tci->def_num <= 0)
							continue;
						len = _snprintf(str_tmp, I_NAME_SIZE, CP_PU64"#%d#%d#",p_id_lst[0], i, j);
						if(len <= 0)
							throw -12;
						tn_id = cMd5.MD5Bits64((var_u1*)str_tmp, len);
						len = _snprintf(ptr_out, ptr_out_end - ptr_out, "    <def%d>\n\
																		<tn><![CDATA[%.*s]]></tn>\n\
																		<tn_id><![CDATA["CP_PU64"]]></tn_id>\n\
																		</def%d>\n",
																		i, 
																		pst_tci->def_len, pst_tci->ptr_def,
																		tn_id, i);
						if(len <= 0)
							throw -13;
						ptr_out += len;
						if(ptr_tn_out + 4 > ptr_tn_out_end)
							throw -14;
						pst_bxi->pos = ptr_tn_out;
						ptr_tn_out += 4;
						len = _snprintf(ptr_tn_out, ptr_tn_out_end - ptr_tn_out, "<title><![CDATA[%.*s]]></title>\n\
																				 <alias><![CDATA[%.*s]]></alias>\n\
																				 <eng><![CDATA[%.*s]]></eng>\n\
																				 <tn_id><![CDATA["CP_PU64"]]></tn_id>\n\
																				 <tn><![CDATA[%.*s]]></tn>\n\
																				 <player><![CDATA[%.*s]]></player>\n\
																				 <showtime><![CDATA[%d]]></showtime>\n\
																				 <region><![CDATA[%.*s]]></region>\n\
																				 <src_num><![CDATA[%d]]></src_num>\n",
																				 st_std.title_len, st_std.ptr_title,
																				 st_std.alias_len, st_std.ptr_alias,
																				 st_std.eng_len, st_std.ptr_eng,
																				 tn_id,
																				 pst_tci->def_len, pst_tci->ptr_def,
																				 ast_cluster[i].pn_len, ast_cluster[i].ptr_pn,
																				 st_std.show_time,
																				 st_std.region_len, st_std.ptr_region,
																				 pst_tci->def_num);
						if(len <= 0)
							throw -15;
						ptr_tn_out += len;
						CLUSTER_LIST_INFO *pst_cli = pst_tci->pst_head;
						var_4 count = 0;
						while(pst_cli)
						{
							len = _snprintf(ptr_tn_out, ptr_tn_out_end - ptr_tn_out, "<src%d>\n\
																					 <srcsite><![CDATA[%.*s]]></srcsite>\n\
																					 <plnk><![CDATA[%.*s]]></plnk>\n\
																					 <tag><![CDATA[%.*s]]></tag>\n\
																					 <def><![CDATA[%.*s]]></def>\n\
																					 <lastmod><![CDATA[%u]]></lastmod>\n\
																					 </src%d>\n",
																					 count,
																					 pst_cli->pst_si->psite_len, pst_cli->pst_si->ptr_site,
																					 pst_cli->pst_di->plnk_len, pst_cli->pst_di->ptr_plnk,
																					 pst_cli->pst_di->tag_len, pst_cli->pst_di->ptr_tag,
																					 pst_cli->pst_di->def_len, pst_cli->pst_di->ptr_def, 
																					 pst_cli->pst_si->last_mod,
																					 count);
							if(len <= 0)
								throw -16;
							ptr_tn_out += len;
							count++;
							pst_cli = pst_cli->pst_next;
						}
						if(pst_bxi + 1 > pst_txi_end)
							throw -17;
						pst_bxi->len = ptr_tn_out - pst_bxi->pos;
						pst_bxi->id = tn_id;
						*(var_4*)pst_bxi->pos = pst_bxi->len;
						pst_bxi->md5 = cMd5.MD5Bits64((var_u1*)(pst_bxi->pos + 4), pst_bxi->len - 4); 
#ifdef  DEBUG_PRINT
						printf("##### TID["CP_PU64"] FID["CP_PU64"] 以下为输出数据 #####\n%.*s\n\n", pst_bxi->id, pst_bxi->md5,pst_bxi->len - 4, pst_bxi->pos + 4);
#endif
						pst_bxi++; 
					}
					len = _snprintf(ptr_out, ptr_out_end - ptr_out, "  </play_src>\n");
					if(len <= 0)
						throw -18;
					ptr_out += len;
				}
				len = _snprintf(ptr_out, ptr_out_end - ptr_out, "</video>\n");
				if(len <= 0)
					throw -19;
				ptr_out += len;
				if(pst_fxi + 1 > pst_sxi_end)
				{
					throw -20;
				}
				pst_fxi->id = p_id_lst[0];
				pst_fxi->len = ptr_out - pst_fxi->pos;
				*(var_4*)pst_fxi->pos = pst_fxi->len;
				pst_fxi->md5 = cMd5.MD5Bits64((var_u1*)(pst_fxi->pos + 4), pst_fxi->len - 4); 
#ifdef  DEBUG_PRINT
				printf("##### SID["CP_PU64"] FID["CP_PU64"] 以下为输出数据 #####\n%.*s\n\n", pst_fxi->id, pst_fxi->md5, pst_fxi->len - 4, pst_fxi->pos + 4);
#endif
				pst_fxi++;
				for(i = 1; i < in_id_num[t];i++)
				{
					if(arr_lid_info[i - 1])
					{
						cp_lib->free_info(arr_lid_info[i - 1]);
						arr_lid_info[i - 1] = NULL;
					}
				}
				arr_lid_info = NULL;
				cp_lib->free_info(ret_std_info);
				ret_std_info = NULL;
			}
			out_tid_num = pst_bxi - pst_tid_lst;
			out_sid_fid = pst_fxi - pst_sid_lst;
			ret = 0;
		} 
		catch (var_4 err)
		{
			ret = err;
		}
		if(ret_std_info)
		{
			cp_lib->free_info(ret_std_info);
			ret_std_info = NULL;
		}
		if(arr_lid_info)
		{
			for(i = 1; i < in_id_num[t];i++)
			{
				if(arr_lid_info[i - 1])
				{
					cp_lib->free_info(arr_lid_info[i - 1]);
					arr_lid_info[i - 1] = NULL;
				}
			}
			arr_lid_info = NULL;
		}
		return ret;
	}
	
private:
	UT_HashTable_Lite<var_u8> m_hs_nid;
	UT_HashTable_Lite<var_u8> m_hs_sid;
	UT_HashTable_Lite<var_u8> m_hs_lid;

	CP_MUTEXLOCK    m_user_lck;
	CP_MUTEXLOCK_RW m_global_lck;

	UC_Mem_Allocator_Recycle m_lib_INODE;
	UC_Mem_Allocator_Recycle m_lib_CNODE;
	UC_Mem_Allocator_Recycle m_lib_UNODE;
};
