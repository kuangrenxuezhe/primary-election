// UC_FileTransfers_Client.h

#ifndef _UC_FILE_TRANSFERS_CLIENT_H_
#define _UC_FILE_TRANSFERS_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <io.h>
#include <errno.h>
#include <sys/stat.h>

#include "./stable/UC_Communication.h"

typedef struct _File_Handle_
{
	FILE* fp;
	SOCKET sock;
	long f_type;
	long o_type;
	char filename[512];
	// cache
	char* buffer;
	char* buf_pos;
	long buf_size;
	long buf_free;
} FH;

class UC_FileTransfers_Client
{
public:
	UC_FileTransfers_Client()
	{
	}

	~UC_FileTransfers_Client()
	{
		UC::ClearSocket();
	}

	long InitSystem()
	{
		if(UC::InitSocket())
			return -1;
		return 0;
	}

	long AccessFile(char* in_readip, char* in_localip, char* in_path, unsigned short in_port = 0)
	{	
		char filename[512];
		if(GetFullFileName(in_readip, in_localip, in_path, filename))
			return -1;
		return (long)ControlFile(filename, "ACCESS", in_port);
	}

	long DeleteFile(char* in_readip, char* in_localip, char* in_path, unsigned short in_port = 0)
	{
		char filename[512];
		if(GetFullFileName(in_readip, in_localip, in_path, filename))
			return -1;
		return (long)ControlFile(filename, "DELETE", in_port);
	}

	long GetFileSize(char* in_readip, char* in_localip, char* in_path, unsigned short in_port = 0)
	{
		char filename[512];
		if(GetFullFileName(in_readip, in_localip, in_path, filename))
			return -1;
		return (long)ControlFile(filename, "SIZE", in_port);		
	}

	__int64 GetFileSize_i64(char* in_readip, char* in_localip, char* in_path, unsigned short in_port = 0)
	{
		char filename[512];
		if(GetFullFileName(in_readip, in_localip, in_path, filename))
			return -1;
		return ControlFile(filename, "SIZE", in_port);		
	}	

	FH* OpenFile(char* in_readip, char* in_localip, char* in_path, char in_opentype, long in_buflen = 4096, unsigned short in_port = 0)
	{
		char filename[300];
		if(strcmp(in_readip, in_localip) == 0)
			strcpy(filename, in_path);
		else
		{
			sprintf(filename, "\\\\%s\\%s", in_readip, in_path);
			char* p = strchr(filename, ':');
			if(p == NULL)
				return NULL;
			*p = '$';
		}

		return OpenFile(filename, in_opentype, in_buflen, in_port);
	}

	FH* OpenFile(char* in_filename, char in_opentype, long in_buflen = 4096, unsigned short in_port = 0)
	{
		if(in_opentype != 'w' && in_opentype != 'r')
			return NULL;
		
		if(in_buflen <= 0)
			in_buflen = 4096;
	
		FH* cpRet = NULL;

		char tempname[300];
		strcpy(tempname, in_filename);

		if((tempname[0] == '\\' && tempname[1] == '\\') || (tempname[0] == '/' && tempname[1] == '/'))
		{
			char* p1 = tempname;
			if(p1[0] != '\\' || p1[1] != '\\')
				return NULL;
			char* p2 = p1 + 2;
			p1 = strchr(p2, '\\');
			*p1 = 0;
			SOCKET sClient;
			if(UC::ConnectServer(p2, in_port, sClient))
				return NULL;
			*p1 = '\\';
			p1[2] = ':';

			char filename[300];
			*(long*)filename = 4;
			*(long*)filename += sprintf(filename + 4, "%c%s", toupper(in_opentype), p1 + 1);

			if(UC::SendBuffer(sClient, filename, *(long*)filename))
			{
				UC::CloseSocket(sClient);
				return NULL;
			}
			if(UC::RecvBuffer(sClient, filename, 2))
			{
				UC::CloseSocket(sClient);
				return NULL;
			}
			if(filename[0] == 'N' && filename[1] == 'O')
			{
				UC::CloseSocket(sClient);
				return NULL;
			}
			if(filename[0] != 'O' || filename[1] != 'K')
			{
				UC::CloseSocket(sClient);
				return NULL;
			}
			
			cpRet = new FH;
			if(cpRet == NULL)
			{
				UC::CloseSocket(sClient);
				return NULL;
			}

			cpRet->buffer = new char[in_buflen];
			if(cpRet->buffer == NULL)
			{
				UC::CloseSocket(sClient);
				return NULL;
			}
			cpRet->buf_size = in_buflen;

			cpRet->fp = NULL;
			cpRet->sock = sClient;
			cpRet->f_type = 0;
			if(in_opentype == 'w')
			{
				cpRet->o_type = 0;
				cpRet->buf_free = in_buflen;
				cpRet->buf_pos = cpRet->buffer;
			}
			else
			{
				cpRet->o_type = 1;
				cpRet->buf_free = 0;
				cpRet->buf_pos = cpRet->buffer + in_buflen;
			}
		}
		else
		{
			cpRet = new FH;
			if(cpRet == NULL)
				return NULL;
			FILE* fp = NULL;
			if(in_opentype == 'w')
			{
				fp = fopen(tempname, "wb");
				cpRet->o_type = 0;
			}
			else
			{
				fp = fopen(tempname, "rb");
				cpRet->o_type = 1;
			}
			if(fp == NULL)
			{
				delete cpRet;
				return NULL;
			}
			if(in_buflen != 4096)
				setvbuf(fp, NULL, _IOFBF, in_buflen);
			cpRet->buffer = NULL;
			cpRet->buf_size = in_buflen;
			cpRet->buf_free = 0;
			cpRet->buf_pos = NULL;

			cpRet->fp = fp;
			cpRet->sock = NULL;
			cpRet->f_type = 1;
		}

		strcpy(cpRet->filename, in_filename);

		return cpRet;
	}

