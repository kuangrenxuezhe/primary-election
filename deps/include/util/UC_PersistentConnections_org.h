// UC_PersistentConnections.h

#ifndef _UC_PERSISTENT_CONNECTIONS_H_
#define _UC_PERSISTENT_CONNECTIONS_H_

#include "UH_Define.h"
#include "UC_Journal.h"
#include "UT_HashSearch.h"
#include "UT_Queue.h"

typedef struct _pcs_socket_connect_
{
	var_u8                       uid_conn;
	CP_SOCKET_T                  sck_live;
	struct _pcs_socket_connect_* ptr_next;
} PCS_SOCKET_CONNECT;

typedef struct _pcs_access_point_
{
	var_4               sck_num_recv;
	PCS_SOCKET_CONNECT* sck_lst_recv;

	var_4               sck_num_send;
	PCS_SOCKET_CONNECT* sck_lst_send;
} PCS_ACCESS_POINT;

class UC_PersistentConnections_Server
{
public:
	UC_PersistentConnections_Server()
	{
	}
	~UC_PersistentConnections_Server()
	{
	}

	var_4 init_system(var_u2 listen_port, var_4 total_max_conn, var_4 single_max_conn, var_4 recv_buffer_size, var_4 recv_queue_size, var_4 send_queue_size)
	{
		m_listen_port = listen_port;

		m_total_max_conn = total_max_conn;
		m_single_max_conn = single_max_conn;

		m_recv_buffer_size = recv_buffer_size + 32;

		m_recv_queue_size = recv_queue_size;
		m_send_queue_size = send_queue_size;

		if(cp_init_socket())
			return -1;

		if(cp_listen_socket(m_listen_sock, m_listen_port))
			return -1;

		if(m_conn_loop.InitHashSearch(m_total_max_conn, sizeof(var_vd*)))
			return -1;

		if(m_recv_queue.InitQueue(m_recv_queue_size))
			return -1;

		if(m_send_queue.InitQueue(m_send_queue_size))
			return -1;

		if(m_journal.init(JT_SCR, JL_LOW))
			return -1;

		if(cp_create_thread(listen_connect, this))
			return -1;
		
		m_journal.record(JL_LOW, "PCS - init system ok\n");

		return 0;
	}

	var_4 recv_buffer(var_vd*& handle, var_1*& buffer, var_4& buflen)
	{
		m_journal.record(JL_LOW, "PCS - recv called\n");

		// 4byte(^-^!) 8byte(uuid) 4byte(sn) 4byte(request length) xbyte(request buffer)
		buffer = m_recv_queue.PopData();
		handle = (var_vd*)buffer;
		buffer += 16;
		buflen = *(var_4*)buffer;
		buffer += 4;

		return 0;
	}

	var_4 send_buffer(var_vd*& handle, var_1* buffer, var_4 buflen)
	{
		m_journal.record(JL_LOW, "PCS - send called\n");

		if(handle == NULL)
			return -1;

		var_u8 uuid = *(var_u8*)((var_1*)handle + 4);

		PCS_SOCKET_CONNECT* sc = pop_send_connect(uuid);
		if(sc == NULL)
		{
			delete (var_1*)handle;
			handle = NULL;
			return -1;
		}
		
		// 4byte(^-^!) 8byte(uuid) 4byte(sn) 4byte(request length) xbyte(request buffer)
		var_1* head = (var_1*)handle;
		*(var_4*)(head + 16) = buflen;
				
		if(cp_sendbuf(sc->sck_live, head, 20))
		{
			cp_close_socket(sc->sck_live);
			delete sc;

			delete (var_1*)handle;
			handle = NULL;
			return -1;
		}
		if(cp_sendbuf(sc->sck_live, buffer, buflen))
		{
			cp_close_socket(sc->sck_live);
			delete sc;

			delete (var_1*)handle;
			handle = NULL;
			return -1;
		}

		delete (var_1*)handle;
		handle = NULL;

		if(push_send_connect(uuid, sc))
		{
			cp_close_socket(sc->sck_live);
			delete sc;
		}

		return 0;
	}

