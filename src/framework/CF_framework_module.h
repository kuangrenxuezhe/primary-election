//
//  CF_framework_module.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_MODULE_H_
#define _CF_FRAMEWORK_MODULE_H_

#include "util/UH_Define.h"
#include "util/UC_Mem_Allocator_Recycle.h"

#include "framework/CF_framework_config.h"
#include "framework/CF_framework_interface.h"

#include "proto/message.pb.h"

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
    
    var_4 query_interface(const TransferRequest& t_request, TransferRespond* t_respond)
    {
        var_4 ret_val = 0;
        var_4 sub_protocol = t_request.sub_protocol();
        
        // for candidate: candidate set, user status; for algoritm: algoritm, user category
        
        if(sub_protocol == 1)  // for candidate: candidate set
        {
            Recommend recommend;
            if(t_request.protocol().UnpackTo(&recommend) == false)
            {
                t_respond->set_respond_code(-1000);
                return -1;
            }
            
            CandidateSet candidate_set;
            if((ret_val = m_module->query_candidate_set(recommend, &candidate_set)))
            {
                t_respond->set_respond_code(ret_val);
                return -1;
            }
            
            t_respond->mutable_protocol()->PackFrom(candidate_set);
            
            return 0;
        }
        
        if(sub_protocol == 2) // for candidate: user status
        {
            User user;
            if(t_request.protocol().UnpackTo(&user) == false)
            {
                t_respond->set_respond_code(-1000);
                return -2;
            }
            
            UserStatus user_status;
            if((ret_val = m_module->query_user_status(user, &user_status)))
            {
                t_respond->set_respond_code(ret_val);
                return -2;
            }
            
            t_respond->mutable_protocol()->PackFrom(user_status);
            
            return 0;
        }
        
        if(sub_protocol == 3) // for algoritm: algoritm
        {
            CandidateSetBase candidate_set_base;
            if(t_request.protocol().UnpackTo(&candidate_set_base) == false)
            {
                t_respond->set_respond_code(-1000);
                return -3;
            }
            
            AlgorithmPower algorithm_power;
            if((ret_val = m_module->query_algorithm(candidate_set_base, &algorithm_power)))
            {
                t_respond->set_respond_code(ret_val);
                return -3;
            }
            
            t_respond->mutable_protocol()->PackFrom(algorithm_power);
            
            return 0;
        }

        if(sub_protocol == 4) // for algoritm: user category
        {
            Category category;
            if(t_request.protocol().UnpackTo(&category) == false)
            {
                t_respond->set_respond_code(-1000);
                return -4;
            }
            
            AlgorithmCategory algorithm_category;
            if((ret_val = m_module->query_user_category(category, &algorithm_category)))
            {
                t_respond->set_respond_code(ret_val);
                return -4;
            }
            
            t_respond->mutable_protocol()->PackFrom(algorithm_category);

            return 0;
        }

        t_respond->set_respond_code(-9000);
        
        return -9;
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
            {
                printf("thread_work - recv_request failure\n");
                continue;
            }
            
            TransferRequest t_request;
            if(t_request.ParseFromArray(recv_buf, recv_len) == false)
            {
                printf("thread_work - t_request.ParseFromArray failure\n");
                continue;
            }
            
            var_4 ret_val;
            
            TransferRespond t_respond;
            t_respond.set_respond_code(0);
            
            switch(t_request.main_protocol())
            {
                case 1: // action
                {
                    Action action;
                    if(t_request.protocol().UnpackTo(&action) == false)
                    {
                        t_respond.set_respond_code(-1000);
                        break;
                    }
                    
                    if((ret_val = cc->m_module->update_action(action)))
                    {
                        t_respond.set_respond_code(ret_val);
                        break;
                    }
                    
                    t_respond.mutable_protocol()->PackFrom(action);
                    
                    break;
                }
                case 2: // item
                {
                    Item item;
                    if(t_request.protocol().UnpackTo(&item) == false)
                    {
                        t_respond.set_respond_code(-1000);
                        break;
                    }
                    
                    if((ret_val = cc->m_module->update_item(item)))
                    {
                        t_respond.set_respond_code(ret_val);
                        break;
                    }
                    
                    break;
                }
                case 3: // subscribe
                {
                    Subscribe subscribe;
                    if(t_request.protocol().UnpackTo(&subscribe) == false)
                    {
                        t_respond.set_respond_code(-1000);
                        break;
                    }
                    
                    if((ret_val = cc->m_module->update_subscribe(subscribe)))
                    {
                        t_respond.set_respond_code(ret_val);
                        break;
                    }
                    
                    break;
                }
                case 4: // for candidate: candidate set, user status; for algoritm: algoritm, user category
                {
                    cc->query_interface(t_request, &t_respond);
                    break;
                }
                case 5: // feedback
                {
                    Feedback feedback;
                    if(t_request.protocol().UnpackTo(&feedback) == false)
                    {
                        t_respond.set_respond_code(-1000);
                        break;
                    }
                    
                    if((ret_val = cc->m_module->update_feedback(feedback)))
                    {
                        t_respond.set_respond_code(ret_val);
                        break;
                    }
                    
                    break;;
                }
                default:
                {
                    t_respond.set_respond_code(-9000);
                    break;
                }
            }
            
            var_4 size = t_respond.ByteSize();
            if(4 + size > cc->m_cfg.md_message_size || t_respond.SerializeToArray(send_buf + 4, cc->m_cfg.md_message_size) == false)
            {
                send_len = 4;
                *(var_4*)send_buf = -1;
            }
            else
            {
                send_len = 4 + size;
                *(var_4*)send_buf = size;
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

// action.ByteSize();
// action.SerializeToArray(<#void *data#>, <#int size#>)
