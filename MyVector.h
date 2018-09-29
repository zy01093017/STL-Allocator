#pragma once

#include "TypeTraits.h"
#include "Allocator.h"
//#include "STLConstruct.h"


template<class T1, class T2>
inline void Construct(T1* p, const T2 value)
{
	new(p)T2(value);
}

template<class T>
inline void Destory(T* p)
{
	if (p)
		p->~T();
}

template<class Iterator>
void Destory(Iterator first, Iterator last)
{
	//需要进行析构
	while (first != last)
	{
		Destory(&(*first));
		first++;
	}
}
//vector的迭代器是一个T*
template<class T, class Allocate = alloc>
class MyVector
{
public:
	typedef SimpleAlloc<T, Allocate> SimpleAlloc;
	typedef T* Iterator;
	typedef const T* ConstIterator;
	
public:

	
	//构造函数
	MyVector()
		:_start(NULL)
		, _finish(NULL)
		, _endOfStarge(NULL)
	{}

	MyVector(size_t n, const T& data)
	{
		_start = SimpleAlloc::Allocate(n);
		for (size_t index = 0; index < n; ++index)
			Construct(_start + idx, data);
		_finish = _start + n;
		_endOfStarge = _finish;
	}
	//析构函数
	~MyVector()
	{
		clear();
		SimpleAlloc::Deallocate((T*)_start, Capacity());
	}

	/////////////////////Modifiers//////////////////////
	void PushBack(const T& data)
	{
		_CheckCapacity();
		Construct(_finish, data);
		++_finish;
	}
	void PopBack()
	{
		assert(_start == _finish);
		Destory(_finish - 1);
		--_finsih;
	}
	Iterator Insert(Iterator pos, const T& data)
	{
		assert(pos >= Begin() && pos <= end());//
		_CheckCapacity();
		for (Iterator it = _finish; it > pos; --it)
			*it = *(it - 1);
		*pos = data;
		++_finish;
		return pos;
	}
	Iterator Erase(Iterator pos)
	{
		assert(pos >= Begin() && pos <= end());//
		for (Iterator it = pos; it < _finish - 1; ++it)
			*it = *(it + 1);
		Destory(_finish - 1);
		--_finish;
		return pos;
	}
	

	void clear()
	{
		Destory(_start, _finish);
		_finish = _start;
	}

	///////////////////////////Capacity////////////////
	size_t Size()const
	{
		return  _finish - _start;
	}
	size_t Capacity()const
	{
		return _endOfStarge - _start;
	}
	bool Empty()const
	{
		return _start == NULL;
	}
	void Resize(size_t newSize, const T& data)
	{
		if (newsize < Size())
		{
			Destory(_start + newSize, _finish);
			_finish = _start + newSize;
		}
		else if (_start + newsize < _endOfStarge)
		{
			for (size_t idx = Size(); idx < newSize; ++idx)
				Construct(_start + idx, data);
		}
		else
		{
			Iterator temp = SimpleAlloc::Allocate(newSize);
			size_t oldSize = Size();
			//搬移元素
			for (size_t idx = 0; idx < oldSize(); ++idx)
				Construct(temp + idx, _start[idx]);
			//销毁旧空间
			Destory(_start, _finish);
			SimpleAlloc::DeAllocate(_start, Capacity());
			_start = temp;
			_finish = _start + newSize;
			_endOfStarge = _finish;
		}
	}

	void Reserve(size_t n)
	{
		size_t oldSize = Size();
		//如果n小于容量，什么也不做
		if (n < Capacity())
		{
			return;
		}
		//如果n大于容量，则重新开辟空间,但不初始化
		Itertaor temp = SimpleAlloc::Allocate(n);
		for (size_t it = 0; idx < oldSize; ++idx)
			Construct(temp + idx, _start[idx]);
		Destory(_start, Capacity());
		_start = temp;
		_finsih = _start + oldSize;
		_endOfStarge = _start + n;
	}


	///////////////////////Iterator//////////////////////
	Iterator Begin()
	{
		return _start;
	}
	Iterator End()
	{
		return _finish;
	}
	ConstIterator Begin() const
	{
		return _start;
	}

	ConstIterator End() const
	{
		return _finish;
	}

	T operator[](size_t pos)
	{
		assert(pos < Size());
		return _start[pos];
	}

	Iterator& at(size_t pos)
	{
		if (pos >= Size())
		{
			throw out_of_range("pos out of range");
		}

		return _start[pos];
	}


private:
	void _CheckCapacity()
	{
		if (_finish == _endOfStarge)
		{
			int oldSize = Size();
			size_t newSize = 2 * oldSize + 3;
			Iterator temp = SimpleAlloc::Allocate(newSize);
			for (size_t idx = 0; idx < oldSize; ++idx)
				Construct(temp + idx, _start[idx]);
			if (_start)
			{
				Destory(_start, _finish);
				SimpleAlloc::Deallocate(_start, Capacity());
			}
			_start = temp;
			_finish = _start + oldSize;
			_endOfStarge = _start + newSize;

		}
	}
private:
	Iterator _start;
	Iterator _finish;
	Iterator _endOfStarge;
};



void Print_MyVector(const MyVector<int>& v1)
{
	MyVector<int>::ConstIterator it1 = v1.Begin();
	while (it1 != v1.End())
	{
		//*it1 = 10;
		cout << *it1 << " ";
		++it1;
	}
	cout << endl;
}

void TestMyVector()
{
	MyVector<int> v1;
	v1.PushBack(1);
	v1.PushBack(2);
	v1.PushBack(3);
	v1.PushBack(4);

	Print_MyVector(v1);

	/*for (size_t i = 0; i < v1.Size(); ++i)
	{
		cout<<v1[i]<<" ";
	}
	cout<<endl;

	MyVector<int>::Iterator it1 = v1.Begin();
	while (it1 != v1.End())
	{
		cout << *it1 << " ";
		++it1;
	}
	cout << endl;*/
}