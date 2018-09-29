#pragma once

#include "Allocator.h"
#include <assert.h>

template<class T>
struct ListNode
{
	ListNode<T>* _next;
	ListNode<T>* _prev;
	T _x;

	ListNode(const T& x = T())
		:_next(NULL)
		, _prev(NULL)
		, _x(x)
	{}
};


template<class T, class Ref, class Ptr>
struct __ListIterator
{
	typedef ListNode<T> Node;
	typedef __ListIterator<T, Ref, Ptr> Self;

	Node* _node;

	__ListIterator(Node* node)//构造
		:_node(node)
	{}

	Ref operator*()//拷贝构造
	{
		return _node->_x;
	}

	Ptr operator->()//运算符重载
	{
		return &(operator*());
	}

	Self& operator++()//前置++
	{
		_node = _node->_next;
		return *this;
	}

	Self operator++(int)//后置++
	{
		Self tmp(_node);
		_node = _node->_next;

		return tmp;
	}

	Self& operator--()
	{
		_node = _node->_prev;
		return *this;
	}

	Self operator--(int)
	{
		Self tmp(_node);
		_node = _node->_prev;

		return tmp;
	}

	bool operator==(const Self& s) const
	{
		return _node == s._node;
	}

	bool operator!=(const Self& s) const
	{
		return _node != s._node;
	}
};

template<class T,class Alloc = alloc>
class MyList
{
	typedef ListNode<T> Node;
public:
	typedef __ListIterator<T, T&, T*> Iterator;
	typedef __ListIterator<T, const T&, const T*> ConstIterator;
	typedef SimpleAlloc<Node, Alloc> ListNodeAllocator;
	Iterator Begin()
	{
		//return Iterator(_head->_next);
		return _head->_next;
	}

	Iterator End()
	{
		//return Iterator(_head);
		return _head;
	}


	ConstIterator Begin() const
	{
		//return Iterator(_head->_next);
		return _head->_next;
	}

	ConstIterator End() const
	{
		//return Iterator(_head);
		return _head;
	}

	/*************申请空间****************/
	Node* BuyNode(const T& x)
	{
		Node*  node = ListNodeAllocator::Allocate();//申请空间
		new (node)Node(x);

		return node;
	}


	MyList()
	{
		_head = BuyNode(T());
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Clear()
	{
		Iterator it = Begin();
		while (it != End())
		{
			Iterator cur = it;
			++it;
			DestoryNode(cur._node);
		}

		_head->_next = _head;
		_head->_prev = _head;
	}

	void DestoryNode(Node* p)
	{
		p->~Node();
		ListNodeAllocator::Deallocate(p);
	}

	~MyList()
	{
		Clear();
		DestoryNode(_head);
	}

	/***************增删改查****************/
	void PushBack(const T& x)
	{
		Node* tail = _head->_prev;
		// tail newnode  _head
		//Node* newnode = new Node(x);
		Node* newnode = BuyNode(x);
		tail->_next = newnode;
		newnode->_prev = tail;

		newnode->_next = _head;
		_head->_prev = newnode;
	}
	void PushFront(const T& x)
	{
		Node* node = BuyNode(x);
		node->_next = _head->_next;
		node->_prev = _head;
		_head->_next = node;
	}
	void PopBack()
	{
		if (_head == _head)
			return;
		Node* pDelNode = _head->_prev;
		_head->_prev = _head->_prev->_prev;
		_head->_prev->_prev->_next = _head;
		DestoryNode(pDelNode)//先析构在销毁空间
	}

	void PopFront()
	{
		if (_head->_prev = _head)
			return;
		Node* pDelNode = _head->_next;
		_head->_next = pDelNode->_next;
		pDelNode->_next->_prev = _head;
		DestoryNode(pDelNode)//先析构在销毁空间
	}

	void Insert(Iterator pos, const T& x)
	{
		Node* cur = pos._node;
		Node* prev = cur->_prev;
		//Node* newnode = new Node(x);
		Node* newnode = BuyNode(x);

		prev->_next = newnode;
		newnode->_prev = prev;
		newnode->_next = cur;
		cur->_prev = newnode;
	}



	Iterator Erase(Iterator& pos)
	{
		assert(pos != End());

		Node* cur = pos._node;
		Node* prev = cur->_prev;
		Node* next = cur->_next;

		next->_prev = prev;
		prev->_next = next;
		//delete cur;
		DestoryNode(cur);

		pos = prev;

		return next;
	}


	/***********容量******************/
	size_t  Size()const
	{
		int count = 0;
		Node* cur = _head;
		while (cur->_next != _head)
		{
			++count;
			cur = cur->_next;
		}
		return count;
	}

	bool Empty()const
	{
		if (_head->_next != _head)
			return false;
		else
			return true;
	}
	size_t max_size()const
	{
		return -1;
	}

protected:
	Node* _head;
};



void PrintList(const MyList<int>& l1)
{
	MyList<int>::ConstIterator it1 = l1.Begin();
	while (it1 != l1.End())
	{
		cout << *it1 << " ";
		++it1;
	}
	cout << endl;
}

void TestMyList()
{
	MyList<int> l1;
	l1.PushBack(1);
	l1.PushBack(2);
	l1.PushBack(3);
	l1.PushBack(4);

	MyList<int>::Iterator it = l1.Begin();
	while (it != l1.End())
	{
		if (*it % 2 == 0)
		{
			it = l1.Erase(it);
		}
		else
		{
			++it;
		}

		/*if (*it % 2 == 0)
		{
			l1.Erase(it);
		}*/

		++it;
	}

	PrintList(l1);

	/*cout<<sizeof(MyList<int>::Iterator)<<endl;
	cout<<sizeof(ListNode<int>*)<<endl;*/
}

void PushDataMalloc(size_t n)
{
	MyList<int, __MallocAllocTemplate<0>> l;
	for (size_t i = 0; i < n; ++i)
	{
		l.PushBack(i);
	}
	
}

void PushDataAlloc(size_t n)
{
	MyList<int, __DefaultAllocTemplate<false, 0>> l;
	for (size_t i = 0; i < n; ++i)
	{
		l.PushBack(i);
	}
}


#include <time.h>

void TestMyListAllocOP()
{
	size_t n = 1000000;

	size_t begin1 = clock();
	PushDataMalloc(n);
	PushDataMalloc(n);
	PushDataMalloc(n);
	PushDataMalloc(n);
	size_t end1 = clock();
	cout << end1 - begin1 << endl;

	size_t begin2 = clock();
	PushDataAlloc(n);
	PushDataAlloc(n);
	PushDataAlloc(n);
	PushDataAlloc(n);
	size_t end2 = clock();
	cout << end2 - begin2 << endl;
}