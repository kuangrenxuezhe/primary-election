#ifndef __MFSCLIENT_H__
#define __MFSCLIENT_H__

#include <stdio.h>
#if defined(WIN32)
	#include <winsock2.h>
	#include <sys/stat.h>

	#ifndef S_IFMT
	#   ifdef _S_IFMT
	#   define S_IFMT _S_IFMT
	#   else
	#   define S_IFMT 0170000
	#   endif
	#endif

	#ifndef S_ISDIR
	#   define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
	#endif

	#ifndef S_ISCHR
	#   define S_ISCHR(m) ((m & S_IFMT) == S_IFCHR)
	#endif

	#ifndef S_ISBLK
	#   ifdef S_IFBLK
	#   define S_ISBLK(m) ((m & S_IFMT) == S_IFBLK)
	#   else
	#   define S_ISBLK(m) (0)
	#   endif
	#endif

	#ifndef S_ISREG
	#   define S_ISREG(m) ((m & S_IFMT) == S_IFREG)
	#endif

	#ifndef S_ISFIFO
	#   ifdef S_IFIFO
	#   define S_ISFIFO(m) ((m & S_IFMT) == S_IFIFO)
	#   else
	#   define S_ISFIFO(m) (0)
	#   endif
	#endif

	#ifndef S_ISLNK
	#   ifdef _S_ISLNK
	#   define S_ISLNK(m) _S_ISLNK(m)
	#   else
	#   ifdef _S_IFLNK
	#       define S_ISLNK(m) ((m & S_IFMT) == _S_IFLNK)
	#   else
	#       ifdef S_IFLNK
	#       define S_ISLNK(m) ((m & S_IFMT) == S_IFLNK)
	#       else
	#       define S_ISLNK(m) (0)
	#       endif
	#   endif
	#   endif
	#endif

	#ifndef S_ISSOCK
	#   ifdef _S_ISSOCK
	#   define S_ISSOCK(m) _S_ISSOCK(m)
	#   else
	#   ifdef _S_IFSOCK
	#       define S_ISSOCK(m) ((m & S_IFMT) == _S_IFSOCK)
	#   else
	#       ifdef S_IFSOCK
	#       define S_ISSOCK(m) ((m & S_IFMT) == S_IFSOCK)
	#       else
	#       define S_ISSOCK(m) (0)
	#       endif
	#   endif
	#   endif
	#endif
#else
	#include <inttypes.h>
	#include <unistd.h>
#endif 

#define _M_READ		(1<<1)
#define _M_WRITE	(1<<2)
#define _M_RW		(1<<3)	// Ŀǰ��֧��
#define _M_APPEND	(1<<4)
#define _M_TEXT		(1<<5)
#define _M_BINARY	(1<<6)
#define _M_EOF		(1<<7)
#define _M_ERR		(1<<8)
#define _M_YOURBUF  (1<<9) 
#define _M_MYBUF    (1<<10)
#define _M_NOBUF    (1<<11)

/* General use macros */
#define M_ANYBUF(s)		((s)->_flag & (_M_MYBUF|_M_NOBUF|_M_YOURBUF))
#define M_NOBUF(s)      ((s)->_flag & _M_NOBUF)
#define M_MYBUF(s)      ((s)->_flag & _M_MYBUF)
#define M_YOURBUF(s)    ((s)->_flag & _M_YOURBUF)
#define M_HASBUF(s)     ((s)->_flag & (_M_MYBUF|_M_YOURBUF))

#define M_ISDIR(a)			(a==S_IFDIR)
#define M_ISFILE(a)			(a==S_IFREG)


typedef struct
{
	_dev_t     st_dev;
	_ino_t     st_ino;
	unsigned short st_mode;
	short      st_nlink;
	short      st_uid;
	short      st_gid;
	_dev_t     st_rdev;
	__int64    st_size;
	__int64    st_atime;
	__int64    st_mtime;
	__int64	   st_ctime;
}mfs_stat;

typedef struct
{
	char *_ptr;
	int	_cnt;
	char *_base;
	int	_bufsiz;
	int	_flag;
	int	_charbuf;
	void *_file;
	char *_name;
#if defined(WIN32)
	__int64 _offset;
#else
	int64_t _offset;
#endif
}mfs_file;

typedef struct
{
	int _ino;
	char _name[300];
	mfs_stat _st;
}mfs_dirent;

typedef struct
{
	int _ino;
	void* _dirbuf;
	char* _name;
	mfs_dirent _dirent;
}mfs_dir;