	// 返回读出的长度 -1 错误 
	long ReadFile(char* buffer, long buflen, FH* fh)
	{
		if(fh->o_type == 0)
			return -1;
		if(fh->f_type == 1)
			return fread(buffer, 1, buflen, fh->fp);
		if(fh->buf_free >= buflen)
		{
			memcpy(buffer, fh->buf_pos, buflen);
			fh->buf_pos += buflen;
			fh->buf_free -= buflen;
			return buflen;
		}
		else
		{
			long finlen = fh->buf_free;
			memcpy(buffer, fh->buf_pos, fh->buf_free);
			fh->buf_pos += fh->buf_free;
			fh->buf_free = 0;
			
			if(buflen - finlen >= fh->buf_size)
			{
				long calclen = (buflen - finlen)/fh->buf_size*fh->buf_size;
				long commbuf[2];
				commbuf[0] = 1;
				commbuf[1] = calclen;
				if(UC::SendBuffer(fh->sock, (char*)commbuf, 8))
					return -1;
				long recvlen = 0;
				for(;;)
				{
					if(UC::RecvBuffer(fh->sock, (char*)commbuf, 8))
						return -1;
					if(UC::RecvBuffer(fh->sock, buffer + finlen + recvlen, commbuf[1]))
						return -1;
					recvlen += commbuf[1];
					if(commbuf[0] != commbuf[1] || recvlen == calclen)
						break;
				}
				finlen += recvlen;
				if(calclen != recvlen || buflen == finlen)
					return finlen;
			}

			long commbuf[2];
			commbuf[0] = 1;
			commbuf[1] = fh->buf_size;
			if(UC::SendBuffer(fh->sock, (char*)commbuf, 8))
				return -1;
			long recvlen = 0;
			for(;;)
			{
				if(UC::RecvBuffer(fh->sock, (char*)commbuf, 8))
					return -1;
				if(UC::RecvBuffer(fh->sock, fh->buffer + recvlen, commbuf[1]))
					return -1;
				recvlen += commbuf[1];
				if(commbuf[0] != commbuf[1] || recvlen == fh->buf_size)
					break;
			}
			fh->buf_free = recvlen;
			fh->buf_pos = fh->buffer;					
				
			long copylen = fh->buf_free > buflen - finlen ? buflen - finlen : fh->buf_free;
			memcpy(buffer + finlen, fh->buf_pos, copylen);
			fh->buf_pos += copylen;
			fh->buf_free -= copylen;
			finlen += copylen;

			return finlen;
		}

		return 0;
	}
	
	long WriteFile(char* buffer, long buflen, FH* fh)
	{
		if(fh->o_type == 1)
			return -1;
		if(fh->f_type == 1)
		{
			if(fwrite(buffer, 1, buflen, fh->fp) != (unsigned)buflen)
				return -1;
			return buflen;
		}
		if(fh->buf_free >= buflen)
		{
			memcpy(fh->buf_pos, buffer, buflen);
			fh->buf_pos += buflen;
			fh->buf_free -= buflen;
			return buflen;
		}
		else
		{
			if(fh->buf_free != fh->buf_size)
			{
				long commbuf[2];
				commbuf[0] = 1;
				commbuf[1] = fh->buf_size - fh->buf_free;
				if(UC::SendBuffer(fh->sock, (char*)commbuf, 8))
					return -1;
				if(UC::SendBuffer(fh->sock, fh->buffer, fh->buf_size - fh->buf_free))
					return -1;
				fh->buf_pos = fh->buffer;
				fh->buf_free = fh->buf_size;		
				if(UC::RecvBuffer(fh->sock, (char*)commbuf, 8))
					return -1;
				if(memcmp(commbuf, "OKOKOKOK", 8))
					return -1;
			}

			long commbuf[2];
			commbuf[0] = 1;
			commbuf[1] = buflen;
			if(UC::SendBuffer(fh->sock, (char*)commbuf, 8))
				return -1;
			if(UC::SendBuffer(fh->sock, buffer, buflen))
				return -1;
			if(UC::RecvBuffer(fh->sock, (char*)commbuf, 8))
				return -1;
			if(memcmp(commbuf, "OKOKOKOK", 8))
				return -1;

			return buflen;
		}

		return 0;
	}

