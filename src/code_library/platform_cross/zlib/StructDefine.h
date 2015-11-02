// StructDefine.h
#ifndef _STRUCTDEFINE_H__
#define _STRUCTDEFINE_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#include <direct.h>

#define MAX_URL_LEN 256					// Url
#define MAX_MODULE_PAGE_NUM		256
#define MAX_MODULE_BLOCK_NUM	256

#define MAX_MACHINE_NUM		64		// 最大机器数	
#define MAX_BLOCK_NUM		80		// 页下最大块儿数
#define MAX_ITEM_NUM		16		// 一个Rss下最大item数量
#define MAX_LIST_NUM		16		// 一个Item下最大列表数量
#define MAX_DATA_SIZE		(1<<20)

#define WIN32COMMON
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned __int64 u_int64;

/*
字段说明					标识符		字段类型		字段类型	字段长度
页的ID						id			unsigned long	number		20
页的title1					name1		char*			Varchar2	60
页的title2					name2		char*			Varchar2	60
页的类型					type		unsigned char	char		1
*/
/*
struct TabInfo
{
	u_long id[32];				
	char name1[32][61];			
	char name2[32][61];			
	u_char type[32];			

	char num;
};
*/
/*
字段说明														标识符		字段类型		字段类型	字段长度
块ID															id			unsigned long	number		20
块标题															title		char*			Varchar2	60
块图标															icon		char*			Varchar2	60
块类型															type		unsigned char	number		2
块外框的css(边框颜色，边框是否显示和背景颜色)					outstyle	char*			Varchar2	200
块标题条的css(标题条的背景色和是否显示标题条)					tstyle		char*			Varchar2	200
块图标是否显示              									ityle		char*			Varchar2	200
块标题字体的css(字体，字号，字颜色，字背景色，加粗，斜体)		tfstyle		char*			Varchar2	200
块内容区的字体css												cfstyle		char*			Varchar2	200
块的模板号														mtemplet	unsigned char	number		10
判断块是否引用的标示											set			char			char		1
更新提醒														remind		char			char		1
信息过滤(多个词之间用空格分开)									info		char*			Varchar2	100
块的数据url														url			char*			Varchar2	200
块的源数据url(只是在原味订阅中有值)								burl		char*			Varchar2	200
块的关键词														keyword		char*			Varchar2	60
块的显示行数													num			unsigned short	number		2
数据类型(新闻，论坛)											infotype	char			number		2
数据来源(中搜，百度)											server		char*			Varchar2	60
判读是否用户指定的标题的标示									istitle		char			char		1
在天气预报块中表示省份：吉林，在电视预报块中表示地区：央视		pr			char*			Varchar2	60
在天气预报块中表示地区：长春，在电视预报块中表示电台：CCTV-5	ct			char*			Varchar2	60
"更多"，用户可改文字。											more        char*           Varchar2    60
"更多的链接"，用户也可改链接地址。                              mlink       char*           Varchar2    200
"是否显示更多"，用户可设为显示或不显示。                        ismore      char            char        1
*/
/*
struct BlockInfo
{
	u_long id;			
	char title[61];		
	char icon[61];		
	u_short type;		
	char outstyle[201];	
	char tstyle[201];	
	char ityle[200];			
	char tfstyle[201];	
	char cfstyle[201];	
	u_long mtemplet;	
	char set;			
	char remind;		
	char info[101];		
	char url[513];		
	char burl[201];		
	char keyword[61];	
	u_short num;		
	u_short infotype;	
	char server[61];	
	char istitle;		
	char pr[61];		
	char ct[61];
	char more[61];
	char mlink[201];
	char ismore;

	char isvalid;
};
*/
/*
字段说明					标识符		字段类型		字段类型	字段长度
页的ID						id			unsigned long	number		20
页的title1					name1		char*			Varchar2	60
页的title2					name2		char*			Varchar2	60
页的类型					type		unsigned char	char		1
页户主						start		char*			Varchar2	120
关键字						keyword		char*			Varchar2	60
页css地址(例：/css/xxx.css)	stylepath	char*			Varchar2	200
页的url						url			char*			Varchar2	200
今日关注块的标示			fg			char			char		1
块是否挤占(1挤占,0不挤占)   isoverlap   bool            char        1
页结构						module		char*			Varchar2	600
*/
/*
struct BlockPos
{
	u_long id;
	u_short lt; // left top
	u_short rt; // right top
	u_short lb; // left bottom
	u_short rb; // right bottom
};
struct PageInfo
{
	u_long id;				
	char name1[61];			
	char name2[61];			
	char type;				
	char start[121];		
	char keyword[61];		
	char stylepath[201];	
	char url[201];			
	char fg;	
	char isoverlap;
	BlockPos module[80];
	char modulenum;
	
	char isvalid;
};
*/
struct BaseInfo
{
	long titlelen;
	char* title;

	long descriptionlen;
	char* description;

	long linklen;
	char* link;
	
	long pubDatalen;
	char* pubData;

	long sourcelen;
	char* source;

	long authorlen;
	char* author;

	long docurllen;
	char* docurl;
};
struct XMLInfo
{
	char isvalid;
	char* content;
	long contentlen;
	BaseInfo cPubInfo;

	long lItemNum;
	BaseInfo caItemInfo[MAX_ITEM_NUM];