	PCS_SOCKET_CONNECT* pop_send_connect(var_u8 uuid)
	{
		var_vd* ret_buf = NULL;

		m_conn_lock.lock();
		if(m_conn_loop.SearchKey_FL(uuid, &ret_buf))
		{
			m_conn_lock.unlock();
			return NULL;
		}
		PCS_ACCESS_POINT* ap = *(PCS_ACCESS_POINT**)ret_buf;
		if(ap->sck_lst_send == NULL)
		{
			m_conn_lock.unlock();
			return NULL;
		}
		PCS_SOCKET_CONNECT* sc = ap->sck_lst_send;
		ap->sck_lst_send = sc->ptr_next;
		m_conn_lock.unlock();

		return sc;
	}

	var_4 push_send_connect(var_u8 uuid, PCS_SOCKET_CONNECT* sc)
	{
		var_vd* ret_buf = NULL;

		m_conn_lock.lock();
		if(m_conn_loop.SearchKey_FL(uuid, &ret_buf))
		{
			m_conn_lock.unlock();
			return -1;
		}
		PCS_ACCESS_POINT* ap = *(PCS_ACCESS_POINT**)ret_buf;
		sc->ptr_next = ap->sck_lst_send;
		ap->sck_lst_send = sc;
		m_conn_lock.unlock();

		return 0;
	}

	static CP_THREAD_T listen_connect(var_vd* arg)
	{
		// 取得类指针
		UC_PersistentConnections_Server* cc = (UC_PersistentConnections_Server*)arg;

		// 初始化客户socket
		struct sockaddr_in my_client_addr;
#ifdef _WIN32_ENV_
		var_4 length = sizeof(my_client_addr);
#else
		socklen_t length = sizeof(my_client_addr);
#endif

		//
		CP_SOCKET_T client_sock;
		var_1 recv_buf[32];

		for(;;)
		{
			client_sock = accept(cc->m_listen_sock, (struct sockaddr*)&my_client_addr, &length);
			if(client_sock < 0)
			{
				cc->m_journal.record(JL_LOW, "PCS - accept connect failure\n");
				
				continue;
			}
			cc->m_journal.record(JL_LOW, "PCS - accept connect success\n");

			var_4 flag = 1;
#ifdef _WIN32_ENV_
			setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (var_1*)&flag, sizeof(flag));
#else
			setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (var_vd*)&flag, sizeof(flag));
#endif

			if(cp_recvbuf(client_sock, recv_buf, 16))
			{
				cc->m_journal.record(JL_LOW, "PCS - recv protocol failure\n");

				cp_close_socket(client_sock);
				continue;
			}
			cc->m_journal.record(JL_LOW, "PCS - recv protocol success\n");

			// 4byte(^-^!) 8byte(uuid) 4byte(<<<< or >>>>)
			if(*(var_u4*)recv_buf != *(var_u4*)"^-^!")
			{
				cc->m_journal.record(JL_LOW, "PCS - protocol check failure\n");

				cp_close_socket(client_sock);
				continue;
			}
			cc->m_journal.record(JL_LOW, "PCS - protocol check success\n");

			var_u8 conn_tag = *(var_u8*)(recv_buf + 4);			
			var_4 is_type = 0; // recv
			if(*(var_u4*)(recv_buf + 12) == *(var_u4*)"<<<<")
			{
				is_type = 1;   // send
				cc->m_journal.record(JL_LOW, "PCS - type is send, tag code is "CP_PU64"\n", conn_tag);
			}
			else
				cc->m_journal.record(JL_LOW, "PCS - type is recv, tag code is "CP_PU64"\n", conn_tag);

			var_vd* ret_buf = NULL;
			PCS_ACCESS_POINT* ap = NULL;
			PCS_SOCKET_CONNECT* sc = NULL;			

