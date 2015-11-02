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
#define _M_RW		(1<<3)	// 目前不支持
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

	//特别强调
	//mfs windows客户端文件路径以左斜线开始，非左斜线结束。比如说路径/a/b，最
	//终目的是b，/a/b/最终目的就是空

	////////////////////////////////////////////////////////////////////////
	// 环境设置函数
	////////////////////////////////////////////////////////////////////////

	// [功能] 初始化MFSClient环境
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_startup(const char* cfgfile);

	// [功能] 清除MFSClient环境
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_cleanup();

	////////////////////////////////////////////////////////////////////////
	// 文件读写操作函数
	////////////////////////////////////////////////////////////////////////

	// 目前mode仅支持：r/w/rb/wb/a/ab
	mfs_file* mfsclient_fopen(const char* path, const char* mode);

	// [功能] 关闭mfs_file指针
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_fclose(mfs_file* stream);

	// [功能] 重新设置缓冲区
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_setbuf(mfs_file* stream, char* buf, size_t size);

	// [功能] 刷新缓冲区
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_fflush(mfs_file* stream);

	// [功能] 写文件
	// [返回值] 写入文件数据个数
	size_t mfsclient_fwrite(const void* buffer, size_t size, size_t num, mfs_file* stream);

	// [功能] 模块化写文件
	// [返回值] 写入文件数据个数
	int mfsclient_fprintf(mfs_file *stream, const char *format, ...);

	// [功能] 写字符串到文件
	// [返回值] 写入文件数据个数
	int mfsclient_fputs (const char *string, mfs_file *stream);

	// [功能] 写字符到文件
	// [返回值] 写入字符
	int mfsclient_fputc (int ch, mfs_file *stream);

	// [功能] 改变文件大小
	// [返回值] 0: 表示正常，-1:表示出错
#if defined(WIN32)
	int mfsclient_ftruncate (mfs_file *stream, unsigned __int64 length);
#else
	int mfsclient_ftruncate (mfs_file *stream, uint64_t length);
#endif

	// [功能] 获取指定大小字符串
	// [返回值] 字符串地址
	char* mfsclient_fgets(char *string, int count, mfs_file *stream);

	// [功能] 获取一个字符
	// [返回值] 字符int值
	int mfsclient_fgetc(mfs_file *stream);

	// [功能] 读文件
	// [返回值] 读出的数据个数
	size_t mfsclient_fread(void *buffer, size_t size, size_t num, mfs_file *stream);

	// [功能] 改变文件指针位置
	// [返回值] 0: 表示正常，-1:表示出错
#if defined(WIN32)
	int mfsclient_fseek(mfs_file* stream, __int64 offset, int fromwhere);
#else
	int mfsclient_fseek(mfs_file* stream, int64_t offset, int fromwhere);
#endif

	// [功能] 获取文件游标位置
	// [返回值] 文件长度
#if defined(WIN32)
	__int64 mfsclient_ftell(mfs_file* stream);
#else
	int64_t mfsclient_ftell(mfs_file* stream);
#endif
	////////////////////////////////////////////////////////////////////////
	// 文件操作函数
	////////////////////////////////////////////////////////////////////////

	// [功能] 判断文件状态
	// 00 : 是否存在 02 : 可写 04 : 可读
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_access(const char* path, int mode);

	// [功能] 删除文件
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_remove(const char* path);

	// [功能] 改变文件名称
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_rename(const char* from_path, const char* to_path);

	// [功能] 获取文件长度
	// [返回值] 文件长度
#if defined(WIN32)
	__int64 mfsclient_filelength(mfs_file *stream);
#else
	int64_t mfsclient_filelength(mfs_file *stream);
#endif

	////////////////////////////////////////////////////////////////////////
	// 目录操作函数
	////////////////////////////////////////////////////////////////////////

	// [功能] 创建目录
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_mkdir(const char* path, int mode);

	// [功能] 删除目录
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_rmdir(const char* path);

	// [功能] 打开一个目录
	// [返回值] mfs_dir指针
	mfs_dir* mfsclient_opendir(const char *path);

	// [功能] 遍历目录
	// [返回值] mfs_dirent指针
	mfs_dirent* mfsclient_readdir(mfs_dir* dir);

	// [功能] 关闭目录指针
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_closedir(mfs_dir* dir);

	/////////////////////////////////////////////////////////////////////////
	// 文件统计操作函数
	////////////////////////////////////////////////////////////////////////

	// [功能] 获取文件属性mfs_stat
	// [返回值] 0: 表示正常，-1:表示出错
	int mfsclient_stat(const char *path, mfs_stat *st);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif // __MFSCLIENT_H__