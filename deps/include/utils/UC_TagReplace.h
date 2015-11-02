//
//  UC_TagReplace.h
//  code_library
//
//  Created by zhanghl on 12-12-24.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_TAGREPLACE_H_
#define _UC_TAGREPLACE_H_

#include "UH_Define.h"
#include "UT_HashSearch.h"
#include "UC_MD5.h"
#include "UT_Sort.h"

class UC_TagReplace
{
public:
    var_4 init_tag_replace()
    {
        m_hs_work = new UT_HashSearch<var_u8>;
        if(m_hs_work == NULL)
            return -1;
        if(m_hs_work->InitHashSearch(10240, -1))
            return -1;
        
        m_hs_idle = new UT_HashSearch<var_u8>;
        if(m_hs_idle == NULL)
            return -1;
        if(m_hs_idle->InitHashSearch(10240, -1))
            return -1;
        
        m_hs_test = new UT_HashSearch<var_u8>;
        if(m_hs_test == NULL)
            return -1;
        if(m_hs_test->InitHashSearch(10240, -1))
            return -1;

        return 0;
    }
    
    var_4 trim_table(var_1* table_buf, var_4 table_len, var_1* new_table_buf, var_4 new_table_len, var_4& ret_table_len)
    {
        var_u8* md5_key = new(std::nothrow) var_u8[1<<20];
        var_1** lst_key = new(std::nothrow) var_1*[1<<20];
        var_1** lst_val = new(std::nothrow) var_1*[1<<20];
        var_4*  lst_key_len = new(std::nothrow) var_4[1<<20];
        var_4*  lst_val_len = new(std::nothrow) var_4[1<<20];
        var_4   all_num = 0;
        
        var_1* beg = table_buf;
        var_1* end = table_buf + table_len;
        
        while(beg < end)
        {
            var_1* mid = beg;
            
            while(mid < end && (*mid == ' ' || *mid == '\t' || *mid == '\r'))
                mid++;
            if(mid >= end)
                break;
            
            while(mid < end && *mid != '\n' && *mid != '=')
                mid++;
            if(mid >= end || *mid == '\n')
                return -1;
            
            var_1* pos_b = NULL;
            var_1* pos_e = NULL;
            
            if(cp_skip_useless_char(beg, (var_4)(mid - beg), pos_b, pos_e) )
                return -1;
            pos_e++;
            
            lst_key[all_num] = pos_b;
            lst_key_len[all_num] = (var_4)(pos_e - pos_b);
            md5_key[all_num] = m_md5.MD5Bits64((var_u1*)pos_b, (var_4)(pos_e - pos_b));
            
            beg = ++mid;
            
            while(mid < end && *mid != '\n')
                mid++;
            
            if(beg == mid)
                return -1;
            
            if(cp_skip_useless_char(beg, (var_4)(mid - beg), pos_b, pos_e))
                return -1;
            pos_e++;
            
            lst_val[all_num] = pos_b;
            lst_val_len[all_num] = (var_4)(pos_e - pos_b);
            
            beg = ++mid;
            
            all_num++;
        }
        
        qs_recursion_1k_4p<var_u8, var_1*, var_1*, var_4, var_4>(0, all_num - 1, md5_key, lst_key, lst_val, lst_key_len, lst_val_len);
                
        var_1* ptr = new_table_buf;
        
        var_4 pos = 0;
        for(var_4 i = 0; i < all_num; )
        {
            while(i < all_num && md5_key[pos] == md5_key[i])
                i++;
            
            memcpy(ptr, lst_key[pos], lst_key_len[pos]);
            ptr += lst_key_len[pos];
            *ptr++ = '=';
            
            for(var_4 j = pos; j < i; j++)
            {
                memcpy(ptr, lst_val[j], lst_val_len[j]);
                ptr += lst_val_len[j];
                *ptr++ = ';';
            }
            *(ptr - 1) = '\n';
            
            pos = i;
        }
        
        ret_table_len = (var_4)(ptr - new_table_buf);
        
        delete md5_key;
        delete lst_key;
        delete lst_val;
        delete lst_key_len;
        delete lst_val_len;

        return 0;
    }

