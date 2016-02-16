/** 
 * matlab_dump.h
 * dump C++ data into matlab environment in run-time (using matlab engine)
 */

#ifndef matlab_dump_h__ 
#define matlab_dump_h__

#include "types.h"
#include "iter.h"

#include <engine.h>
#include <string>
#include <iterator>
#include <algorithm>
#ifdef HAS_OPENCV
#  include <cxcore.h>
#endif // HAS_OPENCV

#ifdef _MSC_VER
#  if _MSC_VER < 1300
#    error: connot be complied by VC6. VC7 or later is needed.
#  endif
#endif // _MSC_VER

namespace matlab_dbg {

namespace detail {

using namespace std;

// engine_wrapper
// simple wrapper of matlab engine APIs
// see MATLAB on-line help:
//   MATLAB->C and Fortran Functions-By Category->MATLAB engine
// for details 
// matlab engine API 的简单包装
class engine_wrapper
{
public:
  // do-not lock on default
  engine_wrapper (bool islock = false) 
    : varname_("temp__"), ishold_(false), islock_(islock) 
  {
    default_size ();
    pEngine_ = engOpen("\0");
  }

  // inject [itFirst, itLast) into MATLAB workspace as matrix
  // need forward iterator or more refined one
  template<class iter_t>
  bool inject (iter_t itFirst, iter_t itLast)
  {
    if (lock ()) return false;

    typedef iterator_traits<iter_t>::value_type T;
    const mxClassID CID =  cm_traits<iterator_traits<iter_t>::value_type>::CID;
    
    unsigned int height = 
      static_cast<unsigned int>(distance(itFirst, itLast)) / dim_[ePage] / dim_[eWidth];
    dim_[eHeight] = height>0 ? height:1;
    mxArray* tempMatrix = mxCreateNumericArray (
      3, dim_, 
      CID, mxREAL);
    mxArray_iter_3d<T> pd ((T*)(mxGetPr(tempMatrix)),
                            dim_[eWidth], dim_[eHeight], dim_[ePage]);

    copy (itFirst, itLast, pd);

    int status = engPutVariable(pEngine_, 
      valid_varname (), 
      tempMatrix);

    mxDestroyArray(tempMatrix);
    if (!hold()) default_size ();
    return (status == 0);
  }

  // inject existing mxArray
  bool inject (const mxArray* p)
  {
    int status = engPutVariable(pEngine_, 
      valid_varname (), p);
    return status>=0;
  }
  // evaluate valid command line in MATLAB workspace
  bool eval_str (const string& command) 
  {
    if (lock ()) return false;
    return engEvalString(pEngine_, command.c_str ()) == 0;
  }

  // width, a.k.a column of matrix 
  void         width (unsigned int rhs) {dim_[eWidth] = (rhs>0 ? rhs:1);}
  unsigned int width ()                 {return dim_[eWidth];}

  // height, a.k.a row of matrix
  void         height (unsigned int rhs) {dim_[eHeight] = (rhs>0 ? rhs:1);}
  unsigned int height ()                 {return dim_[eHeight];}

  // pages of matrix
  void         npage (unsigned int rhs) {dim_[ePage] = (rhs>0 ? rhs:1);}
  unsigned int npage ()                 {return dim_[ePage];}

  // name of the to-be-injected matrix
  void   name (const string& rhs) {varname_=rhs;}
  string name ()                  {return varname_;}

  // hold the paras(width, name,...)?
  void hold (bool rhs) {ishold_ = rhs;}
  bool hold ()         {return ishold_;}

  // lock the flow? If true, no data will flow to MATLAB workspace by invoking
  // inject() method. 
  // NOTE: the paras(width, name,...) is not controled by this flag
  void lock (bool rhs) {islock_ = rhs;}
  bool lock ()         {return islock_;}
  
protected:
private:
  engine*   pEngine_;

  enum eDim    {eHeight = 0,  eWidth= 1, ePage = 2};
  mwSize dim_[3];  // 暂时只支持3维up to 3-D is supported currently
  string       varname_;
  
  bool ishold_; // 保持参数么？hold the paras(width, name,...)?
  bool islock_; // lock the flow?

  // 返回有效的变量名，默认为"temp__"
  // return valid MATLAB variable names. default to "temp__"
  const char* valid_varname () {
    return varname_.empty() ? "temp__" : varname_.c_str();
  }

  void default_size () {
    dim_[eHeight] = dim_[eWidth] = dim_[ePage] = 1;
  }

};


// manipulators

// manipulators with void arg
inline engine_wrapper& 
operator << (engine_wrapper& ew, engine_wrapper& (*fcn) (engine_wrapper&))
{
  return (*fcn) (ew);
}

// manipulators with one arg
template<class T>
class fcn_unary
{
public:
  fcn_unary (engine_wrapper& (*fcn) (engine_wrapper&, T), T val) 
    : fcn_(fcn), val_(val) {}