#ifdef __cplusplus
extern "C" 
{
#endif

	//�ر�ǿ��
	//mfs windows�ͻ����ļ�·������б�߿�ʼ������б�߽���������˵·��/a/b����
	//��Ŀ����b��/a/b/����Ŀ�ľ��ǿ�

	////////////////////////////////////////////////////////////////////////
	// �������ú���
	////////////////////////////////////////////////////////////////////////

	// [����] ��ʼ��MFSClient����
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_startup(const char* cfgfile);

	// [����] ���MFSClient����
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_cleanup();

	////////////////////////////////////////////////////////////////////////
	// �ļ���д��������
	////////////////////////////////////////////////////////////////////////

	// Ŀǰmode��֧�֣�r/w/rb/wb/a/ab
	mfs_file* mfsclient_fopen(const char* path, const char* mode);

	// [����] �ر�mfs_fileָ��
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_fclose(mfs_file* stream);

	// [����] �������û�����
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_setbuf(mfs_file* stream, char* buf, size_t size);

	// [����] ˢ�»�����
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_fflush(mfs_file* stream);

	// [����] д�ļ�
	// [����ֵ] д���ļ����ݸ���
	size_t mfsclient_fwrite(const void* buffer, size_t size, size_t num, mfs_file* stream);

	// [����] ģ�黯д�ļ�
	// [����ֵ] д���ļ����ݸ���
	int mfsclient_fprintf(mfs_file *stream, const char *format, ...);

	// [����] д�ַ������ļ�
	// [����ֵ] д���ļ����ݸ���
	int mfsclient_fputs (const char *string, mfs_file *stream);

	// [����] д�ַ����ļ�
	// [����ֵ] д���ַ�
	int mfsclient_fputc (int ch, mfs_file *stream);

	// [����] �ı��ļ���С
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
#if defined(WIN32)
	int mfsclient_ftruncate (mfs_file *stream, unsigned __int64 length);
#else
	int mfsclient_ftruncate (mfs_file *stream, uint64_t length);
#endif

	// [����] ��ȡָ����С�ַ���
	// [����ֵ] �ַ�����ַ
	char* mfsclient_fgets(char *string, int count, mfs_file *stream);

	// [����] ��ȡһ���ַ�
	// [����ֵ] �ַ�intֵ
	int mfsclient_fgetc(mfs_file *stream);

	// [����] ���ļ�
	// [����ֵ] ���������ݸ���
	size_t mfsclient_fread(void *buffer, size_t size, size_t num, mfs_file *stream);

	// [����] �ı��ļ�ָ��λ��
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
#if defined(WIN32)
	int mfsclient_fseek(mfs_file* stream, __int64 offset, int fromwhere);
#else
	int mfsclient_fseek(mfs_file* stream, int64_t offset, int fromwhere);
#endif

	// [����] ��ȡ�ļ��α�λ��
	// [����ֵ] �ļ�����
#if defined(WIN32)
	__int64 mfsclient_ftell(mfs_file* stream);
#else
	int64_t mfsclient_ftell(mfs_file* stream);
#endif
	////////////////////////////////////////////////////////////////////////
	// �ļ���������
	////////////////////////////////////////////////////////////////////////

	// [����] �ж��ļ�״̬
	// 00 : �Ƿ���� 02 : ��д 04 : �ɶ�
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_access(const char* path, int mode);

	// [����] ɾ���ļ�
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_remove(const char* path);

	// [����] �ı��ļ�����
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_rename(const char* from_path, const char* to_path);

	// [����] ��ȡ�ļ�����
	// [����ֵ] �ļ�����
#if defined(WIN32)
	__int64 mfsclient_filelength(mfs_file *stream);
#else
	int64_t mfsclient_filelength(mfs_file *stream);
#endif

	////////////////////////////////////////////////////////////////////////
	// Ŀ¼��������
	////////////////////////////////////////////////////////////////////////

	// [����] ����Ŀ¼
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_mkdir(const char* path, int mode);

	// [����] ɾ��Ŀ¼
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_rmdir(const char* path);

	// [����] ��һ��Ŀ¼
	// [����ֵ] mfs_dirָ��
	mfs_dir* mfsclient_opendir(const char *path);

	// [����] ����Ŀ¼
	// [����ֵ] mfs_direntָ��
	mfs_dirent* mfsclient_readdir(mfs_dir* dir);

	// [����] �ر�Ŀ¼ָ��
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_closedir(mfs_dir* dir);

	/////////////////////////////////////////////////////////////////////////
	// �ļ�ͳ�Ʋ�������
	////////////////////////////////////////////////////////////////////////

	// [����] ��ȡ�ļ�����mfs_stat
	// [����ֵ] 0: ��ʾ������-1:��ʾ����
	int mfsclient_stat(const char *path, mfs_stat *st);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif // __MFSCLIENT_H__