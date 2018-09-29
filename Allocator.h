#pragma once


#include "TraceLog.h"

typedef void(*HANDLE_FUNC)();

//һ���ռ���������ʵ�֣�����һ����ڴ棩
template<int inst>
class __MallocAllocTemplate
{
private:
	static void* OOM_Malloc(size_t n)//alloc����ռ�ʧ��
	{
		while (1)
		{
			if (__malloc_alloc_oom_handler == 0)//���ϵͳû�У����쳣
			{
				throw bad_alloc();
			}
			else
			{
				__malloc_alloc_oom_handler();//��Ϊ0��������ͷ��ڴ�
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
	static void* Allocate(size_t n)//����ռ�
	{
		void* result = malloc(n);
		if (result == 0)
		{
			result = OOM_Malloc(n);//ϵͳ�ռ䴦����
		}
		return result;
	}

	static void Deallocate(void *p, size_t /* n */)//�ռ��ͷź���
	{
		free(p);
	}

	//���ú���ָ��__malloc_alloc_oom_handler,ֻ�����ú���ܽ��пռ䲻��ʱ�Ĵ���
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
//	cout << "�ͷ��ڴ�" << endl;
//	Sleep(1000);
//}
//
//void TestMallocAlloc()
//{
//	__MallocAllocTemplate<0>::SetMallocHandler(FreeHandle);
//	void* p1 = __MallocAllocTemplate<0>::Allocate(10);
//	void* p2 = __MallocAllocTemplate<0>::Allocate(0x7fffffff);
//}


/****************�����ռ�������***********************/
template<bool threads,int inst>
class __DefaultAllocTemplate
{
public:
	//���ֽ�Ӧ������ռ�Ĵ�С��Ӧ�����λ��
	static size_t FREELIST_INDEX(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}

	//���϶��뵽8��������������ǰ����Ϊ8��bytes��ȷ����
	static size_t RoundUp(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) & ~(__ALIGN - 1));
	}

	static char* ChunkAlloc(size_t size, size_t& nobjs)
	{
		size_t totalbytes = size*nobjs;//20������Ĵ�С
		size_t leftbytes = _endfree - _startfree;//�ڴ��ʣ���С
		if (leftbytes >= totalbytes)//��ʾ�ڴ��ʣ����ڴ��С�㹻
		{
			__TRACE_DEBUG("�ڴ�������㹻20���������ڴ�\n");
			char* ret = _startfree;
			_startfree += totalbytes;
			return ret;
		}
		else if (leftbytes > size)//����20��������ڴ�ռ�
		{
			nobjs = leftbytes / size;
			__TRACE_DEBUG("�ڴ����ֻ��%u���������ڴ�\n", nobjs);
		
			totalbytes = size*nobjs;
			char* ret = _startfree;
			_startfree += totalbytes;
			return ret;
		}
		else
		{
			__TRACE_DEBUG("�ڴ����һ�����󶼲���\n");
			//�ڴ����һ������Ĵ�С������
			if (leftbytes > 0)//����ʣ����Ƭ�����䴦��
			{
				size_t index = FREELIST_INDEX(leftbytes);//��λ�ã�����һС���ڴ������Ӧ���ڴ�ص�����������
				((Obj*)_startfree)->_freelistlink = _freelist[index];
				_freelist[index] = (Obj*)_startfree;
			}

			//���û��ʣ����ڴ棬����ϵͳ����
			size_t bytestoget = totalbytes * 2 + RoundUp(_heapsize >> 4);

			__TRACE_DEBUG("��ϵͳ����%ubytes�ֽ�\n", bytestoget);
			_startfree = (char*)malloc(bytestoget);
			if (_startfree == NULL)//ϵͳ�ռ䲻��ʱ�������������в��ҿռ�
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
				//����������Ҳû���㹻��Ŀռ䣬��һ���ռ���������������
				_endfree = NULL;//���ｫendfree��Ϊ0����һ���ռ�����������ʧ�ܺ�startfreeΪ0���ʽ�����Ϊ0
				_startfree = (char*)__MallocAllocTemplate<0>::Allocate(bytestoget);//����һ���ռ�������
			}

			_endfree = _startfree + bytestoget;
			_heapsize += bytestoget;

			return ChunkAlloc(size, nobjs);
		}


	}

	//����һ���ڴ棬��ʣ���ڴ��ҵ���������
	static void* ReFill(size_t size)
	{
		size_t nobjs = 20;
		char* chunk = ChunkAlloc(size, nobjs);

		if (nobjs == 1)
		{
			return chunk;
		}
		//��ʣ���ڴ��ҵ�������������
		size_t index = FREELIST_INDEX(size);
		__TRACE_DEBUG("����һ��,��ʣ��%u������ҵ�freelist[%u]\n", nobjs - 1, index);
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

	//����ռ�
	static void* Allocate(size_t size)
	{
		if (size > __MAX_BYTES)
		{
			__TRACE_DEBUG("��һ���ռ���������������:%u bytes\n", size);
			return __MallocAllocTemplate<0>::Allocate(size);//�������128���ֽڣ�����һ���ռ�������

		}
		__TRACE_DEBUG("������ռ���������������:%u bytes\n", size);
		//���û�г���128���ֽڣ��͵��ö����ռ�������
		//1.�ȼ����Ӧλ��
		size_t index = FREELIST_INDEX(size);
		Obj* res = _freelist[index];//��������
		if (res == 0)//�����ǰλ��û�У���ȥȡ�ϴ��һ���ڴ�
		{
			return ReFill(RoundUp(size));//����ȡ��
		}
		else//�У��ͷ���һ���ڴ�
		{
			__TRACE_DEBUG("��freelist[%u]λ��ȡһ���ڴ�����\n", index);
			Obj* obj = _freelist[index];
			_freelist[index] = obj->_freelistlink;

			return obj;
		}
	}

	static void Deallocate(void* p, size_t n)
	{
		if (n > __MAX_BYTES)
		{
			__TRACE_DEBUG("��һ���ռ������������ͷ�:%p %ubytes\n", p, n);
			__MallocAllocTemplate<0>::Deallocate(p, n);//�����ڴ�أ��͵���һ���ռ�������
		}
		else
		{
			size_t index = FREELIST_INDEX(n);//С���ڴ�أ��ͽ������������������
			__TRACE_DEBUG("������%p�ҵ�freelist[%u]\n", p, index);

			((Obj*)p)->_freelistlink = _freelist[index];
			_freelist[index] = (Obj*)p;
		}
	}

public:
	enum { __ALIGN = 8 };//Ĭ����8���ֽ�
	enum { __MAX_BYTES = 128 };//����Ĵ���ڴ�Ĭ����128
	enum { __NFREELISTS = __MAX_BYTES / __ALIGN };
	
	union Obj
	{
		union Obj* _freelistlink;
		char _client[1];
	};

	
	static Obj* _freelist[__NFREELISTS];
	// ����������Ϲ���

	static char* _startfree;
	static char* _endfree;
	static size_t _heapsize;
	
};

template <bool threads, int inst>
typename __DefaultAllocTemplate<threads, inst>::Obj*
__DefaultAllocTemplate<threads, inst>::_freelist[__NFREELISTS] = { 0 };

// �ڴ��
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



//������ʵ�ֵĿռ����������з�װ����Ϊ�����Ŀռ��������ĺ����������ֽ���
//������ͨ���ڿ��ٿռ�ʱ������ǲ���������


void TestDefaultAlloc()
{
	for (size_t i = 0; i < 21; ++i)
	{
		__TRACE_DEBUG("�����%u������\n", i);
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



