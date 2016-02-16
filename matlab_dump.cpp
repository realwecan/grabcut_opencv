/*M///////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2006, 
// gauss.1982@gmail.com, MutiMedia Centre, Beijing University of Post and Telecommunications, 
// all rights reserved.
// 
// Any suggestion/bug report is strongly welcome.
//
//M*/

#include "matlab_dump.h"

// edit "islock" manually here.
// this technique will save considerable compiling time if you want
// to prevent all your data flowing to matlab temporarily
matlab_dbg::engine_wrapper matlab (/*islock = */ false);

#include "mc_convert.h"

namespace matlab_dbg {
  namespace detail {

  #ifdef HAS_OPENCV


  template<>
  engine_wrapper& setvar<IplImage*> (engine_wrapper& ew, IplImage* p)
  {
    mxArray* pp = ::image_to_new_mat (p);
    ew.inject(pp);
    mxDestroyArray(pp);
    return ew;
  }

  template<>
  engine_wrapper& setvar<CvMat*> (engine_wrapper& ew, CvMat* p)
  {
    mxArray* pp = ::CvMat_to_new_mxArr (p);
    ew.inject(pp);
    mxDestroyArray(pp);
    return ew;
  }

  #endif // HAS_OPENCV

  } // namespace detail
} // namespace matlab_dbgs