			cc->m_conn_lock.lock();
			var_4 retval = cc->m_conn_loop.SearchKey_FL(conn_tag, &ret_buf);
			if(retval)
			{
				ap = new PCS_ACCESS_POINT;
				ap->sck_lst_recv = NULL;
				ap->sck_lst_send = NULL;
				ap->sck_num_recv = 0;
				ap->sck_num_send = 0;
			}
			else
				ap = *(PCS_ACCESS_POINT**)ret_buf;
/*
			if((is_type == 0 && ap->sck_num_recv >= cc->m_single_max_conn) || (is_type == 1 && ap->sck_num_send >= cc->m_single_max_conn))
			{
				if(retval)
					delete ap;
				cc->m_conn_lock.unlock();

				cp_close_socket(client_sock);
				continue;
			}
*/
			if(retval && cc->m_conn_loop.AddKey_FL(conn_tag, &ap))
			{
				delete ap;
				cc->m_conn_lock.unlock();

				cp_close_socket(client_sock);
				continue;
			}

			sc = new PCS_SOCKET_CONNECT;
			sc->sck_live = client_sock;
			sc->uid_conn = conn_tag;				

			if(is_type == 0)
			{
				ap->sck_num_recv++;

				cc->m_tmp_conn = sc;
				cc->m_tmp_flag = 1;
				while(cp_create_thread(thread_recv, (var_vd*)cc))
				{
					cp_sleep(5000);
					continue;
				}
				while(cc->m_tmp_flag)
					cp_sleep(100);
			}
			else
			{
				ap->sck_num_send++;

				sc->ptr_next = ap->sck_lst_send;
				ap->sck_lst_send = sc;
			}
			cc->m_conn_lock.unlock();

			if(is_type == 0)
				cc->m_journal.record(JL_LOW, "PCS - recv connect ok, tag "CP_PU64"\n", conn_tag);
			else
				cc->m_journal.record(JL_LOW, "PCS - send connect ok, tag "CP_PU64"\n", conn_tag);
		}

		return 0;
	}

	static CP_THREAD_T thread_recv(var_vd* arg)
	{
		UC_PersistentConnections_Server* cc = (UC_PersistentConnections_Server*)arg;

		PCS_SOCKET_CONNECT* sc = cc->m_tmp_conn;
		cc->m_tmp_conn = NULL;
		cc->m_tmp_flag = 0;

		cc->m_journal.record(JL_LOW, "PCS - recv thread is running\n");

		for(;;)
		{
			// 4byte(^-^!) 8byte(uuid) 4byte(sn) 4byte(request length) xbyte(request buffer)
			var_1* buffer = new var_1[cc->m_recv_buffer_size];		

			if(cp_recvbuf(sc->sck_live, buffer, 20))
			{
				delete buffer;
				break;
			}

			if(*(var_u4*)buffer != *(var_u4*)"^-^!")
			{
				delete buffer;
				break;
			}

			if(cp_recvbuf(sc->sck_live, buffer + 20, *(var_4*)(buffer + 16)))
			{
				delete buffer;
				break;
			}

			cc->m_journal.record(JL_LOW, "PCS - recv request is ok\n");

			cc->m_recv_queue.PushData(buffer);

			cc->m_journal.record(JL_LOW, "PCS - request push to queue ok\n");
		}

		cp_close_socket(sc->sck_live);
		delete sc;

		cc->m_journal.record(JL_LOW, "PCS - thread is exit\n");

		return 0;
	}

public:
	var_u2 m_listen_port;

	var_4 m_total_max_conn;
	var_4 m_single_max_conn;

	var_4 m_recv_buffer_size;

	var_4 m_recv_queue_size;
	var_4 m_send_queue_size;
	
	CP_SOCKET_T m_listen_sock;

	CP_MUTEXLOCK          m_conn_lock;
	UT_HashSearch<var_u8> m_conn_loop;

	UT_Queue<var_1*> m_recv_queue;
	UT_Queue<var_1*> m_send_queue;

	UC_Journal m_journal;

	var_4               m_tmp_flag;	
	PCS_SOCKET_CONNECT* m_tmp_conn;
};

