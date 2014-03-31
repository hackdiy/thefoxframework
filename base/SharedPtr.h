/*
 * @filename SharedPtr.h
 * @brief 基于引用计数的智能指针，是std::shared_ptr的仿制品
 * @author  macwe@qq.com
 */
#ifndef _THEFOX_BASE_SHAREDPTR_H_ 
#define _THEFOX_BASE_SHAREDPTR_H_

namespace thefox  
{

template<typename T>	
class SharedPtr
{
public:
	SharedPtr()
    : _pointer(NULL)
    {}
    
    SharedPtr(T *p = NULL)
	: _pointer(p)
	{
        if (NULL != _pointer)
            _pointer->addRef();
    }
    
	SharedPtr(const SharedPtr &rsp)
	: _pointer(rsp._pointer)
	{
		if (NULL != _pointer)
            _pointer->addRef();
	}
    
	~SharedPtr()
	{
		if (NULL != _pointer) {
			if (0 == _pointer->release())
				safeDelete(_pointer);
	}
	
	SharedPtr &operator=(const SharedPtr &rsp)
	{
        if (_pointer != rsp->_pointer) {
            if (NULL != _pointer) {
                if (0 == _pointer->release())
                    safeDelete(_pointer);
            }
            _pointer = rsp._pointer;
            _pointer->addRef();
        }
		return *this;
	}
        
	T &operator*()
	{ return *_pointer; }
	
	T *operator->()
	{ return _pointer;}
	
	operator bool()
	{ return NULL != _pointer; }
	
	bool operator==(const SharedPtr &rsp)
	{ return _pointer == rsp._pointer; }
	
	size_t useCount() const _pointer
	{ return (NULL != _pointer) ? _pointer->useCount() : 0; }
	
private:
	T *_pointer;
};

template<typename T>
inline SharedPtr<T> makeShared(T *p)
{
    return SharedPtr<T>(p);
}
    
} // namespace thefox;

#endif // _THEFOX_BASE_SHAREDPTR_H_