    var_4 update_table(var_1* table_buf, var_4 table_len, var_4 test_flg)
    {
        var_1* new_table_buf = new(std::nothrow) var_1[5<<20];
        var_4  new_table_len = 5<<20;
        var_4  ret_table_len = 0;
        
        trim_table(table_buf, table_len, new_table_buf, new_table_len, ret_table_len);
        
        m_hs_idle->ClearHashSearch();
        
        var_1* beg = new_table_buf;
        var_1* end = new_table_buf + ret_table_len;
        
        while(beg < end)
        {
            var_1* mid = beg;
            
            while(mid < end && (*mid == ' ' || *mid == '\t' || *mid == '\r'))
                mid++;
            if(mid >= end)
                break;
            
            while(mid < end && *mid != '\n' && *mid != '=')
                mid++;
            if(mid >= end || *mid == '\n')
                return -1;
            
            var_1* pos_b = NULL;
            var_1* pos_e = NULL;
            
            if(cp_skip_useless_char(beg, (var_4)(mid - beg), pos_b, pos_e) )
                return -1;
            pos_e++;
            
            var_u8 key = m_md5.MD5Bits64((var_u1*)pos_b, (var_4)(pos_e - pos_b));
            
            beg = ++mid;
            
            while(mid < end && *mid != '\n')
                mid++;

            if(beg == mid)
                return -1;
            
            if(cp_skip_useless_char(beg, (var_4)(mid - beg), pos_b, pos_e))
                return -1;
            pos_e++;

            var_vd* ret_buf = NULL;
            var_4   ret_len = 0;
            
            if(m_hs_idle->AddKey_VL(key, pos_b, (var_4)(pos_e - pos_b), &ret_buf, &ret_len))
                return -1;
            
            beg = ++mid;
        }
        
        m_hs_lock.lock_w();
        
        if(test_flg == 1)
        {
			m_hs_temp = m_hs_test;
            m_hs_test = m_hs_idle;
            m_hs_idle = m_hs_temp;
        }
        else
        {
			m_hs_temp = m_hs_work;
            m_hs_work = m_hs_idle;
            m_hs_idle = m_hs_work;
        }
        
        m_hs_lock.unlock();
        
        return 0;
    }
    
    var_4 replace_tag(var_1* key_tag, var_4 key_len, var_1* val_tag, var_4 val_len, var_4& ret_len, var_4 replace_flg, var_4 test_flg)
    {
        var_u8 key = m_md5.MD5Bits64((var_u1*)key_tag, (var_4)key_len);
        
        m_hs_lock.lock_r();
        
        UT_HashSearch<var_u8>* hs = NULL;
        
        if(test_flg == 1)
            hs = m_hs_test;
        else
            hs = m_hs_work;
        
        var_vd* buf = NULL;
        var_4   len = 0;

        if(hs->SearchKey_VL(key, &buf, &len))
        {
            m_hs_lock.unlock();

            return -1;
        }
        
        var_1* beg = (var_1*)buf;
        var_1* end = (var_1*)buf + len - 1;
        
        if(replace_flg == 1)
        {
            while(beg <= end && *end != ';')
                end--;
            if(beg >= end)
            {
                beg = (var_1*)buf;
                end = (var_1*)buf + len;
            }
			else
			{
				beg = ++end;
				end = (var_1*)buf + len;
			}
        }
		else
			end++;
        
        if(end - beg > val_len)
        {
            m_hs_lock.unlock();
			
            return -1;
        }
        
        ret_len = (var_4)(end - beg);
        memcpy(val_tag, beg, ret_len);
        
        m_hs_lock.unlock();
        
        return 0;
    }
    
    
public:
    UT_HashSearch<var_u8>* m_hs_work;
    UT_HashSearch<var_u8>* m_hs_idle;
    UT_HashSearch<var_u8>* m_hs_test;
    UT_HashSearch<var_u8>* m_hs_temp;
    CP_MUTEXLOCK_RW        m_hs_lock;
    UC_MD5                 m_md5;
};

#endif // _UC_TAGREPLACE_H_
