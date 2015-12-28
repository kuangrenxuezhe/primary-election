//
//  CF_framework_jparse.h
//
//  Created by zhanghl on 15-8-17.
//  Copyright (c) 2015年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_JPARSE_H_
#define _CF_FRAMEWORK_JPARSE_H_

#include "util/UH_Define.h"
#include "cJSON.h"
#include "proto/message.pb.h"

using namespace module::protocol;

class CF_framework_jparse
{
public:
    var_4 parse_json(var_1* json_buffer, TransferRequest* t_request)
    {
        cJSON* json = cJSON_Parse(json_buffer);
        if(json == NULL)
        {
            printf("parse_json - parse error:\n%s\n", json_buffer);
            return -1;
        }
        
        cJSON* node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
        {
            cJSON_Delete(json);
            
            printf("parse_json - no find type:\n%s\n", json_buffer);
            return -1;
        }
        
        var_4 protocol_type = node->valueint;
        var_4 code = -888;
        
        printf("parse_json - type value %d\n", protocol_type);
        
        t_request->set_main_protocol(protocol_type);
        
        switch (protocol_type)
        {
            case 1:
            {
                Action action;
                code = parse_action(json, &action);
                t_request->mutable_protocol()->PackFrom(action);
                break;
            }
            case 2:
            {
                Item item;
                code = parse_item(json, &item);
                t_request->mutable_protocol()->PackFrom(item);
                break;
            }
            case 3:
            {
                Subscribe subscribe;
                code = parse_subscribe(json, &subscribe);
                t_request->mutable_protocol()->PackFrom(subscribe);
                break;
            }
            case 4:
            {
                Recommend recommend;
                code = parse_recommend(json, &recommend);
                t_request->mutable_protocol()->PackFrom(recommend);
                break;
            }
            case 5:
            {
                HeartBeat heartbeat;
                code = parse_heartbeat(json, &heartbeat);
                t_request->mutable_protocol()->PackFrom(heartbeat);
                break;
            }
            case 6:
            {
                Feedback feedback;
                code = parse_feedback(json, &feedback);
                t_request->mutable_protocol()->PackFrom(feedback);
                break;
            }
            default:
                printf("parse_json - type value error:\n%s\n", json_buffer);
                break;
        }
        
        cJSON_Delete(json);
        
        return code;
    }

private:
    var_4 parse_action(cJSON* json, Action* action)
    {
        cJSON* node = NULL;
        
        node = cJSON_GetObjectItem(json, "clicktime");
        if(node == NULL)
            return -101;
        else
            action->set_click_time(node->valueint);
        
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -102;
        else
            action->set_user_id(cp_strtoval_u64(node->valuestring));
        
        node = cJSON_GetObjectItem(json, "docid");
        if(node == NULL)
            return -103;
        else
            action->set_item_id(cp_strtoval_u64(node->valuestring));
        
        node = cJSON_GetObjectItem(json, "staytime");
        if(node == NULL)
            action->set_stay_time(0);
        else
            action->set_stay_time(node->valueint);
        
        node = cJSON_GetObjectItem(json, "action");
        if(node == NULL)
            return -104;
        else
            action->set_action((ActionType)node->valueint);
        
        node = cJSON_GetObjectItem(json, "location");
        if(node == NULL)
            action->set_location("");
        else
            action->set_location(node->valuestring);
        
        node = cJSON_GetObjectItem(json, "srpid");
        if(node == NULL)
            action->set_srp_id("");
        else
            action->set_srp_id(node->valuestring);
        
        node = cJSON_GetObjectItem(json, "source");
        if(node == NULL)
            action->set_click_source(0);
        else
            action->set_click_source(node->valueint);
        
        node = cJSON_GetObjectItem(json, "dislike");
        if(node == NULL)
            action->set_dislike("");
        else
            action->set_dislike(node->valuestring);
        
        node = cJSON_GetObjectItem(json, "zone");
        if(node == NULL)
            action->set_zone("");
        else
            action->set_zone(node->valuestring);
        
        return 0;
    }
    
