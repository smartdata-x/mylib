#ifndef SCOPED_PTR_H__
#define SCOPED_PTR_H__
#include <assert.h>
#include "basictypes.h"
#include "check/newmacro.h"

template<class C>
class scoped_ptr{
public:
	typedef C elemet_type;

	explicit scoped_ptr(C* p = NULL): ptr_(p){}

	~scoped_ptr(){
		enum{type_must_be_complete = sizeof(C)};
		if (ptr_ != NULL)
		  delete ptr_;
	}

	void reset(C* p = NULL){
		if(p!=ptr_){
			enum{ type_must_be_complete = sizeof(C)};
			if (ptr_ != NULL)
			  delete ptr_;
			ptr_ = p;
		}
	}

	C& operator*() const{
		assert(ptr_!=0);
		return *ptr_;
	}

	C* operator->() const{
		assert(ptr_!=0);
		return ptr_;
	}
	C* get() const {return ptr_;}

	bool operator==(C* p) const{return ptr_ == p;}
	bool operator!=(C* p) const{return ptr_ != p;}

	void swap(C* p2){
		C* temp_ptr = ptr_;
		ptr_ = p2;
		p2 = temp_ptr;
	}

	C*  release(void){
		C* ret_val = ptr_;
		ptr_ = NULL;
		return ret_val;
	}

private:
	C* ptr_;

	template <class C2> bool operator==(scoped_ptr<C2> const& p2) const;
	template <class C2> bool operator!=(scoped_ptr<C2> const& p2) const;

	DISALLOW_COPY_AND_ASSIGN(scoped_ptr);
};
#endif