  engine_wrapper& operator () (engine_wrapper& ew) const {
    return fcn_ (ew, val_);
  }
private:
  engine_wrapper& (*fcn_) (engine_wrapper&, T);
  T val_;
};

template<class T>
inline engine_wrapper& operator << (engine_wrapper& ew, const fcn_unary<T>& fcn) 
{
  return fcn(ew);
}


// set name of the to-be-injected matrix
inline engine_wrapper& setname (engine_wrapper& ew, const string& name)
{
  ew.name(name);
  return ew;
}

inline fcn_unary<const string&> name (const string& name) {
  return fcn_unary<const string&> (setname, name);
}

// set width, a.k.a column, of the to-be-injected matrix
inline engine_wrapper& setwidth (engine_wrapper& ew, unsigned int w) 
{
  ew.width (w);
  return ew;
}

inline fcn_unary<unsigned int> width (unsigned int w) {
  return fcn_unary<unsigned int> (setwidth, w);
}

// set height, a.k.a row, of the to-be-injected matrix
inline engine_wrapper& setheight (engine_wrapper& ew, unsigned int h)
{
  ew.height (h);
  return ew;
}

inline fcn_unary<unsigned int> height (unsigned int h) {
  return fcn_unary<unsigned int> (setheight, h);
}
// set pages, a.k.a the 3rd dimension, of the to-be-injected matrix
inline engine_wrapper& setpage (engine_wrapper& ew, unsigned int p)
{
  ew.npage (p);
  return ew;
}

inline fcn_unary<unsigned int> page (unsigned int p) {
  return fcn_unary<unsigned int> (setpage, p);
}

inline fcn_unary<unsigned int> channel (unsigned int c) { // no more than an alias
  return fcn_unary<unsigned int> (setpage, c);
}


// hold paras(width, name,...)?
inline engine_wrapper& sethold (engine_wrapper& ew, bool b)
{
  ew.hold (b);
  return ew;
}

inline fcn_unary<bool> hold (bool b) {
  return fcn_unary<bool> (sethold, b);
}

// lock the flow? (to save compiling time in conjunction with some technique)
inline engine_wrapper& setlock (engine_wrapper& ew, bool b)
{
  ew.lock (b);
  return ew;
}

inline fcn_unary<bool> lock (bool b) {
  return fcn_unary<bool> (setlock, b);
}

// set beginning position of the to-be-injected matrix
template<class iter_t>
inline engine_wrapper& setstart (engine_wrapper& ew, iter_t itStart)
{
  ew.inject (itStart, itStart+ew.width()*ew.height()*ew.npage());
  return ew;
}

template<class iter_t>
inline fcn_unary<iter_t> start (iter_t itStart) {
  return fcn_unary<iter_t> (setstart, itStart);
}



// set valid command line to be executed in MATLAB workspace
inline engine_wrapper& setcmd (engine_wrapper& ew, const string& command)
{
  ew.eval_str (command);
  return ew;
}

inline fcn_unary<const string&> cmd (const string& command) {
  return fcn_unary<const string&> (setcmd, command);
}

// provide easy access to certain type. need specialization.
template<typename T>
engine_wrapper& setvar (engine_wrapper& ew, T v);
template<typename T>
inline fcn_unary<T> var (T v) {
  return fcn_unary<T> (setvar, v);
}

#ifdef HAS_OPENCV

// short-cut to dump IplImage
template<>
engine_wrapper& setvar<IplImage*> (engine_wrapper& ew, IplImage* p);
// short-cut to dump CvMat
template<>
engine_wrapper& setvar<CvMat*> (engine_wrapper& ew, CvMat* p);
#endif // HAS_OPENCV

// manipulators with two args
template<class T>
class fcn_binary
{
public:
  fcn_binary (engine_wrapper& (*fcn) (engine_wrapper&, T, T), T val1, T val2) 
    : fcn_(fcn), val1_(val1), val2_(val2) {}

    engine_wrapper& operator () (engine_wrapper& ew) const {
      return fcn_ (ew, val1_, val2_);
    }
private:
  engine_wrapper& (*fcn_) (engine_wrapper&, T, T);
  T val1_, val2_;
};

template<class T>
inline engine_wrapper& operator << (engine_wrapper& ew,  const fcn_binary<T>& fcn) 
{
  return fcn(ew);
}

// set range (in [beg, end) form) of the to-be-injected matrix
template<class iter_t>
inline engine_wrapper& setrange (engine_wrapper& ew, iter_t itFirst, iter_t itLast) 
{
  ew.inject (itFirst, itLast);
  return ew;
}

template<class iter_t>
inline fcn_binary<iter_t> range (iter_t itFirst, iter_t itLast) {
  return fcn_binary<iter_t> (setrange, itFirst, itLast);
}

} // namespace matlab_dbg::detail


// external interface
using detail::engine_wrapper;

using detail::name;
using detail::width;
using detail::height;
using detail::page;
using detail::channel;
using detail::hold;
using detail::lock;
using detail::cmd;
using detail::var;


using detail::range;
using detail::start;


} // namespace matlab_dbg


// a global variabe similar to cout, clog
extern matlab_dbg::engine_wrapper matlab;


#endif // matlab_dump_h__ 