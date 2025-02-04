
#ifndef __HANDLE_H__
#define __HANDLE_H__

////////////////////////////////////////////////////////////////////
// Автоматически удаляемый указатель
////////////////////////////////////////////////////////////////////
template<class T>
class PtrHandle 
{
public:
	PtrHandle(T* p = 0) : ptr(p) {}
    PtrHandle(const PtrHandle& p) : ptr(p.release()) {}
	~PtrHandle() { delete ptr; }

	void set(T* p) { ptr = p; }

	PtrHandle& operator=(const PtrHandle& p) 
	{ 
		if (get() != p.get()) 
		{ 
			delete ptr; 
			ptr = p.release(); 
		} 
		return *this; 
	}

	PtrHandle& operator=(T* p) 
	{
		if(get() != p) 
			delete ptr;
		set(p);
		return *this;
	}

	T* get() const { return ptr; }
	T* release() const { T* tmp = ptr; ptr = 0; return tmp; }
  
	T* operator->() const {return ptr;}
	T& operator*() const {return *ptr;}
	T* operator() () const {return ptr;}
	operator T* () const { return ptr; }

private:
	mutable T* ptr;
};

////////////////////////////////////////////////////////////////////
// Автоматически удаляемый указатель
// с отслеживанием владельцев.
// Обекты должны наследовать ShareHandleBase 
////////////////////////////////////////////////////////////////////
template<class T>
class ShareHandle {
public:
	ShareHandle(T *p = 0) { set(p); }
	ShareHandle(const ShareHandle& orig) { set(orig.ptr); }
	
	~ShareHandle() 
	{ 
		if(ptr && !ptr->decrRef()) 
			delete ptr; 
	}

	void set(T *p) 
	{ 
		ptr = p; 
		if(p) 
			p->addRef(); 
	}
  
	ShareHandle& operator=(const ShareHandle& orig) 
	{
		if (ptr && !ptr->decrRef() && ptr != orig.ptr) 
			delete ptr;
		set(orig.ptr);
		return *this;
	}
	
	ShareHandle& operator=(T* p) 
	{
		if (ptr && !ptr->decrRef() && ptr != p) 
			delete ptr;
		set(p);
		return *this;
	}
  
	T* get() const { return ptr; }
	T* release() { T* tmp = ptr; if (ptr) ptr->decrRef(); ptr = 0; return tmp; }
  
	T* operator->() const { return ptr; }
	T& operator*() const { return *ptr; }
	T* operator() () const { return ptr; }

#ifndef _SAFE_PTR_
	operator T* () const { return ptr; }
#else
	typedef ShareHandle<T> ThisType;
    typedef T* (ThisType::*unspecified_bool_type)() const;
	
    operator unspecified_bool_type() const // never throws
    {
        return ptr == 0? 0: &ThisType::get;
    }
#endif

private:
	T *ptr;
};

class ShareHandleBase 
{
public:
	ShareHandleBase() { handleCount = 0; }
	ShareHandleBase(const ShareHandleBase&) { handleCount = 0; }
	~ShareHandleBase(){}
	void addRef() { ++handleCount; }
	int decrRef() { return --handleCount; }
	int numRef() const { return handleCount; }

private:
	mutable int handleCount;
};

#endif // __HANDLE_H__