typedef struct _pcc_socket_connect_
{
	CP_SOCKET_T                  sck_live;
	struct _pcc_socket_connect_* ptr_next;
} PCC_SOCKET_CONNECT;

typedef struct _pcc_live_task_
{
	var_4 task_busy;

	time_t task_time;
	var_4  task_flag;

	var_u4 task_id;

	var_1* recv_buf;
	var_4  recv_len;
} PCC_LIVE_TASK;

class UC_PersistentConnections_Client
{
public:
	UC_PersistentConnections_Client()
	{
	}
	~UC_PersistentConnections_Client()
	{
	}

	var_4 init_system(var_1* remote_ip, var_u2 remote_port, var_4 connect_num, var_4 time_out)
	{
		strcpy(m_remote_ip, remote_ip);
		m_remote_port = remote_port;

		m_connect_num = connect_num;
		m_connect_tag = cp_get_uuid();
				
		m_time_out = time_out;

		if(cp_init_socket())
			return -1;

		if(m_journal.init(JT_SCR, JL_LOW))
			return -1;

		m_live_id = 0;
		if(m_live_task.InitHashSearch(m_connect_num * 10, sizeof(var_vd*)))
			return -1;

		m_send_idle = 0;
		m_send_live = 0;
		m_send_loop = NULL;

		for(var_4 i = 0; i < m_connect_num; i++)
		{
			while(cp_create_thread(thread_recv, this))
			{
				m_journal.record(JL_LOW, "PCC - create recv thread failure, retry\n");

				cp_sleep(5000);
				continue;
			}
		}

		m_journal.record(JL_LOW, "PCC - init system ok\n");

		return 0;
	}

	var_4 send_buffer(var_vd*& handle, var_1* send_buf, var_4 send_len, var_1* recv_buf)
	{
		m_journal.record(JL_LOW, "PCC - send called\n");

		PCC_LIVE_TASK* lt = new PCC_LIVE_TASK;
		handle = (var_vd*)lt;

		lt->task_busy = 0;

		// recv
		lt->recv_buf = recv_buf;
		lt->task_flag = 0;
		lt->task_time = time(NULL);		

		m_live_lock.lock();
		lt->task_id = m_live_id++;
		if(m_live_task.AddKey_FL(lt->task_id, &lt))
			assert(0);
		m_live_lock.unlock();

		// send
		var_1 head_buf[32]; // 4byte(^-^!) 8byte(uuid) 4byte(sn) 4byte(request length) xbyte(request buffer)
		*(var_u4*)head_buf = *(var_u4*)"^-^!";
		*(var_u8*)(head_buf + 4) = m_connect_tag;
		*(var_u4*)(head_buf + 12) = lt->task_id;
		*(var_4*)(head_buf + 16) = send_len;

		PCC_SOCKET_CONNECT* sc = NULL;
		if(pop_send_loop(sc))
		{
			delete_live_task(lt);
			delete lt;
			return -1;
		}
				
		for(var_4 i = 0;; i++)
		{
			if(cp_sendbuf(sc->sck_live, head_buf, 20) == 0)
				break;
			
			drop_send_loop(sc);

			if(i == 0 && pop_send_loop(sc) == 0)
				continue;

			delete_live_task(lt);
			delete lt;
			return -1;
		}
		if(cp_sendbuf(sc->sck_live, send_buf, send_len))
		{
			drop_send_loop(sc);

			delete_live_task(lt);
			delete lt;
			return -1;
		}

		push_send_loop(sc);

		return 0;
	}

	var_4 recv_buffer(var_vd*& handle, var_1*& buffer, var_4& buflen)
	{
		m_journal.record(JL_LOW, "PCC - recv called\n");

		PCC_LIVE_TASK* lt = (PCC_LIVE_TASK*)handle;

		var_4 retval = 0;

		for(;;)
		{
			if(lt->task_flag == 1 || (time(NULL) - lt->task_time) > m_time_out)
			{
				delete_live_task(lt);

				handle = NULL;
				buffer = lt->recv_buf;
				buflen = lt->recv_len;

				if(lt->task_flag != 1)
					retval = -1;
			
				while(lt->task_busy)
					cp_sleep(1);
				delete lt;

				break;				
			}
			cp_sleep(10);
		}

		return retval;
	}

