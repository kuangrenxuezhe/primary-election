//
//  CF_framework_module.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_MODULE_H_
#define _CF_FRAMEWORK_MODULE_H_

#include "../code_library/platform_cross/UH_Define.h"
#include "../code_library/platform_cross/UC_Mem_Allocator_Recycle.h"

#include "CF_framework_config.h"
#include "CF_framework_interface.h"
#include "CF_framework_jparse.h"

#include "CenterToModuleProtocol.pb.h"

#include "rdkafkacpp.h"

using namespace RdKafka;

class UC_Kafka
{
public:
    UC_Kafka()
    {
        m_partition = 0;
        
        m_cfg_kafka = NULL;
        m_cfg_topic = NULL;
        
        m_consumer = NULL;
        m_topic = NULL;
    }
    
    var_4 connect(const var_1* host, const var_u2 port, const var_1* topic, var_4 partition = 0, const var_1* save_offset = NULL, var_4 is_init = 0)
    {
        m_partition = partition;
        
        m_cfg_kafka = Conf::create(Conf::CONF_GLOBAL);
        m_cfg_topic = Conf::create(Conf::CONF_TOPIC);
        if(m_cfg_kafka == NULL || m_cfg_topic == NULL)
        {
            printf("create conf kafka or topic error\n");
            return -1;
        }

        std::string str_error;
        
        char brocker[32];
        sprintf(brocker,"%s:%d", host, port);
        
        std::string str_broker = brocker;
        
        if(m_cfg_kafka->set("metadata.broker.list", str_broker, str_error) != Conf::CONF_OK)
        {
            printf("set conf kafka error\n");
            return -1;
        }
        
        if(save_offset)
        {
            if(is_init)
                cp_remove_file((var_1*)save_offset);
            
            if(m_cfg_topic->set("auto.commit.interval.ms", "100", str_error) != Conf::CONF_OK)
            {
                printf("set topic conf auto.commit.interval.ms error\n");
                return -1;
            }
            
            if(m_cfg_topic->set("offset.store.path", save_offset, str_error) != Conf::CONF_OK)
            {
                printf("set topic offset.store.path error\n");
                return -1;
            }
            if(m_cfg_topic->set("auto.offset.reset", "smallest", str_error) != Conf::CONF_OK)
            {
                printf("set topic auto.offset.reset error\n");
                return -1;
            }
        }
        
        m_consumer = RdKafka::Consumer::create(m_cfg_kafka, str_error);
        if(m_consumer == NULL)
        {
            printf("create consumer error\n");
            return -1;
        }

        std::string str_topic = topic;
        
        m_topic = RdKafka::Topic::create(m_consumer, str_topic, m_cfg_topic, str_error);
        if(m_topic == NULL)
        {
            printf("create topic error\n");
            return -1;
        }

        if(m_consumer->start(m_topic, m_partition, Topic::OFFSET_STORED) != ERR_NO_ERROR)
        {
            printf("start consumer error\n");
            return -1;
        }
        
        return 0;
    }
    
    var_vd disconnect()
    {
        if(m_consumer != NULL)
        {
            if(m_topic != NULL)
            {
                m_consumer->stop(m_topic, m_partition);
                m_consumer->poll(1000);
                
                delete m_topic;
                m_topic = NULL;
            }
            
            delete m_consumer;
            m_consumer = NULL;
            
            wait_destroyed(5000);
        }
    }

    var_4 message(var_1* result, const var_8 max_size, var_8& ret_size)
    {
        if(m_consumer == NULL || m_topic == NULL)
        {
            printf("first call connect\n");
            return -1;
        }

        var_4 ret_val  = 0;
        
        RdKafka::Message* msg = m_consumer->consume(m_topic, m_partition, 10000);
        switch(msg->err())
        {
            case ERR__TIMED_OUT:
                printf("message - time out\n");
                ret_val = -100;
                break;
                
            case ERR__PARTITION_EOF:
                printf("message - partition eof\n");
                ret_val = -200;
                break;
                
            case ERR_NO_ERROR:
            {
                ret_size = msg->len(); // msg->offset()
                if(max_size < ret_size + 1)
                    ret_val = -300;
                else
                {
                    memcpy(result, msg->payload(), ret_size);
                    result[ret_size] = 0;
                }
                break;
            }
            default:
                printf("unknow error - %d, %s\n", msg->err(), msg->errstr().c_str());
                
                ret_val = -400;
                break;
        }
        
        delete msg;

        m_consumer->poll(0);
        
        return ret_val;
    }
    
