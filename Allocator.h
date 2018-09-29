#pragma once


#include "TraceLog.h"

typedef void(*HANDLE_FUNC)();

//一级空间配置器的实现（申请一大块内存）
template<int inst>
class __MallocAllocTemplate
{
private:
	static void* OOM_Malloc(size_t n)//alloc申请空间失败
	{
		while (1)
		{
			if (__malloc_alloc_oom_handler == 0)//如果系统没有，抛异常
			{
				throw bad_alloc();
			}
			else
			{
				__malloc_alloc_oom_handler();//不为0，则进行释放内存
				void* ret = malloc(n);
				if (ret)
				{
					return ret;
				}
			}
		}
	}
	static HANDLE_FUNC __malloc_alloc_oom_handler;

public:
	static void* Allocate(size_t n)//申请空间
	{
		void* result = malloc(n);
		if (result == 0)
		{
			result = OOM_Malloc(n);//系统空间处理不足
		}
		return result;
	}

	static void Deallocate(void *p, size_t /* n */)//空间释放函数
	{
		free(p);
	}

	//设置函数指针__malloc_alloc_oom_handler,只有设置后才能进行空间不足时的处理
	static HANDLE_FUNC SetMallocHandler(HANDLE_FUNC f)
	{
		HANDLE_FUNC old = __malloc_alloc_oom_handler;
		__malloc_alloc_oom_handler = f;
		return old;
	}
};

template<int inst>
HANDLE_FUNC __MallocAllocTemplate<inst>::__malloc_alloc_oom_handler = 0;

//#include <windows.h>
//
//void FreeHandle()
//{
//	cout << "释放内存" << endl;
//	Sleep(1000);
//}
//
//void TestMallocAlloc()
//{
//	__MallocAllocTemplate<0>::SetMallocHandler(FreeHandle);
//	void* p1 = __MallocAllocTemplate<0>::Allocate(10);
//	void* p2 = __MallocAllocTemplate<0>::Allocate(0x7fffffff);
//}


/****************二级空间配置器***********************/
template<bool threads,int inst>
class __DefaultAllocTemplate
{
public:
	//该字节应该申请空间的大小对应链表的位置
	static size_t FREELIST_INDEX(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}

	//向上对齐到8的整数倍处（当前处理为8，bytes不确定）
	static size_t RoundUp(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) & ~(__ALIGN - 1));
	}

	static char* ChunkAlloc(size_t size, size_t& nobjs)
	{
		size_t totalbytes = size*nobjs;//20个对象的大小
		size_t leftbytes = _endfree - _startfree;//内存池剩余大小
		if (leftbytes >= totalbytes)//表示内存池剩余的内存大小足够
		{
			__TRACE_DEBUG("内存池中有足够20个对象大块内存\n");
			char* ret = _startfree;
			_startfree += totalbytes;
			return ret;
		}
		else if (leftbytes > size)//不够20个对象的内存空间
		{
			nobjs = leftbytes / size;
			__TRACE_DEBUG("内存池中只有%u个对象大块内存\n", nobjs);
		
			totalbytes = size*nobjs;
			char* ret = _startfree;
			_startfree += totalbytes;
			return ret;
		}
		else
		{
			__TRACE_DEBUG("内存池中一个对象都不够\n");
			//内存池中一个对象的大小都不够
			if (leftbytes > 0)//还有剩余碎片，将其处理
			{
				size_t index = FREELIST_INDEX(leftbytes);//算位置，将这一小块内存挂在相应的内存池的自由链表下
				((Obj*)_startfree)->_freelistlink = _freelist[index];
				_freelist[index] = (Obj*)_startfree;
			}

			//如果没有剩余的内存，就向系统申请
			size_t bytestoget = totalbytes * 2 + RoundUp(_heapsize >> 4);

			__TRACE_DEBUG("向系统申请%ubytes字节\n", bytestoget);
			_startfree = (char*)malloc(bytestoget);
			if (_startfree == NULL)//系统空间不足时，在自由链表中查找空间
			{
				size_t index = FREELIST_INDEX(leftbytes);
				while (index < __NFREELISTS)
				{
					if (_freelist[index])
					{
						Obj* obj = _freelist[index];
						_freelist[index] = obj->_freelistlink;


						_startfree = (char*)obj;
						_endfree = _startfree + (index + 1)*__ALIGN;
						return ChunkAlloc(size, nobjs);
					}
					++index;
				}
				//自由链表中也没有足够大的空间，到一级空间配置器进行申请
				_endfree = NULL;//这里将endfree置为0，当一级空间配置器申请失败后，startfree为0，故将其置为0
				_startfree = (char*)__MallocAllocTemplate<0>::Allocate(bytestoget);//调用一级空间配置器
			}

			_endfree = _startfree + bytestoget;
			_heapsize += bytestoget;

			return ChunkAlloc(size, nobjs);
		}


	}

	//返回一块内存，将剩余内存块挂到链表下面
	static void* ReFill(size_t size)
	{
		size_t nobjs = 20;
		char* chunk = ChunkAlloc(size, nobjs);

		if (nobjs == 1)
		{
			return chunk;
		}
		//将剩余内存块挂到自由链表下面
		size_t index = FREELIST_INDEX(size);
		__TRACE_DEBUG("返回一个,将剩余%u个对象挂到freelist[%u]\n", nobjs - 1, index);
		Obj* cur = (Obj*)(chunk + size);
		_freelist[index] = cur;

		for (size_t i = 0; i < nobjs - 2; ++i)
		{
			Obj* next = (Obj*)((char*)cur + size);
			cur->_freelistlink = next;
			cur = next;
		}
		cur->_freelistlink = NULL;

		return chunk;
	}

	//申请空间
	static void* Allocate(size_t size)
	{
		if (size > __MAX_BYTES)
		{
			__TRACE_DEBUG("向一级空间配置器进行申请:%u bytes\n", size);
			return __MallocAllocTemplate<0>::Allocate(size);//如果大于128个字节，调用一级空间配置器

		}
		__TRACE_DEBUG("向二级空间配置器进行申请:%u bytes\n", size);
		//如果没有超出128个字节，就调用二级空间配置器
		//1.先计算对应位置
		size_t index = FREELIST_INDEX(size);
		Obj* res = _freelist[index];//自由链表
		if (res == 0)//如果当前位置没有，就去取较大的一块内存
		{
			return ReFill(RoundUp(size));//向上取整
		}
		else//有，就返回一块内存
		{
			__TRACE_DEBUG("在freelist[%u]位置取一个内存块对象\n", index);
			Obj* obj = _freelist[index];
			_freelist[index] = obj->_freelistlink;

			return obj;
		}
	}

	static void Deallocate(void* p, size_t n)
	{
		if (n > __MAX_BYTES)
		{
			__TRACE_DEBUG("向一级空间配置器进行释放:%p %ubytes\n", p, n);
			__MallocAllocTemplate<0>::Deallocate(p, n);//大于内存池，就调用一级空间配置器
		}
		else
		{
			size_t index = FREELIST_INDEX(n);//小于内存池，就将其挂在自由链表下面
			__TRACE_DEBUG("将对象%p挂到freelist[%u]\n", p, index);

			((Obj*)p)->_freelistlink = _freelist[index];
			_freelist[index] = (Obj*)p;
		}
	}