	XMLInfo(){memset(this, 0, sizeof(XMLInfo));}
	void Clear()
	{
		memset(&cPubInfo, 0, sizeof(BaseInfo));
		memset(caItemInfo, 0, MAX_ITEM_NUM * sizeof(BaseInfo));
	}
};
/*
typedef struct _Express_Overlapped_
{
	// 标准结构
	OVERLAPPED cOverlapped;
	SOCKET sClient;	
	char szaBuffer[IPADDRESSLEN<<1];
	char szDealType;

	WSABUF wsaBuf;

	// 服务端接收缓冲区
	char* szpRecvBuf;
	long  lRecvFinLen;
	long  lRecvRemLen;

	// 服务端发送缓冲区
	char* szpSendBuf;
	long  lSendFinLen;
	long  lSendRemLen;

	// 检索计数
	long lSearchCount;
} EXPRESS_OVERLAPPED;

typedef struct D2_Data_Node
{
	u_int64 doc;		// DocID命中列表
	u_int64 site;		// SitID命中列表
	u_int64 company;	// 公司名编码列表	
	u_int64 source;		// 来源ID列表
	unsigned long time;	// 时间列表
	float value;		// 价格列表
	float power;		// 权值列表
	long good;			// 诚信指数列表
}D2_RESULT;

typedef struct _NewsCache_ConfigInfo_
{
	long lOverLappedNum;       // 重叠结构个数

	long lSendBufSize;         // 发送缓冲区大小
	long lRecvBufSize;         // 接收缓冲区大小
	long lRecvThreadNum;       // 接收线程数
	unsigned short usRecvPort; // 接收端口号

	u_short usCacheAddPort;		// Cache 加载端口
	long lFileNumPage;			// 页Cache文件个数
	char szPathPage[128];		// 页Cache文件路径
	long lFileNumBlock;			// 块Cache文件个数
	char szPathBlock[128];		// 块Cache文件路径
	long lFileNumTabReg;		// 注册用户页标签
	char szPathTabReg[128];		// 
	long lFileNumTabNReg;		// 非注册用户页标签
	char szPathTabNReg[128];
	char szusername[128];		// 用户名
	char szuserpwd[128];		// 密码
	char szdbname[128];			// 数据库名

	long lDataMacNum;		    // 发送机器数量
	u_short usDataSendPort;		// 发送端口号
	long lDataThreadNum;		// 发送线程数(Pre IP)
	char szaDataMacIP[MAX_MACHINE_NUM][16];	// 发送机器IP

	long InitConfigInfo(char* configfile)
	{
		char configname[128];

		UC_ReadConfigFile cConfig;
		if(cConfig.InitConfigFile(configfile))
			return -1;
		if(cConfig.GetFieldValue("OVER_LAPPED_NUM", lOverLappedNum))
			return -1;
		if(cConfig.GetFieldValue("SEND_BUF_SIZE", lSendBufSize))
			return -1;
		if(cConfig.GetFieldValue("RECV_BUF_SIZE", lRecvBufSize))
			return -1;
		if(cConfig.GetFieldValue("RECV_THREAD_NUM", lRecvThreadNum))
			return -1;
		if(cConfig.GetFieldValue("RECV_PORT", usRecvPort))
			return -1;
		// Cache Info
		if(cConfig.GetFieldValue("CACHE_ADD_PORT", usCacheAddPort))
			return -1;
		if(cConfig.GetFieldValue("CACHE_PAGE_NUM", lFileNumPage))
			return -1;
		if(cConfig.GetFieldValue("CACHE_PAGE_PATH", szPathPage))
			return -1;
		if(cConfig.GetFieldValue("CACHE_BLOCK_NUM", lFileNumBlock))
			return -1;
		if(cConfig.GetFieldValue("CACHE_BLOCK_PATH", szPathBlock))
			return -1;
		if(cConfig.GetFieldValue("CACHE_N_TAB_NUM", lFileNumTabNReg))
			return -1;
		if(cConfig.GetFieldValue("CACHE_N_TAB_PATH", szPathTabNReg))
			return -1;
		if(cConfig.GetFieldValue("CACHE_R_TAB_NUM", lFileNumTabReg))
			return -1;
		if(cConfig.GetFieldValue("CACHE_R_TAB_PATH", szPathTabReg))
			return -1;
		// 用户信息
		if(cConfig.GetFieldValue("CACHE_USER_NAME", szusername))
			return -1;
		if(cConfig.GetFieldValue("CACHE_USER_PWD", szuserpwd))
			return -1;
		if(cConfig.GetFieldValue("CACHE_DB_NAME", szdbname))
			return -1;
 		// Data Info
		if(cConfig.GetFieldValue("SEND_THREAD_NUM_DATA", lDataThreadNum))
			return -1;
		if(cConfig.GetFieldValue("SEND_PORT_DATA", usDataSendPort))
			return -1;
		if(cConfig.GetFieldValue("SEND_MAC_NUM_DATA", lDataMacNum))
			return -1;
		if(lDataMacNum > MAX_MACHINE_NUM)
			return -1;		
		for(long j = 0; j < lDataMacNum; j++)
		{
			sprintf(configname, "MAC_IP_DATA_%.2d", j);
			if(cConfig.GetFieldValue(configname, szaDataMacIP[j]))
				return -1;
		}
		// 建立路径
		if(MakePath(szPathPage) < 0)
			return -1;
		if(MakePath(szPathBlock) < 0)
			return -1;
		if(MakePath(szPathTabReg) < 0)
			return -1;
		if(MakePath(szPathTabNReg) < 0)
			return -1;
		return 0;
	}
	long MakePath(char* path)
	{
		char* p = path, ch;
		while(*p)
		{
			if(*p != '\\' && *p != '/')
			{
				p++;
				continue;
			}
			ch = *p;
			*p = 0;
			if(access(path, 0) && mkdir(path))
			{
				*p = ch;
				return -1;
			}
			*p++ = ch;
		}
		if(access(path, 0) && mkdir(path))
			return -1;
		return 0;
	}
} NEWSCACHE_CONFIGINFO;
*/
#endif