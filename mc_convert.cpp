#include "mc_convert.h"


#ifdef HAS_OPENCV

#include "iter.h"
#include <cxcore.h>

namespace {
  template<mxClassID CID>
  IplImage* helper_2dmat_to_image (const mxArray* mat)
  {
    int M = mxGetM (mat), N = mxGetN (mat);
    const int DEPTH = mcv_traits<CID>::CV_DEPTH;
    IplImage* pOutImg = cvCreateImage (cvSize(N,M),DEPTH, 1);

    void* pBegOut;
    int pitch;
    cvGetRawData (pOutImg,(uchar**)&pBegOut,&pitch);

    typedef typename mc_traits<CID>::CT T;
    void* pBeg = mxGetData (mat);
    pix_iterator_2d<T,eColWise> it_src1(
      static_cast<T*>(pBeg), N, M);
    pix_iterator_2d<T,eColWise> it_src2(
      static_cast<T*>(pBeg), N, M);
    it_src2.end ();
    pix_iterator_2d<T,eRowWise> it_dest(
      static_cast<T*>(pBegOut), N, M, pitch);

    std::copy (it_src1,it_src2, it_dest);

    return pOutImg;
  }
  template<mxClassID CID>
  CvMat* helper_2dmat_to_cvmat (const mxArray* mat)
  {
    int M = mxGetM (mat), N = mxGetN (mat);
    const int TYPE = mcv_traits<CID>::CV_TYPE;
    CvMat* pOutMat = cvCreateMat(M, N, TYPE);

    void* pBegOut;
    int pitch;
    cvGetRawData (pOutMat,(uchar**)&pBegOut,&pitch);

    typedef typename mc_traits<CID>::CT T;
    void* pBeg = mxGetData (mat);
    pix_iterator_2d<T,eColWise> it_src1(
      static_cast<T*>(pBeg), N, M);
    pix_iterator_2d<T,eColWise> it_src2(
      static_cast<T*>(pBeg), N, M);
    it_src2.end ();
    pix_iterator_2d<T,eRowWise> it_dest(
      static_cast<T*>(pBegOut), N, M, pitch);

    std::copy (it_src1,it_src2, it_dest);

    return pOutMat;
  }
  template<int DEPTH>
  mxArray* helper_2dimage_to_mat (const IplImage* img)
  { 
    void* pBeg;
    int pitch;
    cvGetRawData(img, (uchar**)&pBeg,&pitch);

    CvSize size = cvGetSize (img);
    const mxClassID cid = cvm_traits<DEPTH>::CID;
    mxArray* pArrOut = mxCreateNumericMatrix(size.height,size.width,cid,mxREAL);
    void* pBegOut = mxGetData(pArrOut);

    typedef typename mc_traits<cid>::CT T;
    pix_iterator_2d<T,eRowWise> it_src1(static_cast<T*>(pBeg),
                                        size.width,size.height,pitch);
    pix_iterator_2d<T,eRowWise> it_src2(static_cast<T*>(pBeg),
                                        size.width,size.height,pitch);
    it_src2.end ();
    pix_iterator_2d<T,eColWise> it_dest(static_cast<T*>(pBegOut),
                                        size.width,size.height);

    std::copy (it_src1,it_src2,it_dest);

    return pArrOut;
  }
  template<int TYPE>
  mxArray* helper_2dcvmat_to_mat (const CvMat* mat)
  { 
    void* pBeg;
    int pitch;
    cvGetRawData(mat, (uchar**)&pBeg,&pitch);

    CvSize size = cvGetSize (mat);
    const mxClassID cid = cvm_traits<TYPE>::CID;
    mxArray* pArrOut = mxCreateNumericMatrix(size.height,size.width,cid,mxREAL);
    void* pBegOut = mxGetData(pArrOut);

    typedef typename mc_traits<cid>::CT T;
    pix_iterator_2d<T,eRowWise> it_src1(static_cast<T*>(pBeg),
                                        size.width,size.height,pitch);
    pix_iterator_2d<T,eRowWise> it_src2(static_cast<T*>(pBeg),
                                        size.width,size.height,pitch);
    it_src2.end ();
    pix_iterator_2d<T,eColWise> it_dest(static_cast<T*>(pBegOut),
                                        size.width,size.height);

    std::copy (it_src1,it_src2,it_dest);

    return pArrOut;
  }

