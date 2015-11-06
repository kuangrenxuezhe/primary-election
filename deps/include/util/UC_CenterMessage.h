//
//  UC_CenterMessage.h
//  code_library
//
//  Created by zhanghl on 14-9-10.
//  Copyright (c) 2014年 zhanghl. All rights reserved.
//

#ifndef _UC_CENTER_MESSAGE_H_
#define _UC_CENTER_MESSAGE_H_

#include "UH_Define.h"
#include "UT_Queue.h"
#include "UC_Mem_Allocator_Recycle.h"

class Interface_CenterMessage_Processor
{
public:
    virtual var_4 new_handle(var_vd*& handle) = 0;
    virtual var_4 del_handle(var_vd*& handle) = 0;
    
    virtual var_4  distribute_task(var_vd* handle, var_1* message_buf, var_4 message_len) = 0; // return 'all distribute count'
    virtual var_vd process_task(var_vd* handle, var_4 cur_count) = 0;
    virtual var_4  combine_task(var_vd* handle) = 0;
    virtual var_4  extract_result(var_vd* handle, var_1* result_buf, var_4 result_len, var_4& return_len) = 0;
};

typedef struct _stream_node_
{
    CP_MUTEXLOCK locker;
    
    var_4 all_count;
    var_4 fin_count;
    var_4 cur_count;

    var_vd* handle;
    
    var_4 refer_flg;
    var_4 refer_fin;
} STREAM_NODE;

class UC_CenterMessage
{
public:
    var_4 init(var_4 queue_len, var_4 working_num, Interface_CenterMessage_Processor* processor)
    {
        if(mq_waiting.InitQueue(queue_len))
            return -1;
        if(mq_finished.InitQueue(queue_len))
            return -1;
        
        if(m_stream_node.init(sizeof(STREAM_NODE)))
            return -1;
        
        m_processor = processor;
        
        for(var_4 i = 0; i < working_num; i++)
        {
            if(cp_create_thread(thread_process, this))
                return -1;
        }
        
        return 0;
    }
    
    var_4 put_message(var_1* message_buf, var_4 message_len, var_vd** reference = NULL)
    {
        var_vd* handle = NULL;
        
        if(m_processor->new_handle(handle))
            return -1;
        
        var_4 all_count = m_processor->distribute_task(handle, message_buf, message_len);
        if(all_count <= 0)
        {
            m_processor->del_handle(handle);
            return -1;
        }
        
        STREAM_NODE* node = (STREAM_NODE*)m_stream_node.get_mem();
        node->cur_count = 0;
        node->fin_count = 0;
        node->all_count = all_count;
        
        node->handle = handle;
        
        if(reference)
        {
            *reference = (var_vd*)node;
            node->refer_fin = 0;
            node->refer_flg = 1;
        }
        else
            node->refer_flg = 0;
        
        for(var_4 i = 0; i < all_count; i++)
            mq_waiting.PushData(node);
        
        return 0;
    }
    
    var_4 get_message(var_1* result_buf, var_4 result_len, var_4& return_len, var_vd** reference = NULL)
    {
        assert(*reference != (var_vd*)1);
        
        STREAM_NODE* node = NULL;
        
        if(reference)
        {
            node = (STREAM_NODE*)*reference;
            
            while(node->refer_fin == 0)
                cp_sleep(1);
        }
        else
            node = mq_finished.PopData();
        
        if(m_processor->extract_result(node->handle, result_buf, result_len, return_len))
        {
            if(reference == NULL)
                mq_finished.PushData(node);

            return -1;
        }
        
        m_processor->del_handle(node->handle);
        
        m_stream_node.put_mem((var_vd*)node);
        
        if(reference)
            *reference = (var_vd*)1;

        return 0;
    }
    
    static CP_THREAD_T thread_process(var_vd* argv)
    {
        UC_CenterMessage* cc = (UC_CenterMessage*)argv;
        
        for(;;)
        {
            STREAM_NODE* node = cc->mq_waiting.PopData();
            
            var_4 cur_count = __sync_fetch_and_add(&node->cur_count, 1);
            
            cc->m_processor->process_task(node->handle, cur_count);
            
            var_4 finish_flg = 0;
            
            node->locker.lock();
            node->fin_count++;
            if(node->fin_count == node->all_count)
                finish_flg = 1;
            node->locker.unlock();
            
            if(finish_flg)
            {
                cc->m_processor->combine_task(node->handle);
                
                if(node->refer_flg)
                    node->refer_fin = 1;
                else
                    cc->mq_finished.PushData(node);
            }
        }
        
        return NULL;
    }
    
public:
    UT_Queue<STREAM_NODE*> mq_waiting;
    UT_Queue<STREAM_NODE*> mq_finished;
    
    UC_Mem_Allocator_Recycle m_stream_node;
    
    Interface_CenterMessage_Processor* m_processor;
};

#endif // _UC_CENTER_MESSAGE_H_
