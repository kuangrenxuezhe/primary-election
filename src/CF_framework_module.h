//
//  CF_framework_module.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_MODULE_H_
#define _CF_FRAMEWORK_MODULE_H_

#include "utils/UH_Define.h"
#include "utils/UC_Mem_Allocator_Recycle.h"

#include "CF_framework_config.h"
#include "CF_framework_interface.h"

class CF_framework_module
{
public:
    var_4 init_framework(CF_framework_interface* module)
    {
        if(m_cfg.init())
            return -1;
        
        if(cp_init_socket())
            return -1;
        if(cp_listen_socket(m_listen, m_cfg.md_listen_port))
            return -1;
       
        m_module = module;

        m_module_type = m_module->module_type();
        
        m_is_train = m_module->is_update_train();
        m_is_persistent = m_module->is_persistent_library();

        if(m_module->init_module((var_vd*)&m_cfg))
            return -1;
        
        if(m_mem_message.init(m_cfg.md_message_size))
            return -1;
        
        if(cp_create_thread(thread_work, this, m_cfg.md_work_thread_num))
            return -1;
        
        if(m_is_persistent && cp_create_thread(thread_persistent, this))
            return -1;
        
        if(m_is_train && cp_create_thread(thread_train, this))
            return -1;
        
        return 0;
    }
    
    var_4 recv_request(CP_SOCKET_T& client, var_1* recv_buf, var_4 recv_max, var_4& recv_len)
    {
        if(cp_accept_socket(m_listen, client))
            return -1;
        
        cp_set_overtime(client, 5000);
        
        try {
            if(cp_recvbuf(client, (var_1*)&recv_len, 4))
                throw -2;
            
            if(recv_len > recv_max)
                throw -3;
            
            if(cp_recvbuf(client, recv_buf, recv_len))
                throw -4;
        }
        catch (var_4 error_code)
        {
            cp_close_socket(client);
            client = 0;
            
            return -1;
        }
        
        return 0;
    }
    
    var_4 send_request(CP_SOCKET_T& client, var_1* send_buf, var_4 send_len)
    {
        var_4 retval = cp_sendbuf(client, send_buf, send_len);
        
        cp_close_socket(client);
        client = 0;
        
        return retval;
    }
    
    var_4 update_click(var_1* recv_buf, var_1* send_buf, var_4 send_max, var_4& send_len)
    {
        send_len = 8;
        *(var_4*)send_buf = 4;
        *(var_4*)(send_buf + 4) = m_module->update_click(recv_buf);

        return 0;
    }
    
    var_4 update_document(var_1* recv_buf, var_1* send_buf, var_4 send_max, var_4& send_len)
    {
        send_len = 8;
        *(var_4*)send_buf = 4;
        *(var_4*)(send_buf + 4) = m_module->update_item(recv_buf);
        
        return 0;
    }
    
    var_4 update_circle(var_1* recv_buf, var_1* send_buf, var_4 send_max, var_4& send_len)
    {
        send_len = 8;
        *(var_4*)send_buf = 4;
        *(var_4*)(send_buf + 4) = m_module->update_user(recv_buf);

        return 0;
    }
    
    var_4 query(var_1* recv_buf, var_1* send_buf, var_4 send_max, var_4& send_len)
    {
        var_1* pos = recv_buf + 4;
        
        if(m_module_type == 1)
        {
            *(var_4*)send_buf = 0;
            send_len = 4;

            var_4 type = *(var_4*)pos;
            pos += 4;
            var_u8 uid = *(var_u8*)pos;
            pos += 8;
            
            if(type == 1)
            {
                if(m_module->query_recommend(uid, *(var_4*)pos, send_buf + 4, send_max - 4, send_len))
                    return -1;
            }
            else if(type == 2)
            {
                if(m_module->query_history(uid, send_buf + 4, send_max - 4, send_len))
                    return -1;
            }
            else if(type == 3)
            {
                if(m_module->query_user(uid, send_buf + 4, send_max - 4, send_len))
                    return -1;
            }
            else
                assert(0);
            
            *(var_4*)send_buf = send_len;
            send_len += 4;
        }
        else if(m_module_type == 2)
        {
            var_4 type = *(var_4*)pos;
            pos += 4;
            var_u8 uid = *(var_u8*)pos;
            pos += 8;
            
            if(type == 1)
            {
                var_4 recommend_num = *(var_4*)pos;
                pos += 4;
                var_u8* recommend_lst = (var_u8*)pos;
                pos += recommend_num << 3;
                var_4 history_num = *(var_4*)pos;
                pos += 4;
                var_u8* history_list = (var_u8*)pos;
                pos += history_num << 3;
                
                send_len = recommend_num * 4 + 8;
                if(send_max < send_len)
                {
                    *(var_4*)send_buf = 0;
                    send_len = 4;
                    
                    return -1;
                }
                
                var_f4* recommend_pwr = (var_f4*)(send_buf + 8);
                
                if(m_module->query_algorithm(uid, recommend_num, recommend_lst, recommend_pwr, history_num, history_list))
                {
                    *(var_4*)send_buf = 0;
                    send_len = 4;
                    
                    return -1;
                }
                
                *(var_4*)send_buf = recommend_num * 4 + 4;
                *(var_4*)(send_buf + 4) = recommend_num;
            }
            else if(type == 2)
            {
                var_4  class_num = *(var_4*)pos;
                var_4* class_info = (var_4*)(send_buf + 8);
                
                if(m_module->query_userclass(uid, class_num, class_info))
                {
                    *(var_4*)send_buf = 0;
                    send_len = 4;
                    
                    return -1;
                }
                
                send_len = class_num * 4 + 8;
                
                *(var_4*)send_buf = class_num * 4 + 4;
                *(var_4*)(send_buf + 4) = class_num;
            }
            else
                assert(0);
        }
        else
            assert(0);
        
        return 0;
    }
    
