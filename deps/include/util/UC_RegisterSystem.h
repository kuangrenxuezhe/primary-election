//
//  UC_RegisterSystem.h
//  code_library
//
//  Created by zhanghl on 12-11-6.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_REGISTERSYSTEM_H_
#define _UC_REGISTERSYSTEM_H_

#include "UH_Define.h"
#include "UC_Communication.h"
#include "UT_HashSearch.h"
#include "UC_MD5.h"

#define MAX_REGISTER_SIZE   62
#define MAX_REGISTER_NUM    512

class UC_RegisterSystem_Client
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化注册系统客户端
    // 入参:
    //                  server_ip: 服务端ip
    //                server_port: 服务端port
    //                   time_out: 超时时间,默认为5s
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    const var_4 init(const var_1* server_ip, const var_u2 server_port, var_4 time_out = 5000)
    {
        if(m_cc.init(server_ip, server_port, time_out))
            return -1;
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      register
    // 功能:      向服务端注册
    // 入参:
    //              register_name: 向服务端注册的名称
    //              register_size: 向服务端注册的名称长度
    //               register_key: 服务端返回的注册号
    //                  server_ip: 指定服务器ip,如果指定则使用当前的,否则使用初始化参数
    //                server_port: 指定服务器port,如果指定则使用当前的,否则使用初始化参数
    //                   time_out: 指定服务器超时时间,如果指定则使用当前的,否则使用初始化参数,默认为5s
    //
    // 出参:
    // 返回值:    返回<0函数执行失败,0已经到达注册系统最大数,1新注册成功,2已经被注册过
    // 备注:      对于新注册成功与被注册过,返回的注册号存放在register_key
    //////////////////////////////////////////////////////////////////////
    const var_4 register_to_server(var_1* register_name, var_4 register_size, var_u8* register_key, var_1* server_ip = NULL, var_u2 server_port = 0, var_4 time_out = 5000)
    {
        if(register_size > MAX_REGISTER_SIZE)
            return -1;
        
        // send - "REGISTER"(8) package_size(4) register_size(4) register_name(register_size)
        var_1 buffer[128];
        
        *(var_u8*)buffer = *(var_u8*)"REGISTER";
        *(var_4*)(buffer + 8) = 4 + register_size;
        *(var_4*)(buffer + 12) = register_size;
        memcpy(buffer + 16, register_name, register_size);
        
        var_4 buflen = 16 + register_size;
        
        var_vd* handle = NULL;
        
        if(m_cc.open(handle, server_ip, server_port, time_out))
            return -1;
        
        if(m_cc.send(handle, buffer, buflen))
        {
            m_cc.close(handle);
            return -1;
        }
        
        // recv - "REGISTER"(8) register_flag(4) register_key(8)
        buflen = 20;
        if(m_cc.recv(handle, buffer, buflen))
        {
            m_cc.close(handle);
            return -1;
        }
        
        m_cc.close(handle);
        
        if(*(var_u8*)buffer != *(var_u8*)"REGISTER")
            return -1;
        
        var_4 register_flag = *(var_4*)(buffer + 8);
        if(register_flag != 0 && register_flag != 1 && register_flag != 2)
            return -1;
        
        *register_key = *(var_u8*)(buffer + 12);
        
        return register_flag;
    }
    
private:
    UC_Communication_Client m_cc;
};