  template<mxClassID CID>
  IplImage* helper_3dmat_to_image (const mxArray* mat)
  {
    const mwSize* tmp = mxGetDimensions(mat);
    int M = *tmp, N = *(tmp+1);
    const int DEPTH = mcv_traits<CID>::CV_DEPTH;
    IplImage* pOutImg = cvCreateImage (cvSize (N,M),DEPTH, 3);

    void* pBegOut;
    int pitch;
    cvGetRawData (pOutImg,(uchar**)&pBegOut,&pitch);

    typedef typename mc_traits<CID>::CT T;
    void* pBeg = mxGetData (mat);
    mxArray_iter_3d<T> it_src1(
      static_cast<T*>(pBeg), N, M, 3);
    mxArray_iter_3d<T> it_src2(
      static_cast<T*>(pBeg), N, M, 3);
    it_src2.end ();
    pix_iter_rgb<T> it_dest(
      static_cast<T*>(pBegOut), N, M, pitch);

    std::copy (it_src1,it_src2, it_dest);

    return pOutImg;
  }
  template<int DEPTH>
  mxArray* helper_rgbimage_to_mat (const IplImage* img)
  {
    const int ndim = 3;
    CvSize size = cvGetSize (img);
    mwSize dims[3];
    dims[0] = size.height; dims[1] = size.width; dims[2] = 3;
    const mxClassID cid = cvm_traits<DEPTH>::CID;
    mxArray* pArrOut = mxCreateNumericArray(ndim, dims, cid, mxREAL);
    
    if (img->nChannels==3) {
      void* pBeg;
      int pitch;
      cvGetRawData(img, (uchar**)&pBeg,&pitch);
      void* pBegOut = mxGetData(pArrOut);

      typedef typename mc_traits<cid>::CT T;
      pix_iter_rgb<T> it_src1(static_cast<T*>(pBeg),
        size.width,size.height,pitch);
      pix_iter_rgb<T> it_src2(static_cast<T*>(pBeg),
        size.width,size.height,pitch);
      it_src2.end ();
      mxArray_iter_3d<T> it_dest(static_cast<T*>(pBegOut),
        size.width, size.height, 3);

      std::copy (it_src1,it_src2,it_dest);
    }

    return pArrOut;
  }
} // namespace

mxArray* IplImage_to_new_mxArr (const IplImage* img)
{
  const int TYPE = cvGetElemType (img);

  // 2-d image
  if (CV_64FC1 == TYPE) {
    return helper_2dimage_to_mat<IPL_DEPTH_64F> (img);
  }
  else if (CV_32FC1 == TYPE) {
    return helper_2dimage_to_mat<IPL_DEPTH_32F> (img);
  }
  else if (CV_8UC1 == TYPE) {
    return helper_2dimage_to_mat<IPL_DEPTH_8U> (img);
  }

  // 3-d image
  else if (CV_64FC3 == TYPE) {
    return helper_rgbimage_to_mat<IPL_DEPTH_64F> (img);
  }
  else if (CV_32FC3 == TYPE) {
    return helper_rgbimage_to_mat<IPL_DEPTH_32F> (img);
  }
  else if (CV_8UC3 == TYPE) {
    return helper_rgbimage_to_mat<IPL_DEPTH_8U> (img);
  }

  // unsupported conversion, return null mxArray
  return mxCreateDoubleMatrix(0,0,mxREAL); 
}

mxArray* CvMat_to_new_mxArr (const CvMat* mat)
{
  const int TYPE = cvGetElemType (mat);

  // 2-d image
  if (CV_64FC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_64FC1> (mat);
  }
  else if (CV_32FC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_32FC1> (mat);
  }
  else if (CV_32SC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_32SC1> (mat);
  }
  else if (CV_16SC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_16SC1> (mat);
  }
  else if (CV_16UC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_16UC1> (mat);
  }
  else if (CV_8UC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_8UC1> (mat);
  }
  else if (CV_8SC1 == TYPE) {
    return helper_2dcvmat_to_mat<CV_8SC1> (mat);
  }
  //Multi-dimensional arrays not supported, yet.
/*
  // 3-d image
  else if (CV_64FC3 == TYPE) {
    return helper_rgbimage_to_mat<IPL_DEPTH_64F> (img);
  }
  else if (CV_32FC3 == TYPE) {
    return helper_rgbimage_to_mat<IPL_DEPTH_32F> (img);
  }
  else if (CV_8UC3 == TYPE) {
    return helper_rgbimage_to_mat<IPL_DEPTH_8U> (img);
  }*/

  // unsupported conversion, return null mxArray
  return mxCreateDoubleMatrix(0,0,mxREAL); 
  
}


