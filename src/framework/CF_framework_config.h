//
//  CF_framework_config.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_CONFIG_H_
#define _CF_FRAMEWORK_CONFIG_H_

#include "util/UH_Define.h"
#include "util/UC_ReadConfigFile.h"

#define MAX_MACHINE_NUM 64

typedef struct _center_config_
{
  // framework
  var_u2 fw_listen_port;

  var_4 fw_calc_send_size;
  var_4 fw_calc_recv_size;
  var_4 fw_calc_result_size;

  var_4  fw_calc_num;
  var_1  fw_calc_ip[MAX_MACHINE_NUM][16];
  var_u2 fw_calc_port[MAX_MACHINE_NUM];
  var_f4 fw_calc_factor[MAX_MACHINE_NUM];

  var_1  fw_filter_ip[16];
  var_u2 fw_filter_port;
  var_f4 fw_filter_factor;

  var_4 fw_listen_thread_num;
  var_4 fw_process_thread_num;
  var_4 fw_update_thread_num;
  var_4 fw_queue_size;

  var_4 fw_max_task_size;

  var_4 fw_max_recv_size;
  var_4 fw_max_send_size;
  var_4 fw_max_binary_size;

  var_1 fw_log_path_action[256];
  var_1 fw_log_path_document[256];
  var_1 fw_log_path_pushed[256];
  var_4 fw_log_recoder_interval; // minute

  var_1 fw_cache_path[256];
  var_1 fw_debug_path[256];

  var_1 fw_sync_path[256];
  var_1 fw_sync_ip[16];
  var_1 fw_sync_port;
  var_1 fw_sync_flag;

  var_4 fw_cache_capacity;
  var_4 fw_cache_savecount;
  var_4 fw_cache_flushtime; // minute
  var_4 fw_cache_flushcount;

  var_4 fw_cache_debug;

  var_f4 fw_time_factor;
  var_f4 fw_algorithm_factor;

  // 
  var_4 init()
  {
    UC_ReadConfigFile read;

    if(read.InitConfigFile("config_center.cfg"))
      return -1;

    // framework
    if(read.GetFieldValue("FW_LISTEN_PORT", fw_listen_port))
      return -1;

    if(read.GetFieldValue("FW_CALC_SEND_SIZE", fw_calc_send_size))
      return -1;
    if(read.GetFieldValue("FW_CALC_RECV_SIZE", fw_calc_recv_size))
      return -1;
    if(read.GetFieldValue("FW_CALC_RESULT_SIZE", fw_calc_result_size))
      return -1;

    if(read.GetFieldValue("FW_CALC_NUM", fw_calc_num))
      return -1;
    if(fw_calc_num > MAX_MACHINE_NUM)
      return -1;
    for(var_4 i = 0; i < fw_calc_num; i++)
    {
      var_1 tag[128];

      sprintf(tag, "FW_CALC_IP_%.2d", i);
      if(read.GetFieldValue(tag, fw_calc_ip[i]))
        return -1;

      sprintf(tag, "FW_CALC_PORT_%.2d", i);
      if(read.GetFieldValue(tag, fw_calc_port[i]))
        return -1;

      sprintf(tag, "FW_CALC_FACTOR_%.2d", i);
      if(read.GetFieldValue(tag, fw_calc_factor[i]))
        return -1;
    }

    if(read.GetFieldValue("FW_FILTER_IP", fw_filter_ip))
      return -1;
    if(read.GetFieldValue("FW_FILTER_PORT", fw_filter_port))
      return -1;
    if(read.GetFieldValue("FW_FILTER_FACTOR", fw_filter_factor))
      return -1;

    if(read.GetFieldValue("FW_LISTEN_THREAD_NUM", fw_listen_thread_num))
      return -1;
    if(read.GetFieldValue("FW_PROCESS_THREAD_NUM", fw_process_thread_num))
      return -1;
    if(read.GetFieldValue("FW_UPDATE_THREAD_NUM", fw_update_thread_num))
      return -1;

    if(read.GetFieldValue("FW_QUEUE_SIZE", fw_queue_size))
      return -1;

    if(read.GetFieldValue("FW_MAX_TASK_SIZE", fw_max_task_size))
      return -1;

    if(read.GetFieldValue("FW_MAX_RECV_SIZE", fw_max_recv_size))
      return -1;
    if(read.GetFieldValue("FW_MAX_SEND_SIZE", fw_max_send_size))
      return -1;
    if(read.GetFieldValue("FW_MAX_BINARY_SIZE", fw_max_binary_size))
      return -1;

    if(read.GetFieldValue("FW_LOG_PATH_ACTION", fw_log_path_action))
      return -1;
    if(read.GetFieldValue("FW_LOG_PATH_DOCUMENT", fw_log_path_document))
      return -1;
    if(read.GetFieldValue("FW_LOG_PATH_PUSHED", fw_log_path_pushed))
      return -1;
    if(read.GetFieldValue("FW_LOG_RECODER_INTERVAL", fw_log_recoder_interval))
      return -1;

    if(read.GetFieldValue("FW_CACHE_PATH", fw_cache_path))
      return -1;
    if(read.GetFieldValue("FW_DEBUG_PATH", fw_debug_path))
      return -1;

    if(read.GetFieldValue("FW_SYNC_PATH", fw_sync_path))
      return -1;
    if(read.GetFieldValue("FW_SYNC_IP", fw_sync_ip))
      return -1;
    if(read.GetFieldValue("FW_SYNC_PORT", fw_sync_port))
      return -1;
    if(read.GetFieldValue("FW_SYNC_FLAG", fw_sync_flag))
      return -1;

    if(read.GetFieldValue("FW_CACHE_CAPACITY", fw_cache_capacity))
      return -1;
    if(read.GetFieldValue("FW_CACHE_SAVECOUNT", fw_cache_savecount))
      return -1;
    if(read.GetFieldValue("FW_CACHE_FLUSHTIME", fw_cache_flushtime))
      return -1;
    if(read.GetFieldValue("FW_CACHE_FLUSHCOUNT", fw_cache_flushcount))
      return -1;

    if(read.GetFieldValue("FW_CACHE_DEBUG", fw_cache_debug))
      return -1;

    if(read.GetFieldValue("FW_TIME_FACTOR", fw_time_factor))
      return -1;
    if(read.GetFieldValue("FW_ALGORITHM_FACTOR", fw_algorithm_factor))
      return -1;

    return 0;
  }
} CENTER_CONFIG;