class UC_RegisterSystem_Server : public UC_Communication_Server
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化注册系统服务端
    // 入参:
    //                  save_path: 注册文件保存路径
    //                listen_port: 服务端监听port
    //           fun_notification: 当有新注册的服务时,调用的回调函数
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    const var_4 init(const var_1* save_path, var_u2 listen_port, var_vd (*fun_notification)(var_u8 reg_key, var_4 reg_len, var_1* reg_val) = NULL)
    {
        m_fun_notification = fun_notification;
        
        strcpy(m_save_path, save_path);
        
        sprintf(m_save_file_lib, "%s/register.info", m_save_path);
        sprintf(m_save_file_bak, "%s/register.back", m_save_path);
        
        m_register_num = 0;
        
        if(cp_create_dir(m_save_path))
            return -1;
        
        if(cp_recovery_file(m_save_file_lib))
            return -1;
        
        if(load_register_info())
            return -1;
        
        if(UC_Communication_Server::init(listen_port, 1, 5000, 1, 12, 32, 256, 256))
            return -1;
        
        if(start())
            return -1;
        
        return 0;
    }
    
    var_4 cs_fun_package(const var_1* in_buf, const var_4 in_buf_size, const var_vd* handle = NULL)
    {
        // send - "REGISTER"(8) package_size(4) register_size(4) register_name(register_size)
        
        if(*(var_u8*)in_buf != *(var_u8*)"REGISTER")
            return -1;
        
        if(*(var_4*)(in_buf + 8) > 128)
            return -1;
        
        return *(var_4*)(in_buf + 8);
    }
    
    var_4 cs_fun_process(const var_1* in_buf, const var_4 in_buf_size, var_1* const out_buf, const var_4 out_buf_size, const var_vd* handle = NULL)
    {
        if(*(var_4*)in_buf > MAX_REGISTER_SIZE)
            return -1;
        
        var_u8 register_key = md5.MD5Bits64((var_u1*)(in_buf + 4), *(var_4*)in_buf);
        var_4  register_pos = 0;
        
        for(var_4 i = 0; i < m_register_num; i++)
        {
            if(m_register_key[i] == register_key)
            {
                register_pos = i;
                break;
            }
        }
        
        *(var_u8*)out_buf = *(var_u8*)"REGISTER";
        
        if(register_pos == m_register_num)
        {
            if(m_register_num >= MAX_REGISTER_NUM)
                *(var_4*)(out_buf + 8) = 0;
            else
            {
                m_register_key[m_register_num] = register_key;
                m_register_len[m_register_num] = *(var_4*)in_buf;
                memcpy(m_register_val[m_register_num], in_buf + 4, *(var_4*)in_buf);
                m_register_num++;
                
                if(save_register_info())
                {
                    m_register_num--;
                    return -1;
                }
                
                if(cp_swap_file(m_save_file_bak, m_save_file_lib))
                {
                    m_register_num--;
                    return -1;
                }
                
                *(var_4*)(out_buf + 8) = 1;
            }
        }
        else
            *(var_4*)(out_buf + 8) = 2;
        
        *(var_u8*)(out_buf + 12) = register_key;
        
        if(m_fun_notification && *(var_4*)(out_buf + 8) == 1)
            m_fun_notification(m_register_key[m_register_num - 1], m_register_len[m_register_num - 1], m_register_val[m_register_num - 1]);

        return 20;
    }
    
private:
    var_4 save_register_info()
    {
        FILE* fp = fopen(m_save_file_bak, "wb");
        if(fp == NULL)
            return -1;
        
        if(fwrite(&m_register_num, 4, 1, fp) != 1)
        {
            fclose(fp);
            return -1;
        }
        
        for(var_4 i = 0; i < m_register_num; i++)
        {
            if(fwrite(m_register_key + i, 8, 1, fp) != 1)
            {
                fclose(fp);
                return -1;
            }
            if(fwrite(m_register_len + i, 4, 1, fp) != 1)
            {
                fclose(fp);
                return -1;
            }
            if(fwrite(m_register_val[i], m_register_len[i], 1, fp) != 1)
            {
                fclose(fp);
                return -1;
            }
        }
        
        fclose(fp);
        
        return 0;
    }
    
    var_4 load_register_info()
    {
        if(access(m_save_file_lib, 0))
        {
            printf("register.info no exist, is new register system!\n");
            return 0;
        }
        
        FILE* fp = fopen(m_save_file_lib, "rb");
        if(fp == NULL)
            return -1;
        
        if(fread(&m_register_num, 4, 1, fp) != 1)
        {
            fclose(fp);
            return -1;
        }
        
        for(var_4 i = 0; i < m_register_num; i++)
        {
            if(fread(m_register_key + i, 8, 1, fp) != 1)
            {
                fclose(fp);
                return -1;
            }
            if(fread(m_register_len + i, 4, 1, fp) != 1)
            {
                fclose(fp);
                return -1;
            }
            if(fread(m_register_val[i], m_register_len[i], 1, fp) != 1)
            {
                fclose(fp);
                return -1;
            }
        }
        
        fclose(fp);
        
        return 0;
    }
    
private:
    var_1  m_save_path[256];
    var_1  m_save_file_lib[256];
    var_1  m_save_file_bak[256];
    
    var_u8 m_register_key[MAX_REGISTER_NUM];
    var_4  m_register_len[MAX_REGISTER_NUM];
    var_1  m_register_val[MAX_REGISTER_NUM][64];
    var_4  m_register_num;
    
    UC_MD5 md5;
    
    var_vd (*m_fun_notification)(var_u8 reg_key, var_4 reg_len, var_1* reg_val);
};

#endif // _UC_REGISTERSYSTEM_H_
