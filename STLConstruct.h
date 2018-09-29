#pragma once 

#include"TypeTraits.h"

template<class T1, class T2>
inline void Construct(T1* p, const T2 value)
{
	new(p)T2(value);
}
template<class T>
inline void Destory(T* p)
{
	//	cout << typeid(T), name() << endl;
	if (p)
		p->~T();
}


//原生态类型的迭代器
//template<class Iterator>
//void _Destory_(Iterator first, Iterator last, TrueType)
//{
//	cout << "O(1)" << endl;//不用析构
//}
//自定义类型迭代器
template<class Iterator>
void _Destory_(Iterator first, Iterator last)
{
	//需要进行析构
	while (first != last)
	{
		Destory(&(*first));
		first++;
	}
}
template<class Iterator>
void Destory(Iterator first, Iterator last)
{
	//TypeTriats<IteratorTraits<Iterator>::ValueType>::hasTrivialDestructor del;
	_Destory_(first, last, TypeTriats<IteratorTraits<Iterator>::ValueType>::hasTrivialDestructor());
}
