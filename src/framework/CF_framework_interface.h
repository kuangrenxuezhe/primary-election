//
//  CF_framework_interface.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_INTERFACE_H_
#define _CF_FRAMEWORK_INTERFACE_H_

#include "util/UH_Define.h"

class CF_framework_interface
{
public:
    virtual var_4 module_type() = 0; // itmeFilter = 1, powerCalc = 2
    
    //
    virtual var_4 init_module(var_vd* config_info) = 0;

    // for algorithm: type(var_4, no_use), history_num(var_4), history_list(var_u8 * history_num),
    //                type(var_4, no_use), clicktime(var_4), user_id(var_u8), doc_id(var_u8), staytime(var_4), action(var_4),
    //                location_len(var_4), location(location_len), srpid_len(var_4), srpid(srpid_len)
    //                source(var_4)
    // for candidate: type(var_4, no_use), clicktime(var_4), user_id(var_u8), doc_id(var_u8), staytime(var_4), action(var_4),
    //                location_len(var_4), location(location_len), srpid_len(var_4), srpid(srpid_len)
    //                source(var_4)
    virtual var_4 update_click(var_1* click_info) { return 0; }

    // type(var_4, no_use), doc_id(var_u8), public_time(var_4), push_time(var_4), power(var_f4),
    // category_num(var_4), [category_name_len(var_4), category_name(category_name_len)] * category_num,
    // word_num(var_4), [word_len(var_4), word(word_len), word_power(var_4)] * word_num,
    // srp_num(var_4), [srp_name_len(var_4), srp_name(srp_name_len)] * srp_num,
    // circle_num(var_4), [circle_name_len(var_4), circle_name(circle_name_len)] * circle_num,
    // picture_num(var_4),
    // top_flag(var_4),
    // top_srp_num(var_4), [top_srp_name_len(var_4), top_srp_name(top_srp_name_len)] * top_srp_num,
    // top_circle_num(var_4), [top_circle_name_len(var_4), top_circle_name(top_circle_name_len)] * top_circle_num,
    // doc_type(var_4), srp_power_num(var_4), [srp_power(var_f4)] * srp_power_num
    virtual var_4 update_item(var_1* item_info) { return 0; }
    
    // type(var_4, no_use), user_id(var_u8),
    // srp_num(var_4), [srp_name_len(var_4), srp_name(srp_name_len)] * srp_num,
    // circle_num(var_4), [circle_name_len(var_4), circle_name(circle_name_len)] * circle_num
    virtual var_4 update_user(var_1* user_info) { return 0; }
    
    //
    virtual var_4 query_algorithm(var_u8 user_id, var_4 recommend_num, var_u8* recommend_list, var_f4* recommend_power, var_4 history_num, var_u8* history_list) { return -999; }
    virtual var_4 query_userclass(var_u8 user_id, var_4& class_num, var_4* class_info) { return -999; }
    
    // var_4& recommend_num, var_u8* recommend_list, var_f4* recommend_power, var_4* recommend_time, var_4* recommend_class, var_4* recommend_picnum, var_4& history_num, var_u8* history_list
    virtual var_4 query_recommend(var_u8 user_id, var_4 flag, var_4 beg_time, var_4 end_time, var_1* result_buf, var_4 result_max, var_4& result_len) { return -999; }
    // var_4& history_num, var_u8* history_list
    virtual var_4 query_history(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len) { return -999; }
    // var_4 new_user_flag: 1 - new user, 0 - old user
    virtual var_4 query_user(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len) { return -999; }
    
    //
    virtual var_4 is_persistent_library() = 0;
    virtual var_4 persistent_library() { return 0; }
    
    //
    virtual var_4 is_update_train() = 0;
    virtual var_4 update_train(var_1* update_path) { return 0; }
    
    //
    virtual var_4 update_pushData(var_u8 user_id, var_4 push_num, var_u8* push_data) { return 0; }
};

#endif // _CF_FRAMEWORK_INTERFACE_H_
