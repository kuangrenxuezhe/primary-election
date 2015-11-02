/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <Types.h>
����ļ�     : 
�ļ�ʵ�ֹ��� : <> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : <���������ļ�>
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
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
		char  szpTptName[50];               //������
		char  szpUrl[256];                  //web_url
		char  szpBegin[256];                //����ͷ
		char  szpEnd[256];					//����β
		char  szaUrlFilter[50];             //url����
		char  szpInterBegin[256];			//���ͷ
		char  szpInterEnd[256];              //���Ϊ
		int   iPageTitle;				    //ҳ�����
		int   urlFlg;                       //url��ǩ
		int   exeFlg;                       //ִ�б�ʾ,ִ�еĴ���
		bool  interrelated;                 //�������
	} Template;
	
	typedef struct _URLNODE_STRUCT {
		char szpUrl[256];                   //url
		char szpPUrl[256];                  //pic url
		char szpTitle[256];                 //url_text
		Template* pTemplate;                //����
		_URLNODE_STRUCT* pNext;             //����ز�Ϊ��
		_URLNODE_STRUCT* pPrior;            //����ز�Ϊ��
		char* pContext;                     //����
		char szpNewsTime[30];               //����ʱ��
		bool  isHeader;                     //����ز�Ϊ��ʱ�ж��Ƿ�Ϊͷ
		unsigned long intNum;               //��غ� �������ݸ���
		int  urlFlg;                        //������ʾ 0,1,2,3
	} UrlNode;
#pragma pop()

#endif 