    var_4 update_push(var_1* recv_buf, var_1* send_buf, var_4 send_max, var_4& send_len)
    {
        send_len = 8;
        *(var_4*)send_buf = 4;
        
        var_1* pos = recv_buf + 4;
        
        var_u8 uid = *(var_u8*)pos;
        pos += 8;
        var_4 push_num = *(var_4*)pos;
        pos += 4;
        var_u8* push_lst = (var_u8*)pos;
        pos += push_num << 3;

		*(var_4*)(send_buf + 4) = m_module->update_pushData(uid, push_num, push_lst);

        return 0;
    }
    
    static CP_THREAD_T thread_work(var_vd* argv)
    {
        CF_framework_module* cc = (CF_framework_module*)argv;
        
        CP_SOCKET_T client;

        var_4  recv_len = 0;
        var_1* recv_buf = (var_1*)cc->m_mem_message.get_mem();
        assert(recv_buf);
        
        var_4  send_len = 0;
        var_1* send_buf = (var_1*)cc->m_mem_message.get_mem();
        assert(send_buf);

        for(;;)
        {
            if(cc->recv_request(client, recv_buf, cc->m_cfg.md_message_size, recv_len))
                continue;

            var_4 request_type = *(var_4*)recv_buf;
            
            switch(request_type)
            {
                case 1: // update_click
                    cc->update_click(recv_buf, send_buf, cc->m_cfg.md_message_size, send_len);
                    break;
                    
                case 2: // update_document
                    cc->update_document(recv_buf, send_buf, cc->m_cfg.md_message_size, send_len);
                    break;
                    
                case 3: // update_circle
                    cc->update_circle(recv_buf, send_buf, cc->m_cfg.md_message_size, send_len);
                    break;
                    
                case 4: // query
                    cc->query(recv_buf, send_buf, cc->m_cfg.md_message_size, send_len);
                    break;
                
                case 666: // update push
                    cc->update_push(recv_buf, send_buf, cc->m_cfg.md_message_size, send_len);
                    break;
                    
                default:
                    assert(0);
                    break;
            }
            
            cc->send_request(client, send_buf, send_len);
        }
        
        cc->m_mem_message.put_mem((var_vd*)send_buf);
        cc->m_mem_message.put_mem((var_vd*)recv_buf);
        
        return NULL;
    }

    static CP_THREAD_T thread_persistent(var_vd* argv)
    {
        CF_framework_module* cc = (CF_framework_module*)argv;

        var_u8 save_time_stamp;
        
        FILE* fp = fopen(cc->m_cfg.md_persistent_flag, "r");
        if(fp)
        {
            var_1 time_stamp[64];
            fgets(time_stamp, 64, fp);
            fclose(fp);
            
            save_time_stamp = cp_strtoval_u64(time_stamp);
        }
        else
            save_time_stamp = 0;
        
        for(;; cp_sleep(60000))
        {
            time_t now = time(NULL);
            
            if(now - save_time_stamp < cc->m_cfg.md_persistent_interval * 60)
                continue;
            
            if(cc->m_module->persistent_library())
                continue;
            
            FILE* fp = fopen(cc->m_cfg.md_persistent_flag, "w");
            if(fp)
            {
                fprintf(fp, CP_PU64, now);
                fclose(fp);
            }
            
            save_time_stamp = now;
        }
        
        return NULL;
    }
    
    static CP_THREAD_T thread_train(var_vd* argv)
    {
        CF_framework_module* cc = (CF_framework_module*)argv;
        
        var_u8 old_time_stamp;
        var_u8 new_time_stamp;
        var_u8 tmp_time_stamp;
        
        FILE* fp = fopen(cc->m_cfg.md_train_flag, "r");
        if(fp)
        {
            var_1 time_stamp[64];
            fgets(time_stamp, 64, fp);
            fclose(fp);
            
            old_time_stamp = cp_strtoval_u64(time_stamp);
        }
        else
            old_time_stamp = 0;
        
        for(;; cp_sleep(600000))
        {
            var_vd* handle = NULL;
            
            if(cp_dir_open(handle, cc->m_cfg.md_train_path))
                continue;
            
            var_1 folder[256];
            
            new_time_stamp = old_time_stamp;
            
            while(cp_dir_travel_folder(handle, folder) == 0)
            {
                tmp_time_stamp = cp_strtoval_u64(folder);
                
                if(tmp_time_stamp > new_time_stamp)
                    new_time_stamp = tmp_time_stamp;
            }
            
            cp_dir_close(handle);
            
            if(new_time_stamp <= old_time_stamp)
                continue;
            
            var_1 update_path[256];
            sprintf(update_path, "%s/" CP_PU64, cc->m_cfg.md_train_path, new_time_stamp);
            
            if(cc->m_module->update_train(update_path))
                continue;
            
            FILE* fp = fopen(cc->m_cfg.md_train_flag, "w");
            if(fp)
            {
                fprintf(fp, CP_PU64, new_time_stamp);
                fclose(fp);
            }

            old_time_stamp = new_time_stamp;
        }
        
        return NULL;
    }

public:
    CP_SOCKET_T m_listen;
    
    MODULE_CONFIG m_cfg;

    UC_Mem_Allocator_Recycle m_mem_message;
    
    CF_framework_interface* m_module;
    
    var_4 m_module_type;
    var_4 m_is_train;
    var_4 m_is_persistent;
};

#endif // _CF_FRAMEWORK_MODULE_H_