	var_4 pop_send_loop(PCC_SOCKET_CONNECT*& sc)
	{
		m_send_lock.lock();

		if(m_send_live < m_connect_num)
		{
			sc = new PCC_SOCKET_CONNECT;
			if(connect_server_send(sc))
			{
				delete sc;

				m_send_lock.unlock();				
				return -1;
			}
			m_send_live++;

			m_send_lock.unlock();
			return 0;
		}
		
		if(m_send_idle <= 0)
		{
			m_send_lock.unlock();
			return -1;
		}
		sc = m_send_loop;
		m_send_loop = m_send_loop->ptr_next;
		m_send_idle--;

		m_send_lock.unlock();
		return 0;
	}

	void drop_send_loop(PCC_SOCKET_CONNECT* sc)
	{
		cp_close_socket(sc->sck_live);
		delete sc;

		m_send_lock.lock();
		m_send_live--;
		m_send_lock.unlock();
	}

	void push_send_loop(PCC_SOCKET_CONNECT* sc)
	{
		m_send_lock.lock();

		sc->ptr_next = m_send_loop;
		m_send_loop = sc;
		m_send_idle++;

		m_send_lock.unlock();
	}

	void delete_live_task(PCC_LIVE_TASK* lt)
	{		
		m_live_lock.lock();
		if(m_live_task.DeleteKey_FL(lt->task_id))
			assert(0);
		m_live_lock.unlock();
	}

	var_4 connect_server_send(PCC_SOCKET_CONNECT* sc)
	{
		if(cp_connect_socket(sc->sck_live, m_remote_ip, m_remote_port))
		{
			m_journal.record(JL_LOW, "PCC - connect to %s:%d send failure\n", m_remote_ip, m_remote_port);
			return -1;
		}

		var_4 flag = 1;
#ifdef _WIN32_ENV_
		setsockopt(sc->sck_live, IPPROTO_TCP, TCP_NODELAY, (var_1*)&flag, sizeof(flag));
#else
		setsockopt(sc->sck_live, IPPROTO_TCP, TCP_NODELAY, (var_vd*)&flag, sizeof(flag));
#endif

		m_journal.record(JL_LOW, "PCC - %s:%d connect send success\n", m_remote_ip, m_remote_port);

		// 4byte(^-^!) 8byte(uuid) 4byte(>>>>)
		var_1 buffer[32];
		*(var_u4*)buffer = *(var_u4*)"^-^!";
		*(var_u8*)(buffer + 4) = m_connect_tag;
		*(var_u4*)(buffer + 12) = *(var_u4*)">>>>";

		if(cp_sendbuf(sc->sck_live, buffer, 16))
			return -1;

		m_journal.record(JL_LOW, "PCC - %s:%d connect send(head) success\n", m_remote_ip, m_remote_port);

		return 0;
	}

	var_4 connect_server_recv(CP_SOCKET_T& sck_live)
	{
		if(sck_live != -1)
			cp_close_socket(sck_live);

		while(cp_connect_socket(sck_live, m_remote_ip, m_remote_port))
		{
			m_journal.record(JL_LOW, "PCC - connect to %s:%d recv failure, retry\n", m_remote_ip, m_remote_port);

			cp_sleep(5000);
			continue;
		}

		var_4 flag = 1;
#ifdef _WIN32_ENV_
		setsockopt(sck_live, IPPROTO_TCP, TCP_NODELAY, (var_1*)&flag, sizeof(flag));
#else
		setsockopt(sck_live, IPPROTO_TCP, TCP_NODELAY, (var_vd*)&flag, sizeof(flag));
#endif

		m_journal.record(JL_LOW, "PCC - %s:%d connect recv success\n", m_remote_ip, m_remote_port);

		// 4byte(^-^!) 8byte(uuid) 4byte(<<<<)
		var_1 buffer[32];
		*(var_u4*)buffer = *(var_u4*)"^-^!";
		*(var_u8*)(buffer + 4) = m_connect_tag;
		*(var_u4*)(buffer + 12) = *(var_u4*)"<<<<";

		if(cp_sendbuf(sck_live, buffer, 16))
			return -1;

		m_journal.record(JL_LOW, "PCC - %s:%d connect recv(head) success\n", m_remote_ip, m_remote_port);		

		return 0;
	}