IplImage* mxArr_to_new_IplImage (const mxArray* mat)
{
  // TODO: other types!!!
  const mxClassID id = mxGetClassID(mat);
  mwSize const ndim = mxGetNumberOfDimensions(mat);
  if (2==ndim) {
    if (mxDOUBLE_CLASS == id) {
      return helper_2dmat_to_image<mxDOUBLE_CLASS> (mat);
    }
    else if (mxSINGLE_CLASS == id) {
      return helper_2dmat_to_image<mxSINGLE_CLASS> (mat);
    }
    else if (mxINT32_CLASS == id) {
      return helper_2dmat_to_image<mxINT32_CLASS> (mat);
    }
    else if (mxINT16_CLASS == id) {
      return helper_2dmat_to_image<mxINT16_CLASS> (mat);
    }
    else if (mxUINT16_CLASS == id) {
      return helper_2dmat_to_image<mxUINT16_CLASS> (mat);
    }
    else if (mxINT8_CLASS == id) {
      return helper_2dmat_to_image<mxINT8_CLASS> (mat);
    }
    else if (mxUINT8_CLASS == id) {
      return helper_2dmat_to_image<mxUINT8_CLASS> (mat);
    }
  }
  else if (3==ndim) {
    if (mxDOUBLE_CLASS == id) {
      return helper_3dmat_to_image<mxDOUBLE_CLASS> (mat);
    }
    else if (mxSINGLE_CLASS == id) {
      return helper_3dmat_to_image<mxSINGLE_CLASS> (mat);
    }
    else if (mxUINT8_CLASS == id) {
      return helper_3dmat_to_image<mxUINT8_CLASS> (mat);
    }
  }

  // unsupported conversion, return null IplImage
  return cvCreateImage (cvSize (0,0), IPL_DEPTH_8U,1); 
}
CvMat* mxArr_to_new_CvMat (const mxArray* mat)
{
  // TODO: other types!!!
  const mxClassID id = mxGetClassID(mat);
  mwSize const ndim = mxGetNumberOfDimensions(mat);
  if (2==ndim) {
    if (mxDOUBLE_CLASS == id) {
      return helper_2dmat_to_cvmat<mxDOUBLE_CLASS> (mat);
    }
    else if (mxSINGLE_CLASS == id) {
      return helper_2dmat_to_cvmat<mxSINGLE_CLASS> (mat);
    }
    else if (mxINT32_CLASS == id) {
      return helper_2dmat_to_cvmat<mxINT32_CLASS> (mat);
    }
    else if (mxINT16_CLASS == id) {
      return helper_2dmat_to_cvmat<mxINT16_CLASS> (mat);
    }
    else if (mxUINT16_CLASS == id) {
      return helper_2dmat_to_cvmat<mxUINT16_CLASS> (mat);
    }
    else if (mxINT8_CLASS == id) {
      return helper_2dmat_to_cvmat<mxINT8_CLASS> (mat);
    }
    else if (mxUINT8_CLASS == id) {
      return helper_2dmat_to_cvmat<mxUINT8_CLASS> (mat);
    }
    else if (mxLOGICAL_CLASS == id) {
			return helper_2dmat_to_cvmat<mxLOGICAL_CLASS> (mat);
		}
  }
  //MultiChannel arrays not supported, yet.
  /*else if (3==ndim) {
    if (mxDOUBLE_CLASS == id) {
      return helper_3dmat_to_image<mxDOUBLE_CLASS> (mat);
    }
    else if (mxSINGLE_CLASS == id) {
      return helper_3dmat_to_image<mxSINGLE_CLASS> (mat);
    }
    else if (mxUINT8_CLASS == id) {
      return helper_3dmat_to_image<mxUINT8_CLASS> (mat);
    }
  }*/

  // unsupported conversion, return null CvMat
  return cvCreateMat (0, 0, CV_8UC1); 
}

/* Deprecated */
IplImage* mat_to_new_image (const mxArray* mat)
{
  return mxArr_to_new_IplImage(mat);
}
mxArray* image_to_new_mat (const IplImage* img)
{
  return IplImage_to_new_mxArr(img);
}
#endif // HAS_OPENCV