    var_4 parse_item(cJSON* json, Item* item)
    {
        cJSON* node = NULL;
        cJSON* array = NULL;
        cJSON* object = NULL;
        
        node = cJSON_GetObjectItem(json, "docid");
        if(node == NULL)
            return -200;
        else
            item->set_item_id(cp_strtoval_u64(node->valuestring));
  
        node = cJSON_GetObjectItem(json, "publictime");
        if(node == NULL)
            item->set_publish_time(0);
        else
            item->set_publish_time(node->valueint);
        
        item->set_push_time((var_4)time(NULL));
        
        node = cJSON_GetObjectItem(json, "picturenumber");
        if(node == NULL)
            item->set_picture_num(0);
        else
            item->set_picture_num(node->valueint);
        
        node = cJSON_GetObjectItem(json, "power");
        if(node == NULL)
            item->set_power(0);
        else
            item->set_power(node->valuedouble);
        
        node = cJSON_GetObjectItem(json, "doctype");
        if(node == NULL)
            item->set_item_type((ItemType)0);
        else
            item->set_item_type((ItemType)node->valueint);
        
        array = cJSON_GetObjectItem(json, "category");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -201;
                
                ItemTag* it = item->add_category();
                it->set_tag_name(node->valuestring);
            }
        }
        
        array = cJSON_GetObjectItem(json, "categoryid");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            if(num != item->category_size())
                return -202;
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -203;
                
                ItemTag* it = item->mutable_category(i);
                it->set_tag_id(node->valueint);
            }
        }
        
        if(item->category_size() == 0)
        {
            ItemTag* it = item->add_category();
            it->set_tag_name("");
            it->set_tag_id(13);
        }
        
        array = cJSON_GetObjectItem(json, "word");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                object = cJSON_GetArrayItem(array, i);
                if(object == NULL)
                    return -204;
                
                cJSON* node_one = cJSON_GetObjectItem(object, "word");
                if(node_one == NULL)
                    return -205;
                
                cJSON* node_two = cJSON_GetObjectItem(object, "count");
                if(node == NULL)
                    return -206;
                
                ItemWord* iw = item->add_word();
                
                iw->set_word(node_one->valuestring);
                iw->set_count(node_two->valueint);
            }
        }
        
        array = cJSON_GetObjectItem(json, "srp");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -207;
                
                ItemTag* it = item->add_srp();
                it->set_tag_name(node->valuestring);
            }
        }
        
        array = cJSON_GetObjectItem(json, "srppower");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            if(num != item->srp_size())
                return -208;
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -209;
                
                ItemTag* it = item->mutable_srp(i);
                it->set_tag_power(node->valuedouble);
            }
        }
        
        array = cJSON_GetObjectItem(json, "circle");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -210;
                
                ItemTag* it = item->add_circle();
                it->set_tag_name(node->valuestring);
            }
        }
        
        TopInfo* ti = item->mutable_top_info();
        
        node = cJSON_GetObjectItem(json, "topflag");
        if(node == NULL)
            ti->set_top_type((TopType)0);
        else
        {
            ti->set_top_type((TopType)node->valueint);
            if(node->valueint)
            {
                array = cJSON_GetObjectItem(json, "topsrp");
                if(array)
                {
                    var_4 num = cJSON_GetArraySize(array);
                    
                    for(var_4 i = 0; i < num; i++)
                    {
                        node = cJSON_GetArrayItem(array, i);
                        if(node == NULL)
                            return -211;
                        
                        ti->add_top_srp_id(node->valuestring);
                    }
                }
                
                array = cJSON_GetObjectItem(json, "topcircle");
                if(array)
                {
                    var_4 num = cJSON_GetArraySize(array);
                    
                    for(var_4 i = 0; i < num; i++)
                    {
                        node = cJSON_GetArrayItem(array, i);
                        if(node == NULL)
                            return -212;
                        
                        ti->add_top_circle_id(node->valuestring);
                    }
                }
            }
        }
        
        array = cJSON_GetObjectItem(json, "zone");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -213;
                
                item->add_zone(node->valuestring);
            }
        }
    
        ItemSource* is = item->mutable_data_source();
        
        node = cJSON_GetObjectItem(json, "sourcename");
        if(node == NULL)
            is->set_source_name("");
        else
            is->set_source_name(node->valuestring);
        
        node = cJSON_GetObjectItem(json, "sourceid");
        if(node == NULL)
            is->set_source_id(0);
        else
            is->set_source_id(cp_strtoval_u64(node->valuestring));
        
        array = cJSON_GetObjectItem(json, "tag");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -207;
                
                ItemTag* it = item->add_tag();
                it->set_tag_name(node->valuestring);
            }
        }
        
        array = cJSON_GetObjectItem(json, "tagid");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            if(num != item->tag_size())
                return -208;
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -209;
                
                ItemTag* it = item->mutable_tag(i);
                it->set_tag_id(cp_strtoval_u64(node->valuestring));
            }
        }
        
        array = cJSON_GetObjectItem(json, "tagpower");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            if(num != item->tag_size())
                return -208;
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -209;
                
                ItemTag* it = item->mutable_tag(i);
                it->set_tag_power(node->valuedouble);
            }
        }
        
        return 0;
    }
    
    var_4 parse_subscribe(cJSON* json, Subscribe* subscribe)
    {
        cJSON* node = NULL;
        cJSON* array = NULL;
        
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -300;
        else
            subscribe->set_user_id(cp_strtoval_u64(node->valuestring));
        
        array = cJSON_GetObjectItem(json, "srp");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -301;
                
                subscribe->add_srp_id(node->valuestring);
            }
        }
        
        array = cJSON_GetObjectItem(json, "circle");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -302;
                
                subscribe->add_circle_id(node->valuestring);
            }
        }
        
        return 0;
    }
    
    var_4 parse_recommend(cJSON* json, Recommend* recommend)
    {
        cJSON* node = NULL;
        
        node = cJSON_GetObjectItem(json, "log");
        if(node == NULL)
            recommend->set_log(0);
        else
            recommend->set_log(node->valueint);
        
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -400;
        else
            recommend->set_user_id(cp_strtoval_u64(node->valuestring));
        
        node = cJSON_GetObjectItem(json, "requestnum");
        if(node == NULL)
            recommend->set_request_num(10);
        else
            recommend->set_request_num(node->valueint);
        
        node = cJSON_GetObjectItem(json, "requesttype");
        if(node == NULL)
            recommend->set_recommend_type((RecommendType)0);
        else
            recommend->set_recommend_type((RecommendType)node->valueint);
        
        node = cJSON_GetObjectItem(json, "begtime");
        if(node == NULL)
            recommend->set_beg_time(0);
        else
            recommend->set_beg_time(node->valueint);

        node = cJSON_GetObjectItem(json, "endtime");
        if(node == NULL)
            recommend->set_end_time(0);
        else
            recommend->set_end_time(node->valueint);
        
        node = cJSON_GetObjectItem(json, "zone");
        if(node == NULL)
            recommend->set_zone("");
        else
            recommend->set_zone(node->valuestring);

        node = cJSON_GetObjectItem(json, "network");
        if(node == NULL)
            recommend->set_network((RecommendNetwork)0);
        else
            recommend->set_network((RecommendNetwork)node->valueint);

        return 0;
    }
    
    var_4 parse_heartbeat(cJSON* json, HeartBeat* heartbeat)
    {
        cJSON* node = NULL;

        node = cJSON_GetObjectItem(json, "heartbeat");
        if(node == NULL)
            return -500;
        else
            heartbeat->set_heartbeat(node->valuestring);

        return 0;
    }
    
    var_4 parse_feedback(cJSON* json, Feedback* feedback)
    {
        cJSON* node = NULL;
        cJSON* array = NULL;
        
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -600;
        else
            feedback->set_user_id(cp_strtoval_u64(node->valuestring));
        
        array = cJSON_GetObjectItem(json, "returnlist");
        if(array)
        {
            var_4 num = cJSON_GetArraySize(array);
            
            for(var_4 i = 0; i < num; i++)
            {
                node = cJSON_GetArrayItem(array, i);
                if(node == NULL)
                    return -601;
                
                feedback->add_item_id(cp_strtoval_u64(node->valuestring));
            }
        }

        return 0;
    }
};

#endif // _CF_FRAMEWORK_JPARSE_H_
