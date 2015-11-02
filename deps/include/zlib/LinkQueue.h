/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : LinkQueue.h <
相关文件     : <>
文件实现功能 : <各平台数据类型的定义> 
作者         : <和文强>
版本         : <1.00.00>
--------------------------------------------------------------------------------
备注         : <栈有顺序存储和链式存储,前者的值是固定的，而后者没有
                LIFO(先进先出)
				本栈为链式栈
				如果有n个链式栈，其头指针数组可以用以下方式定义：
				LinkStack<Template*> * s = new linkStack<Template*>[n];
				>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
YYYY/MM/DD   X.Y      <作者或修改者名>    <修改内容>
</PRE>
*******************************************************************************/
#ifndef _LINKQUEUE_H_2006_08_28
#define _LINKQUEUE_H_2006_08_28

#include "Types.h"

class LinkQueue;
//定义结点
//template <class Template*>
class QueueNode {
	friend class LinkQueue;
public :
	Template* nodedata;                        //结点数据
	QueueNode * nodelink;           //结点域
	QueueNode (Template* data, QueueNode*link = NULL)
		:nodedata(data),nodelink(link) {} //构造函数
};
//链式栈
//template <class Template*>
class LinkQueue {
public:
	LinkQueue()
		:front(NULL),rear(NULL) {}
	
	~LinkQueue() { 
		QueueNode* p;
		while(NULL != front){
			p= front;
			front = front->nodelink;
			delete p;
		}
	}
	
	//增加
	void push (Template*& item) {
		if (NULL == front ) front = rear = new QueueNode(item, NULL);
		else
			rear = rear->nodelink = new QueueNode(item, NULL);
	}
	//取出--调用强查看是否为空
	Template* pop() {
		QueueNode* p = front;
		Template* retvalue = p->nodedata;
		front = front->nodelink;
		delete p;
		return retvalue;
	}
	//队列头元素
	Template* getFront(){
		if (isEmpty()) return NULL;
		return front->nodedata;
	}
	
	void makeEmpty() { 
		QueueNode * p;
		while(NULL != front){
			p= front;
			front = front->nodelink;
			delete p;
		}
	}
	
	bool isEmpty() { return (front == NULL);}
	
private:
	
	QueueNode * front,* rear;
};

#endif