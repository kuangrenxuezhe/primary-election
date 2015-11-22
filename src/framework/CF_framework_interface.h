//
//  CF_framework_interface.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_INTERFACE_H_
#define _CF_FRAMEWORK_INTERFACE_H_

#include "../code_library/platform_cross/UH_Define.h"

#include "CenterToModuleProtocol.bp.h"

class CF_framework_interface
{
public:
    virtual var_4 module_type() = 0; // CandidateFilter = 1, PowerCalc = 2
    
    virtual var_4 init_module(var_vd* config_info) = 0;
    
    //
    virtual var_4 update_action(Action& action) { return 0; }

    virtual var_4 update_item(const Item& item) { return 0; }
    
    virtual var_4 update_subscribe(const Subscribe& subscribe) { return 0; }
    
    virtual var_4 update_feedback(const Feedback& feedback) { return 0; }
    
    //
    virtual var_4 query_algorithm(const CandidateSetBase& csb, AlgorithmPower* ap) { return -999; }

    virtual var_4 query_user_category(const Category& category, AlgorithmCategory* ac) { return -999; }
    
    virtual var_4 query_candidate_set(const Recommend& recommend, CandidateSet* cs) { return -999; }

    virtual var_4 query_user_status(const User& user, UserStatus* us) { return -999; }
    
    //
    virtual var_4 is_persistent_library() = 0;
    virtual var_4 persistent_library() { return 0; }
    
    //
    virtual var_4 is_update_train() = 0;
    virtual var_4 update_train(var_1* update_path) { return 0; }
};

#endif // _CF_FRAMEWORK_INTERFACE_H_
