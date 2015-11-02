/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : LinkQueue.h <
����ļ�     : <>
�ļ�ʵ�ֹ��� : <��ƽ̨�������͵Ķ���> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : <ջ��˳��洢����ʽ�洢,ǰ�ߵ�ֵ�ǹ̶��ģ�������û��
                LIFO(�Ƚ��ȳ�)
				��ջΪ��ʽջ
				�����n����ʽջ����ͷָ��������������·�ʽ���壺
				LinkStack<Template*> * s = new linkStack<Template*>[n];
				>
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
</PRE>
*******************************************************************************/
#ifndef _LINKQUEUE_H_2006_08_28
#define _LINKQUEUE_H_2006_08_28

#include "Types.h"

class LinkQueue;
//������
//template <class Template*>
class QueueNode {
	friend class LinkQueue;
public :
	Template* nodedata;                        //�������
	QueueNode * nodelink;           //�����
	QueueNode (Template* data, QueueNode*link = NULL)
		:nodedata(data),nodelink(link) {} //���캯��
};
//��ʽջ
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
	
	//����
	void push (Template*& item) {
		if (NULL == front ) front = rear = new QueueNode(item, NULL);
		else
			rear = rear->nodelink = new QueueNode(item, NULL);
	}
	//ȡ��--����ǿ�鿴�Ƿ�Ϊ��
	Template* pop() {
		QueueNode* p = front;
		Template* retvalue = p->nodedata;
		front = front->nodelink;
		delete p;
		return retvalue;
	}
	//����ͷԪ��
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