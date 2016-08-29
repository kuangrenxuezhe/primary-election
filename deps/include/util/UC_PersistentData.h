//
//  UC_PersistentData.h
//  code_library
//
//  Created by zhanghl on 13-8-26.
//  Copyright (c) 2014年 zhanghl. All rights reserved.
//

#ifndef __UC_PERSISTENT_DATA_H__
#define __UC_PERSISTENT_DATA_H__

#include "UH_Define.h"

#define UC_PS_END_FLG_VAL	"PSENDFLG"
#define UC_PS_END_FLG_LEN	8

typedef struct PersistentData_Info
{
    
} PERSISTENTDATA_INFO;

class UC_PersistentData
{
public:
    var_4 init(var_1* save_path)
    {
        return 0;
    }
    var_4 open(const var_1* key, var_vd*& handle)
    {
        return 0;
    }
    var_4 close(var_vd*& handle)
    {
        return 0;
    }
    var_4 write_inc(var_vd* handle, var_1* buf, var_4 len)
    {
        return 0;
    }
    var_4 write_glb(var_vd* handle, var_1* buf, var_4 len)
    {
        return 0;
    }
    var_4 read_inc(var_vd* handle, var_1* buf, var_4 len)
    {
        return 0;
    }
    var_4 read_glb(var_vd* handle, var_1* buf, var_4 len)
    {
        return 0;
    }
    var_4 delete_inc(var_vd* handle)
    {
        return 0;
    }
    var_4 delete_glb(var_vd* handle)
    {
        return 0;
    }
};

#endif // __UC_PERSISTENT_DATA_H__
