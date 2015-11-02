/*
// =====================================================================================
// 
//       Filename:  res_register.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  05/08/2013 09:31:27 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  Elwin.Gao (elwin), elwin.gao4444@gmail.com
//        Company:  
// 
// =====================================================================================
*/

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#if defined(__linux)
#include <sys/stat.h>
#include <pthread.h>
#endif	// defined(__linux)

#if defined(WIN32)
#include <windows.h>
#include <atlbase.h>
#endif	// defined(WIN32)


// =====================================================================================
//        Class:  ResRegister
//  Description:  
// =====================================================================================
class ResRegister
{
public:
	// ====================  LIFECYCLE     =======================================
	ResRegister(){}
	~ResRegister(){}

	// ====================  INTERFACE     =======================================
	//
	/* 
	// ===  FUNCTION  ======================================================================
	//         Name:  register_resource
	//  Description:  向系统中注册全局资源，多次调用接口不可为一个资源重复注册
	//   Parameters:  count: 注册的资源个数
	//                ...: 每个需要注册的资源都是一个字符串
	//                注意; 只有合法的文件名或路径名才可以作为资源标识符
	//  ReturnValue:  成功占用资源返回0，否则返回-1
	// =====================================================================================
	*/
	int register_resource(int count, ...)
	{
		m_file_list_len = count;
		m_file_list = new(std::nothrow) char*[m_file_list_len];
		if (m_file_list == NULL) {
			perror("ResRegister::register_resource new file_list");
			return -1;
		}

		va_list args;
		va_start(args, count);

		for (int i=0; i<m_file_list_len; ++i) {
			char *arg = va_arg(args, char*);
			m_file_list[i] = register_one_resource(arg);
			if (m_file_list[i] == NULL) {
				puts("ResRegister::register_resource exist, can't register again");
				return -1;
			}
		}
		va_end(args);

#if defined(__linux)
		m_touch_res = true;

		if (pthread_create(&m_tid_touch, NULL, pthread_touch, this) < 0) {
			perror("ResRegister::register_resource starting res heartbeat");
			return -1;
		}
#endif	// defined(__linux)

		return 0;
	}		// -----  end of function register_resource  -----
	/* 
	// ===  FUNCTION  ======================================================================
	//         Name:  unregister_resource
	//  Description:  从系统中注销全局资源
	//   Parameters:  null
	//  ReturnValue:  null
	// =====================================================================================
	*/
	void unregister_resource()
	{
#if defined(__linux)

		m_touch_res = false;
		pthread_join(m_tid_touch, NULL);

		for (int i=0; i<m_file_list_len; ++i) {
			delete m_file_list[i];
			m_file_list[i] = NULL;
		}
		delete m_file_list;
		m_file_list = NULL;

#endif	// defined(__linux)

#if defined(WIN32)

		for (int i=0; i<m_file_list_len; ++i) {
			wchar_t wname[512];
			MultiByteToWideChar(CP_ACP, 0, m_file_list[i], 6, wname, 512);

			HANDLE ghSemaphore = OpenSemaphore(
					DELETE,         // desired access
					FALSE,          // inherit handle
					wname);         // named semaphore
			if (ghSemaphore == NULL) 
			{
				printf("OpenSemaphore error: %d\n", GetLastError());
				return;
			}

			CloseHandle(ghSemaphore);
		}

#endif	// defined(WIN32)
	}		// -----  end of function unregister_resource  -----

private:
	// ==================== PRIVATE METHOD =======================================
	
	/* 
	// ===  FUNCTION  ======================================================================
	//         Name:  register_resource
	//  Description:  向系统中注册全局资源
	//   Parameters:  只有合法的文件名或路径名才可以作为资源标识符
	//  ReturnValue:  成功注册返回注册文件的文件名，否则返回NULL
	// =====================================================================================
	*/
	char* register_one_resource(const char* identifier)
	{
#if defined(__linux)

		char *filename = new(std::nothrow)char[strlen(identifier)+1+18];
		char *p = filename + 18;

		sprintf(filename, "/dev/shm/uniq_res.%s", identifier);

		while ((p=strchr(p, '/')) != NULL)
			*p++ = '_';

		if (access(filename, F_OK) < 0)
			if (touch_file(filename) < 0)
				return NULL;

		struct stat st_buf;
		if (stat(filename, &st_buf) < 0) {
			perror("ResRegister::register_resource get file stat");
			return NULL;
		}

		if (time(NULL) - st_buf.st_mtime < 6)
			return NULL;
		// 这里有一段及短的时间窗口，有可能导致资源被两个进程同时注册
		touch_file(filename);

		return filename;

#endif	// defined(__linux)

#if defined(WIN32)

		char *filename = new(std::nothrow)char[strlen(identifier)+1];
		char *p = filename;

		strcpy(filename, identifier);

		while ((p=strchr(p, '/')) != NULL)
			*p++ = '_';

		HANDLE ghSemaphore;

		wchar_t wname[512];
		MultiByteToWideChar(CP_ACP, 0, identifier, 6, wname, 512);

		ghSemaphore = OpenSemaphore(
				SEMAPHORE_ALL_ACCESS,   // desired access
				FALSE,          // inherit handle
				wname);         // named semaphore
		if (ghSemaphore != NULL) 
			return NULL;

		ghSemaphore = CreateSemaphore( 
				NULL,           // default security attributes
				1,              // initial count
				1,              // maximum count
				wname);         // named semaphore
		if (ghSemaphore == NULL) 
		{
			printf("CreateSemaphore error: %d\n", GetLastError());
			return NULL;
		}

		return filename;

#endif	// defined(WIN32)

        return NULL;
	}		// -----  end of function register_resource  -----

	/* 
	// ===  FUNCTION  ======================================================================
	//         Name:  touch_file
	//  Description:  liek touch command 
	// =====================================================================================
	*/
	int touch_file(const char* filename)
	{
		FILE* fp = fopen(filename, "wb");	// just change mtile & ctime, but atime
		if (fp == NULL) {
			perror("ResRegister::touch_file create new file");
			return -1;
		}
		fclose(fp);

		return 0;
	}		// -----  end of function touch_file  -----

	/* 
	// ===  FUNCTION  ======================================================================
	//         Name:  pthread_touch
	//  Description:  每个5秒，刷新文件，保证持续占有资源
	// =====================================================================================
	*/
#if defined(__linux)
	static void* pthread_touch(void* arg)
	{
		ResRegister* res = (ResRegister*)arg;
		while (res->m_touch_res) {
			for (int i=0; i<res->m_file_list_len; ++i)
				res->touch_file(res->m_file_list[i]);
			sleep(5);
		}
		return NULL;
	}		// -----  end of static function pthread_touch  -----
#endif	// defined(__linux)

	// ====================  DATA MEMBERS  =======================================
	char** m_file_list;
	int m_file_list_len;
	bool m_touch_res;
#if defined(__linux)
	pthread_t m_tid_touch;
#endif	// defined(__linux)

};		// -----  end of class ResRegister  -----

/* 
// ===  FUNCTION  ======================================================================
//         Name:  main
//  Description:  
// =====================================================================================
*/
/*
int main(int argc, char *argv[])
{
#if defined(__linux)
	puts("hello linux");
#endif

#if defined(WIN32)
	puts("hello windows");
#endif
	ResRegister res;
	res.register_resource(2, "hello", "world");
	puts("press entry to continue...");
	getchar();
	res.unregister_resource();

	return EXIT_SUCCESS;
}				// ----------  end of function main  ----------
*/
