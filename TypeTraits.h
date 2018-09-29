
#pragma once

#include "Allocator.h"
#include <assert.h>
#include <string>

struct _TrueType
{};
struct _FalseType
{};


//POD:plain  old  data  ƽ������
template<class T>
struct TypeTraits
{
	typedef _FalseType IsPodType;//�ṹ��TypeTraits�ж���һ����Ա����IsPodType����ֵΪ__FalseType
};

template<>
struct TypeTraits<int>
{
	typedef _TrueType IsPodType;//�ṹ��TypeTraits�ж���һ����Ա����IsPodType����ֵΪ__TrueType

};

template<class T>
struct TypeTraits<T*>
{
	typedef _TrueType IsPodType;
};

template<class T>
T*  __TypeCopy(const T* src, T* dst, size_t n, _TrueType)//���ɺ�������
{
	cout << "memcpy" << endl;
	return   (T*)memcpy(dst, src, sizeof(T)*n);
}



template<class T>
T* __TypeCopy(const T* src, T* dst, size_t n, _FalseType)
{
	cout << "for+operator" << endl;
	for (size_t i = 0; i < n; ++i)
	{
		dst[i] = src[i];
	}
	return dst;
}


template<class T>
T* TypeCopy(const T* src, T* dst, size_t n)
{
	return  __TypeCopy(src, dst, n, TypeTraits<T>::IsPodType());

}