	long CloseFile(FH*& fh)
	{
		if(fh->f_type == 0)
		{
			long commbuf[2];
			// flush cache
			if(fh->o_type == 0 && fh->buf_size != fh->buf_free)
			{
				commbuf[0] = 1;
				commbuf[1] = fh->buf_size - fh->buf_free;
				if(UC::SendBuffer(fh->sock, (char*)commbuf, 8))
					return -1;
				if(UC::SendBuffer(fh->sock, fh->buffer, fh->buf_size - fh->buf_free))
					return -1;
				if(UC::RecvBuffer(fh->sock, (char*)commbuf, 8))
					return -1;
				if(memcmp(commbuf, "OKOKOKOK", 8))
					return -1;
			}
			//	
			commbuf[0] = 0;
			if(UC::SendBuffer(fh->sock, (char*)commbuf, 8))
				return -1;
			UC::CloseSocket(fh->sock);
			delete fh->buffer;
		}
		else
			fclose(fh->fp);

		delete fh;
		fh = NULL;

		return 0;
	}

private:
	long GetFullFileName(char* in_readip, char* in_localip, char* in_path, char* filename)
	{
		if(strcmp(in_readip, in_localip) == 0)
			strcpy(filename, in_path);
		else
		{
			sprintf(filename, "\\\\%s\\%s", in_readip, in_path);
			char* p = strchr(filename, ':');
			if(p == NULL)
				return -1;
			*p = '$';
		}

		return 0;
	}

	__int64 ControlFile(char* in_filename, char* in_controltype, unsigned short in_port)
	{
		char tempname[300];
		strcpy(tempname, in_filename);

		if((tempname[0] == '\\' && tempname[1] == '\\') || (tempname[0] == '/' && tempname[1] == '/'))
		{
			char* p1 = tempname;
			if(p1[0] != '\\' || p1[1] != '\\')
				return -1;
			char* p2 = p1 + 2;
			p1 = strchr(p2, '\\');
			*p1 = 0;
			SOCKET sClient;
			if(UC::ConnectServer(p2, in_port, sClient))
				return -1;
			*p1 = '\\';
			p1[2] = ':';

			char filename[300];
			*(long*)filename = 4;
			if(strcmp(in_controltype, "ACCESS") == 0)
				*(long*)filename += sprintf(filename + 4, "C-A:%s", p1 + 1);
			else if(strcmp(in_controltype, "DELETE") == 0)
				*(long*)filename += sprintf(filename + 4, "C-D:%s", p1 + 1);
			else if(strcmp(in_controltype, "SIZE") == 0)
				*(long*)filename += sprintf(filename + 4, "C-S:%s", p1 + 1);
			else
			{
				UC::CloseSocket(sClient);
				return -1;
			}

			if(UC::SendBuffer(sClient, filename, *(long*)filename))
			{
				UC::CloseSocket(sClient);
				return -1;
			}
			if(UC::RecvBuffer(sClient, filename, 12))
			{
				UC::CloseSocket(sClient);
				return -1;
			}
			UC::CloseSocket(sClient);

			if(*(long*)filename != *(long*)"C-A:" && *(long*)filename != *(long*)"C-D:" && *(long*)filename != *(long*)"C-S:")
				return -1;

			return *(__int64*)(filename + 4);
		}
		else
		{
			// 文件存在    1
			// 文件不存在  0
			// 函数错误   -1
			if(strcmp(in_controltype, "ACCESS") == 0)
			{
				int ret = access(in_filename, 0);
				if(ret == 0)
					return 1;
				if(errno == ENOENT)
					return 0;
				return -1;
			}
			// 删除成功    1
			// 文件不存在  0
			// 函数错误   -1
			else if(strcmp(in_controltype, "DELETE") == 0)
			{
				int ret = remove(in_filename);
				if(ret == 0)
					return 1;
				if(errno == ENOENT)
					return 1;
				return -1;
			}
			// 成功返回 长度
			// 失败返回   -1
			else if(strcmp(in_controltype, "SIZE") == 0)
			{
				struct _stati64 cInfo;
				if(_stati64(in_filename, &cInfo))
					return -1;
				return cInfo.st_size;
			}
			else
				return -1;
		}

		return -1;
	}
};

#endif // _UC_FILE_TRANSFERS_CLIENT_H_