    var_vd test()
    {
        UC_Kafka kfk;
        
        if(kfk.connect("103.7.221.141", 9099, "formaldehyde"))
            return;
        
        var_1 buf[102400];
        var_8 len = 0;
        var_8 max = 102400;
        
        kfk.message(buf, max, len);
    }
private:
    var_4 m_partition;
    
    Conf* m_cfg_kafka;
    Conf* m_cfg_topic;
    
    Consumer* m_consumer;
    Topic*    m_topic;
    
    var_1 m_save_offset[256];
};

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
        
        m_is_train = m_module->is_update_train();
        m_is_persistent = m_module->is_persistent_library();

        if(m_module->init_module((var_vd*)&m_cfg))
            return -1;

        if(m_kfk.connect(m_cfg.md_kafka_host, m_cfg.md_kafka_port, m_cfg.md_kafka_topic, 0, m_cfg.md_kafka_save))
        {
            printf("init_framework - kafka connect failure\n");
            return -1;
        }
        
        if(cp_create_thread(thread_kafka, this))
            return -1;
        
        if(cp_create_thread(thread_network, this, m_cfg.md_work_thread_num))
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
    
    static CP_THREAD_T thread_kafka(var_vd* argv)
    {
        CF_framework_module* cc = (CF_framework_module*)argv;
        
        var_8  get_len = 0;
        var_8  get_max = cc->m_cfg.md_message_size;
        var_1* get_buf = new var_1[get_max];
        assert(get_buf);

        CF_framework_jparse jparse;
        
        TransferRequest request;

        for(;;)
        {
            request.Clear();
            
            if(cc->m_kfk.message(get_buf, get_max, get_len))
            {
                printf("thread_kafka - kfk.message failure\n");
                continue;
            }
            
            if(jparse.parse_json(get_buf, &request))
            {
                printf("thread_kafka - jparse.parse_json failure\n%s\n", get_buf);
                continue;
            }
            
            switch(request.main_protocol())
            {
                case 1: // action
                {
                    Action action;
                    if(request.protocol().UnpackTo(&action) == false)
                    {
                        printf("thread_kafka - action unpack(action) failure\n");
                        continue;
                    }
                    if(cc->m_module->update_action(action))
                    {
                        printf("thread_kafka - action update_action failure\n");
                        continue;
                    }
                    
                    printf("thread_kafka - action update_action success\n");
                    break;
                }
                case 2: // item
                {
                    Item item;
                    if(request.protocol().UnpackTo(&item) == false)
                    {
                        printf("thread_kafka - item unpack(item) failure\n");
                        continue;
                    }
                    if(cc->m_module->update_item(item))
                    {
                        printf("thread_kafka - item update_item failure\n");
                        continue;
                    }
                    
                    printf("thread_kafka - item update_item success\n");
                    break;
                }
                case 3: // subscribe
                {
                    Subscribe subscribe;
                    if(request.protocol().UnpackTo(&subscribe) == false)
                    {
                        printf("thread_kafka - subscribe unpack(subscribe) failure\n");
                        continue;
                    }

                    if(cc->m_module->update_subscribe(subscribe))
                    {
                        printf("thread_kafka - subscribe update_subscribe failure\n");
                        continue;
                    }
                    
                    printf("thread_kafka - subscribe update_subscribe success\n");
                    break;
                }
                default:
                {
                    printf("thread_kafka - protocol type error\n");
                    break;
                }
            }
        }
        
        return NULL;
    }

    static CP_THREAD_T thread_network(var_vd* argv)
    {
        CF_framework_module* cc = (CF_framework_module*)argv;
        
        CP_SOCKET_T client;

        var_4  recv_len = 0;
        var_1* recv_buf = new var_1[cc->m_cfg.md_message_size];
        assert(recv_buf);
        
        var_4  send_len = 0;
        var_1* send_buf = new var_1[cc->m_cfg.md_message_size];
        assert(send_buf);
        
        for(;;)
        {
            if(cc->recv_request(client, recv_buf, cc->m_cfg.md_message_size, recv_len))
            {
                printf("thread_network - recv_request failure\n");
                continue;
            }
            
            TransferRequest t_request;
            if(t_request.ParseFromArray(recv_buf, recv_len) == false)
            {
                printf("thread_network - t_request.ParseFromArray failure\n");
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
                case 6: // feedback
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
    
    CF_framework_interface* m_module;
    
    var_4 m_is_train;
    var_4 m_is_persistent;
    
    UC_Kafka m_kfk;
};

#endif // _CF_FRAMEWORK_MODULE_H_
