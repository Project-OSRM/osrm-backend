#if !defined(spp_smartptr_h_guard)
#define spp_smartptr_h_guard


/* -----------------------------------------------------------------------------------------------
 * quick version of intrusive_ptr
 * -----------------------------------------------------------------------------------------------
 */

#include <cassert>
#include <sparsepp/spp_config.h>

// ------------------------------------------------------------------------
class spp_rc
{
public:
    spp_rc() : _cnt(0) {}
    spp_rc(const spp_rc &) : _cnt(0) {}
    void increment() const { ++_cnt; }
    void decrement() const { assert(_cnt); if (--_cnt == 0) delete this; }
    unsigned count() const { return _cnt; }

protected:
    virtual ~spp_rc() {}

private:
    mutable unsigned _cnt;
};

// ------------------------------------------------------------------------
template <class T>
class spp_sptr
{
public:
    spp_sptr() : _p(0) {}
    spp_sptr(T *p) : _p(p)                  { if (_p) _p->increment(); }
    spp_sptr(const spp_sptr &o) : _p(o._p)  { if (_p) _p->increment(); }
#ifndef SPP_NO_CXX11_RVALUE_REFERENCES 
    spp_sptr(spp_sptr &&o) : _p(o._p)       { o._p = (T *)0; }
    spp_sptr& operator=(spp_sptr &&o)
    {
        if (_p) _p->decrement(); 
        _p = o._p;
        o._p = (T *)0; 
    }
#endif    
    ~spp_sptr()                             { if (_p) _p->decrement(); }
    spp_sptr& operator=(const spp_sptr &o)  { reset(o._p); return *this; }
    T* get() const                          { return _p; }
    void swap(spp_sptr &o)                  { T *tmp = _p; _p = o._p; o._p = tmp; }
    void reset(const T *p = 0)             
    { 
        if (p == _p) 
            return; 
        if (_p) _p->decrement(); 
        _p = (T *)p; 
        if (_p) _p->increment();
    }
    T*   operator->() const { return const_cast<T *>(_p); }
    bool operator!()  const { return _p == 0; }

private:
    T *_p;
};    

// ------------------------------------------------------------------------
namespace std
{
    template <class T>
    inline void swap(spp_sptr<T> &a, spp_sptr<T> &b)
    {
        a.swap(b);
    }
}

#endif // spp_smartptr_h_guard
