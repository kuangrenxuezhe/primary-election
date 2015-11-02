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

#define MAX_MACHINE_NUM		64		// ��������	
#define MAX_BLOCK_NUM		80		// ҳ���������
#define MAX_ITEM_NUM		16		// һ��Rss�����item����
#define MAX_LIST_NUM		16		// һ��Item������б�����
#define MAX_DATA_SIZE		(1<<20)

#define WIN32COMMON
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned __int64 u_int64;

/*
�ֶ�˵��					��ʶ��		�ֶ�����		�ֶ�����	�ֶγ���
ҳ��ID						id			unsigned long	number		20
ҳ��title1					name1		char*			Varchar2	60
ҳ��title2					name2		char*			Varchar2	60
ҳ������					type		unsigned char	char		1
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
�ֶ�˵��														��ʶ��		�ֶ�����		�ֶ�����	�ֶγ���
��ID															id			unsigned long	number		20
�����															title		char*			Varchar2	60
��ͼ��															icon		char*			Varchar2	60
������															type		unsigned char	number		2
������css(�߿���ɫ���߿��Ƿ���ʾ�ͱ�����ɫ)					outstyle	char*			Varchar2	200
���������css(�������ı���ɫ���Ƿ���ʾ������)					tstyle		char*			Varchar2	200
��ͼ���Ƿ���ʾ              									ityle		char*			Varchar2	200
����������css(���壬�ֺţ�����ɫ���ֱ���ɫ���Ӵ֣�б��)		tfstyle		char*			Varchar2	200
��������������css												cfstyle		char*			Varchar2	200
���ģ���														mtemplet	unsigned char	number		10
�жϿ��Ƿ����õı�ʾ											set			char			char		1
��������														remind		char			char		1
��Ϣ����(�����֮���ÿո�ֿ�)									info		char*			Varchar2	100
�������url														url			char*			Varchar2	200
���Դ����url(ֻ����ԭζ��������ֵ)								burl		char*			Varchar2	200
��Ĺؼ���														keyword		char*			Varchar2	60
�����ʾ����													num			unsigned short	number		2
��������(���ţ���̳)											infotype	char			number		2
������Դ(���ѣ��ٶ�)											server		char*			Varchar2	60
�ж��Ƿ��û�ָ���ı���ı�ʾ									istitle		char			char		1
������Ԥ�����б�ʾʡ�ݣ����֣��ڵ���Ԥ�����б�ʾ����������		pr			char*			Varchar2	60
������Ԥ�����б�ʾ�������������ڵ���Ԥ�����б�ʾ��̨��CCTV-5	ct			char*			Varchar2	60
"����"���û��ɸ����֡�											more        char*           Varchar2    60
"���������"���û�Ҳ�ɸ����ӵ�ַ��                              mlink       char*           Varchar2    200
"�Ƿ���ʾ����"���û�����Ϊ��ʾ����ʾ��                        ismore      char            char        1
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
�ֶ�˵��					��ʶ��		�ֶ�����		�ֶ�����	�ֶγ���
ҳ��ID						id			unsigned long	number		20
ҳ��title1					name1		char*			Varchar2	60
ҳ��title2					name2		char*			Varchar2	60
ҳ������					type		unsigned char	char		1
ҳ����						start		char*			Varchar2	120
�ؼ���						keyword		char*			Varchar2	60
ҳcss��ַ(����/css/xxx.css)	stylepath	char*			Varchar2	200
ҳ��url						url			char*			Varchar2	200
���չ�ע��ı�ʾ			fg			char			char		1
���Ƿ�ռ(1��ռ,0����ռ)   isoverlap   bool            char        1
ҳ�ṹ						module		char*			Varchar2	600
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
	// ��׼�ṹ
	OVERLAPPED cOverlapped;
	SOCKET sClient;	
	char szaBuffer[IPADDRESSLEN<<1];
	char szDealType;

	WSABUF wsaBuf;

	// ����˽��ջ�����
	char* szpRecvBuf;
	long  lRecvFinLen;
	long  lRecvRemLen;

	// ����˷��ͻ�����
	char* szpSendBuf;
	long  lSendFinLen;
	long  lSendRemLen;

	// ��������
	long lSearchCount;
} EXPRESS_OVERLAPPED;

typedef struct D2_Data_Node
{
	u_int64 doc;		// DocID�����б�
	u_int64 site;		// SitID�����б�
	u_int64 company;	// ��˾�������б�	
	u_int64 source;		// ��ԴID�б�
	unsigned long time;	// ʱ���б�
	float value;		// �۸��б�
	float power;		// Ȩֵ�б�
	long good;			// ����ָ���б�
}D2_RESULT;

typedef struct _NewsCache_ConfigInfo_
{
	long lOverLappedNum;       // �ص��ṹ����

	long lSendBufSize;         // ���ͻ�������С
	long lRecvBufSize;         // ���ջ�������С
	long lRecvThreadNum;       // �����߳���
	unsigned short usRecvPort; // ���ն˿ں�

	u_short usCacheAddPort;		// Cache ���ض˿�
	long lFileNumPage;			// ҳCache�ļ�����
	char szPathPage[128];		// ҳCache�ļ�·��
	long lFileNumBlock;			// ��Cache�ļ�����
	char szPathBlock[128];		// ��Cache�ļ�·��
	long lFileNumTabReg;		// ע���û�ҳ��ǩ
	char szPathTabReg[128];		// 
	long lFileNumTabNReg;		// ��ע���û�ҳ��ǩ
	char szPathTabNReg[128];
	char szusername[128];		// �û���
	char szuserpwd[128];		// ����
	char szdbname[128];			// ���ݿ���

	long lDataMacNum;		    // ���ͻ�������
	u_short usDataSendPort;		// ���Ͷ˿ں�
	long lDataThreadNum;		// �����߳���(Pre IP)
	char szaDataMacIP[MAX_MACHINE_NUM][16];	// ���ͻ���IP

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
		// �û���Ϣ
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
		// ����·��
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