public:
	enum { __ALIGN = 8 };//默认是8个字节
	enum { __MAX_BYTES = 128 };//申请的大块内存默认是128
	enum { __NFREELISTS = __MAX_BYTES / __ALIGN };
	
	union Obj
	{
		union Obj* _freelistlink;
		char _client[1];
	};

	
	static Obj* _freelist[__NFREELISTS];
	// 自由链表配合管理

	static char* _startfree;
	static char* _endfree;
	static size_t _heapsize;
	
};

template <bool threads, int inst>
typename __DefaultAllocTemplate<threads, inst>::Obj*
__DefaultAllocTemplate<threads, inst>::_freelist[__NFREELISTS] = { 0 };

// 内存池
template <bool threads, int inst>
char* __DefaultAllocTemplate<threads, inst>::_startfree = NULL;

template <bool threads, int inst>
char* __DefaultAllocTemplate<threads, inst>::_endfree = NULL;

template <bool threads, int inst>
size_t __DefaultAllocTemplate<threads, inst>::_heapsize = 0;



#ifdef __USE_MALLOC
typedef __MallocAllocTemplate<0> alloc;
#else
typedef __DefaultAllocTemplate<false, 0> alloc;
#endif // __USE_MALLOC



//对上述实现的空间配置器进行封装，因为上述的空间配置器的函数参数是字节数
//而我们通常在开辟空间时传入的是参数的类型


void TestDefaultAlloc()
{
	for (size_t i = 0; i < 21; ++i)
	{
		__TRACE_DEBUG("申请第%u个对象\n", i);
		__DefaultAllocTemplate<false, 0>::Allocate(7);
	}
}

template<class T, class Alloc>
class SimpleAlloc {

public:
	static T* Allocate(size_t n)
	{
		return 0 == n ? 0 : (T*)Alloc::Allocate(n * sizeof (T));
	}

	static T* Allocate(void)
	{
		return (T*)Alloc::Allocate(sizeof (T));
	}

	static void Deallocate(T *p, size_t n)
	{
		if (0 != n) Alloc::Deallocate(p, n * sizeof (T));
	}

	static void Deallocate(T *p)
	{
		Alloc::Deallocate(p, sizeof (T));
	}
};



