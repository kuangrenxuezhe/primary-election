/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : <Types.h>
相关文件     : 
文件实现功能 : <> 
作者         : <和文强>
版本         : <1.00.00>
--------------------------------------------------------------------------------
备注         : <编译配置文件>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
YYYY/MM/DD   X.Y      <作者或修改者名>    <修改内容>
</PRE>
*******************************************************************************/

#ifndef _Types_H_2006_11_25
#define _Types_H_2006_11_25

#include <string.h>
#if defined(_WIN32)
	//
	// Windows/Visual C++
	//
	typedef signed char            Int8;
	typedef unsigned char          UInt8;
	typedef signed short           Int16;
	typedef unsigned short         UInt16;
	typedef signed int             Int32;
	typedef unsigned int           UInt32;
	typedef signed __int64         Int64;
	typedef unsigned __int64       UInt64;

#define Strncasecmp strnicmp 
#else
	//
	// Unix/Linux/GCC
	//
	typedef signed char            Int8;
	typedef unsigned char          UInt8;
	typedef signed short           Int16;
	typedef unsigned short         UInt16;
	typedef signed int             Int32;
	typedef unsigned int           UInt32;
	typedef signed long long       Int64;
	typedef unsigned long long     UInt64;

#define Strncasecmp strncasecmp
#endif

	
#pragma pack(1)
	typedef struct _TEMPLATE_STRUCT 
	{
		char  szpWebName[50];               //webname
		char  szpTptName[50];               //摸版名
		char  szpUrl[256];                  //web_url
		char  szpBegin[256];                //摸版头
		char  szpEnd[256];					//摸版尾
		char  szaUrlFilter[50];             //url过滤
		char  szpInterBegin[256];			//相关头
		char  szpInterEnd[256];              //相关为
		int   iPageTitle;				    //页面标题
		int   urlFlg;                       //url标签
		int   exeFlg;                       //执行标示,执行的次数
		bool  interrelated;                 //有无相关
	} Template;
	
	typedef struct _URLNODE_STRUCT {
		char szpUrl[256];                   //url
		char szpPUrl[256];                  //pic url
		char szpTitle[256];                 //url_text
		Template* pTemplate;                //摸版
		_URLNODE_STRUCT* pNext;             //有相关不为空
		_URLNODE_STRUCT* pPrior;            //有相关不为空
		char* pContext;                     //内容
		char szpNewsTime[30];               //新闻时间
		bool  isHeader;                     //有相关不为空时判断是否为头
		unsigned long intNum;               //相关号 用于数据更新
		int  urlFlg;                        //分析标示 0,1,2,3
	} UrlNode;
#pragma pop()

#endif 