typedef struct _module_config_
{
  // module
  var_u2 md_listen_port;

  var_4  md_persistent_interval; // minute
  var_1  md_persistent_flag[256];

  var_4  md_message_size;

  var_4  md_work_thread_num;

  var_1  md_train_path[256];
  var_1  md_train_flag[256];

  // primary filtrate
  var_4 pf_choose_minutes_scope;
  var_4 pf_max_user_num;      // 系统最大用户数
  var_4 pf_max_circle_num;    // 定时淘汰时，保留的每个用户所订阅的圈子数
  var_4 pf_max_read_num;      // 定时淘汰时，保留的每个用户已阅历史数
  var_4 pf_max_recommend_num; // 定时淘汰时，保留的每个用户已推荐历史数
  var_4 pf_max_dislike_num;   // 定时淘汰时，保留的每个用户不喜欢数
  var_1 pf_store_path[256];   // 序列化路径（不包括文件名）
  var_4 pf_item_num_limit;

  // item based
  var_4 ib_max_item_num;      // 定时淘汰时，保留的item数量
  var_1 ib_store_path[256];   // 序列化路径（不包括文件名）

  // content based information filter
  var_4 cif_para_g;   /* 10 */
  var_4 cif_user_lim; /* 1000000 */
  var_4 cif_cat_lim;  /* 29 */
  var_4 cif_month;    /* 12 */
  var_4 cif_minute;   /* 60 */
  var_4 cif_item_lim; /* 400000 */

  var_1 cif_store_path[256];

  // model based - plsi
  var_4 mcf_user_lim;
  var_4 mcf_item_lim;
  var_4 mcf_group_num; /* k - num of clusters */
  var_4 mcf_log_num;
  var_4 mcf_flush_interval;
  var_4 mcf_interval_num;

  var_1 mcf_store_path[256];

  // model based - minhash
  var_4 mcf_mh_user_lim;
  var_4 mcf_mh_item_lim;
  var_4 mcf_mh_group_num;

  var_1 mcf_mh_store_path[256];
  var_4 mcf_mh_interval_minutes; // 60 minutes
  var_4 mcf_mh_rebuild_num;

  // naive bayesian model
  var_4 nbm_usr_lim; // bounds of users
  var_4 nbm_itm_lim; // bounds of items
  var_4 nbm_cat_no;  // categories of each cat
  var_4 nbm_word_no; // words of each cat
  var_4 nbm_interval_no;
  var_4 nbm_flush_interval;

  var_1 nbm_fn_dic[256];
  var_1 nbm_itm_path[256];

  var_1  md_kafka_host[32];
  var_u2 md_kafka_port;
  var_1  md_kafka_topic[64];
  var_4  md_kafka_partition;
  var_1  md_kafka_save[256];

  var_4 init()
  {
    UC_ReadConfigFile read;

    if(read.InitConfigFile("config_module.cfg"))
      return -1;

    // module
    if(read.GetFieldValue("MD_LISTEN_PORT", md_listen_port))
      return -1;

    if(read.GetFieldValue("MD_PERSISTENT_INTERVAL", md_persistent_interval))
      return -1;
    if(read.GetFieldValue("MD_PERSISTENT_FLAG", md_persistent_flag))
      return -1;

    if(read.GetFieldValue("MD_MESSAGE_SIZE", md_message_size))
      return -1;

    if(read.GetFieldValue("MD_WORK_THREAD_NUM", md_work_thread_num))
      return -1;

    if(read.GetFieldValue("MD_TRAIN_PATH", md_train_path))
      return -1;
    if(read.GetFieldValue("MD_TRAIN_FLAG", md_train_flag))
      return -1;

    // primary filtrate
    if(read.GetFieldValue("PF_CHOOSE_MINUTES_SCOPE", pf_choose_minutes_scope))
      return -1;
    if(read.GetFieldValue("PF_MAX_USER_NUM", pf_max_user_num))
      return -1;
    if(read.GetFieldValue("PF_MAX_CIRCLE_NUM", pf_max_circle_num))
      return -1;
    if(read.GetFieldValue("PF_MAX_READ_NUM", pf_max_read_num))
      return -1;
    if(read.GetFieldValue("PF_MAX_RECOMMEND_NUM", pf_max_recommend_num))
      return -1;
    if(read.GetFieldValue("PF_MAX_DISLIKE_NUM", pf_max_dislike_num))
      return -1;
    if(read.GetFieldValue("PF_STORE_PATH", pf_store_path))
      return -1;
    if(read.GetFieldValue("PF_ITEM_NUM_LIMIT", pf_item_num_limit))
      return -1;

    // item based
    if(read.GetFieldValue("IB_MAX_ITEM_NUM", ib_max_item_num))
      return -1;
    if(read.GetFieldValue("IB_STORE_PATH", ib_store_path))
      return -1;

    // content based information filter
    if(read.GetFieldValue("CIF_PARA_G", cif_para_g))
      return -1;
    if(read.GetFieldValue("CIF_USER_LIM", cif_user_lim))
      return -1;
    if(read.GetFieldValue("CIF_CAT_LIM", cif_cat_lim))
      return -1;
    if(read.GetFieldValue("CIF_MONTH", cif_month))
      return -1;
    if(read.GetFieldValue("CIF_MINUTE", cif_minute))
      return -1;
    if(read.GetFieldValue("CIF_ITEM_LIM", cif_item_lim))
      return -1;

    if(read.GetFieldValue("CIF_STORE_PATH", cif_store_path))
      return -1;

    // model based - plsi
    if(read.GetFieldValue("MCF_USER_LIM", mcf_user_lim))
      return -1;
    if(read.GetFieldValue("MCF_ITEM_LIM", mcf_item_lim))
      return -1;
    if(read.GetFieldValue("MCF_GROUP_NUM", mcf_group_num))
      return -1;
    if(read.GetFieldValue("MCF_LOG_NUM", mcf_log_num))
      return -1;
    if(read.GetFieldValue("MCF_FLUSH_INTERVAL", mcf_flush_interval))
      return -1;
    if(read.GetFieldValue("MCF_INTERVAL_NUM", mcf_interval_num))
      return -1;

    if(read.GetFieldValue("MCF_STORE_PATH", mcf_store_path))
      return -1;

    // model based - minhash
    if(read.GetFieldValue("MCF_MH_USER_LIM", mcf_mh_user_lim))
      return -1;
    if(read.GetFieldValue("MCF_MH_ITEM_LIM", mcf_mh_item_lim))
      return -1;
    if(read.GetFieldValue("MCF_MH_GROUP_NUM", mcf_mh_group_num))
      return -1;

    if(read.GetFieldValue("MCF_MH_STORE_PATH", mcf_mh_store_path))
      return -1;
    if(read.GetFieldValue("MCF_MH_INTERVAL_MINUTES", mcf_mh_interval_minutes))
      return -1;
    if(read.GetFieldValue("MCF_MH_REBUILD_NUM", mcf_mh_rebuild_num))
      return -1;

    // naive bayesian model
    if(read.GetFieldValue("NBM_USR_LIM", nbm_usr_lim))
      return -1;
    if(read.GetFieldValue("NBM_ITM_LIM", nbm_itm_lim))
      return -1;
    if(read.GetFieldValue("NBM_CAT_NO", nbm_cat_no))
      return -1;
    if(read.GetFieldValue("NBM_WORD_NO", nbm_word_no))
      return -1;
    if(read.GetFieldValue("NBM_INTERVAL_NO", nbm_interval_no))
      return -1;
    if(read.GetFieldValue("NBM_FLUSH_INTERVAL", nbm_flush_interval))
      return -1;

    if(read.GetFieldValue("NBM_FN_DIC", nbm_fn_dic))
      return -1;
    if(read.GetFieldValue("NBM_ITM_PATH", nbm_itm_path))
      return -1;

    if(read.GetFieldValue("MD_KAFKA_HOST", md_kafka_host))
      return -1;
    if(read.GetFieldValue("MD_KAFKA_PORT", md_kafka_port))
      return -1;
    if(read.GetFieldValue("MD_KAFKA_TOPIC", md_kafka_topic))
      return -1;
    if(read.GetFieldValue("MD_KAFKA_PARTITION", md_kafka_partition))
      return -1;
    if(read.GetFieldValue("MD_KAFKA_SAVE", md_kafka_save))
      return -1;

    return 0;
  }
} MODULE_CONFIG;

#endif // _CF_FRAMEWORK_CONFIG_H_
