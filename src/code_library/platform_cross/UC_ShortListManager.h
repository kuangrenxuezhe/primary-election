//
//  UC_ShortListManager.h
//  code_library
//
//  Created by zhanghl on 14-9-16.
//  Copyright (c) 2014年 zhanghl. All rights reserved.
//

#ifndef _UC_SHORTLISTMANAGER_H_
#define _UC_SHORTLISTMANAGER_H_

struct Interface_SLM_Data
{
public:
    virtual bool operator==(const Interface_SLM_Data& tmp) = 0;
    virtual Interface_SLM_Data* clone() = 0;
    virtual var_vd destroy() = 0;
    
public:
    Interface_SLM_Data* next;
};

typedef struct _SLM_List_
{
public:
    var_4 get_num()
    {
        return len;
    }
    
    Interface_SLM_Data* travel()
    {
        if(flg == 0)
        {
            pos = lst;
            flg = 1;
        }
        
        if(pos == NULL)
        {
            flg = 0;
            return NULL;
        }
        
        Interface_SLM_Data* tmp = pos;
        pos = pos->next;
        
        return tmp;
    }
    
public:
    Interface_SLM_Data* lst;
    var_4               len;
    var_4               use;
    
    Interface_SLM_Data* pos;
    var_4               flg;
    
    _SLM_List_()
    {
        lst = NULL;
        len = 0;
        use = 0;
        
        pos = NULL;
        flg = 0;
    }
} SLM_LIST;

class UC_ShortListManager
{
public:
    var_4 init(var_4 max_list_size)
    {
        m_max_list_size = max_list_size;
        
        work = new SLM_LIST;
        if(work == NULL)
            return -1;
        
        idle = new SLM_LIST;
        if(work == NULL)
        {
            delete work;
            return -1;
        }
        
        return 0;
    }
    
    SLM_LIST* get_lst()
    {
        lck_query.lock();
        work->use++;
        SLM_LIST* tmp = work;
        lck_query.unlock();
        
        return tmp;
    }
    
    var_vd put_lst(SLM_LIST* lst)
    {
        lck_query.lock();
        assert(lst->use > 0);
        lst->use--;
        lck_query.unlock();
    }
    
    var_4 add(Interface_SLM_Data* data)
    {
        // lock for update
        lck_update.lock();
        
        // find same data
        Interface_SLM_Data* lst = work->lst;
        
        for(var_4 i = 0; i < work->len; i++)
        {
            if(lst == data)
            {
                lck_update.unlock();
                return 1;
            }
            
            lst = lst->next;
        }
        
        if(work->len >= m_max_list_size)
        {
            lck_update.unlock();
            return 2;
        }
        
        // wait idle free
        while(idle->use)
            cp_sleep(1);
        
        destroy_list(idle->lst, idle->len);
        
        // clone work list
        idle->lst = clone_list(work->lst, work->len, idle->len);
        idle->use = 0;
        
        // add new data to idle list
        Interface_SLM_Data* tmp = data->clone();
        assert(tmp);
        
        tmp->next = idle->lst;
        idle->lst = tmp;
        
        idle->len++;
        
        // change idle to work
        lck_query.lock();
        SLM_LIST* temp = work;
        work = idle;
        idle = temp;
        lck_query.unlock();
        
        // unlock for update
        lck_update.unlock();
        
        return 0;
    }
    
    var_4 del(Interface_SLM_Data* data)
    {
        // lock for update
        lck_update.lock();
        
        // find same data
        Interface_SLM_Data* lst = work->lst;
        
        Interface_SLM_Data* del_pos = NULL;
        
        for(var_4 i = 0; i < work->len; i++)
        {
            if(lst == data)
            {
                del_pos = lst;
                break;
            }
            
            lst = lst->next;
        }
        
        if(del_pos == NULL)
        {
            lck_update.unlock();
            return 1;
        }
        
        // wait idle free
        while(idle->use)
            cp_sleep(1);
        
        destroy_list(idle->lst, idle->len);
        
        // clone work list
        idle->lst = clone_list(work->lst, work->len, idle->len, del_pos);
        assert(work->len - 1 == idle->len);
        idle->use = 0;
        
        // change idle to work
        lck_query.lock();
        SLM_LIST* temp = work;
        work = idle;
        idle = temp;
        lck_query.unlock();
        
        // unlock for update
        lck_update.unlock();
        
        return 0;
    }
    
private:
    Interface_SLM_Data* clone_list(Interface_SLM_Data* org_lst, var_4 org_len, var_4& new_len, Interface_SLM_Data* del_pos = NULL)
    {
        Interface_SLM_Data* head = NULL;
        Interface_SLM_Data* tail = NULL;
        
        new_len = org_len;
        
        for(var_4 i = 0; i < org_len; i++)
        {
            if(org_lst == del_pos)
            {
                new_len--;
                
                Interface_SLM_Data* tmp = org_lst;
                org_lst = org_lst->next;
                
                tmp->destroy();
                continue;
            }
            
            Interface_SLM_Data* data = org_lst->clone();
            
            assert(data);
            data->next = NULL;
            
            org_lst = org_lst->next;
            
            if(head == NULL)
            {
                head = data;
                tail = data;
                
                continue;
            }
            
            tail->next = data;
            tail = data;
        }
        
        if(tail)
            tail->next = NULL;
        
        return 0;
    }
    
    var_vd destroy_list(Interface_SLM_Data* org_lst, var_4 org_len)
    {
        for(var_4 i = 0; i < org_len; i++)
        {
            Interface_SLM_Data* tmp = org_lst;
            org_lst = org_lst->next;
            
            tmp->destroy();
        }
    }
    
private:
    var_4 m_max_list_size;
    
    SLM_LIST* work;
    SLM_LIST* idle;
    
    CP_MUTEXLOCK lck_update;
    CP_MUTEXLOCK lck_query;
};

#endif // _UC_SHORTLISTMANAGER_H_