	static CP_THREAD_T thread_recv(var_vd* arg)
	{
		UC_PersistentConnections_Client* cc = (UC_PersistentConnections_Client*)arg;
		
		CP_SOCKET_T sck_live = -1;
		while(cc->connect_server_recv(sck_live))
			cp_sleep(5000);

		cc->m_journal.record(JL_LOW, "PCC - recv thread is running\n");

		for(;;)
		{
			var_1 head_buf[32]; // 4byte(^-^!) 8byte(uuid) 4byte(sn) 4byte(request length) xbyte(request buffer)
			if(cp_recvbuf(sck_live, head_buf, 20))
			{
				cc->m_journal.record(JL_LOW, "PCC - recv head failure\n");
				while(cc->connect_server_recv(sck_live))
					cp_sleep(5000);
				continue;
			}

			if(*(var_u4*)head_buf != *(var_u4*)"^-^!" || *(var_u8*)(head_buf + 4) != cc->m_connect_tag || *(var_u4*)(head_buf + 12) >= cc->m_live_id)
			{
				cc->m_journal.record(JL_LOW, "PCC - check head failure\n");
				while(cc->connect_server_recv(sck_live))
					cp_sleep(5000);
				continue;
			}

			cc->m_live_lock.lock();
			
			var_vd* retbuf = NULL;
			if(cc->m_live_task.SearchKey_FL(*(var_u4*)(head_buf + 12), &retbuf))
			{
				cc->m_live_lock.unlock();

				cc->m_journal.record(JL_LOW, "PCC - find live id failure\n");
				while(cc->connect_server_recv(sck_live))
					cp_sleep(5000);
				continue;
			}
			PCC_LIVE_TASK* lt = *(PCC_LIVE_TASK**)retbuf;

			cc->m_task_lock.lock();
			lt->task_busy++;
			cc->m_task_lock.unlock();

			cc->m_live_lock.unlock();

			lt->recv_len = *(var_4*)(head_buf + 16);

			if(cp_recvbuf(sck_live, lt->recv_buf, lt->recv_len))
			{
				lt->task_flag = -1;

				cc->m_task_lock.lock();
				lt->task_busy--;
				cc->m_task_lock.unlock();

				cc->m_journal.record(JL_LOW, "PCC - recv body failure\n");
				while(cc->connect_server_recv(sck_live))
					cp_sleep(5000);
				continue;
			}
			lt->task_flag = 1;

			cc->m_task_lock.lock();
			lt->task_busy--;
			cc->m_task_lock.unlock();

			cc->m_journal.record(JL_LOW, "PCC - recv data is ok\n");
		}
		
		return 0;
	}

public:
	var_1  m_remote_ip[16];
	var_u2 m_remote_port;

	var_4  m_connect_num;
	var_u8 m_connect_tag;

	var_4 m_time_out;

	UC_Journal m_journal;
			
	CP_MUTEXLOCK          m_task_lock;

	var_u4                m_live_id;
	CP_MUTEXLOCK          m_live_lock;
	UT_HashSearch<var_u4> m_live_task;

	var_4               m_send_idle;
	var_4               m_send_live;
	CP_MUTEXLOCK        m_send_lock;
	PCC_SOCKET_CONNECT* m_send_loop;
};

#endif // _UC_PERSISTENT_CONNECTIONS_H_
