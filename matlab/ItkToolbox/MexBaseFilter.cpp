/* 
 * MexBaseFilter.cpp
 *
 * MexBaseFilter<InVoxelType, OutVoxelType>: This is where the code to
 * actually run the filter on the image lives.
 *
 * The reason is that template explicit specialization is only
 * possible in classes, not in functions. We need explicit
 * specialization to prevent the compiler from compiling certain
 * input/output image data types for some filters that don't accept
 * them.
 */

 /*
  * Author: Ramon Casero <rcasero@gmail.com>
  * Copyright © 2011 University of Oxford
  * Version: 0.4.1
  * $Rev$
  * $Date$
  *
  * University of Oxford means the Chancellor, Masters and Scholars of
  * the University of Oxford, having an administrative office at
  * Wellington Square, Oxford OX1 2JD, UK. 
  *
  * This file is part of Gerardus.
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details. The offer of this
  * program under the terms of the License is subject to the License
  * being interpreted in accordance with English Law and subject to any
  * action against the University of Oxford being under the jurisdiction
  * of the English Courts.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see
  * <http://www.gnu.org/licenses/>.
  */

#ifndef MEXBASEFILTER_CPP
#define MEXBASEFILTER_CPP

/* C++ headers */
#include <iostream>
#include <vector>

/* mex headers */
#include <mex.h>

/* ITK headers */

/* Gerardus headers */
#include "GerardusCommon.hpp"
#include "NrrdImage.hpp"
#include "MexBaseFilter.hpp"
#include "MexDanielssonDistanceMapImageFilter.hpp"
#include "MexSignedMaurerDistanceMapImageFilter.hpp"
#include "MexBinaryThinningImageFilter3D.hpp"

/*
 * MexBaseFilter<InVoxelType, OutVoxelType>: This is where
 * the code to actually run the filter on the image lives.
 */

// BaseFilter cannot be invoked by the user, but defining these
// static strings is necessary when we use EXCLUDEFILTER with
// derived filters
const std::string MexBaseFilter<std::string, std::string>::longname = "BaseFilter";
const std::string MexBaseFilter<std::string, std::string>::shortname = "BaseFilter";

// functions to create the ITK image, filter it and return a Matlab
// result

template <class InVoxelType, class OutVoxelType>
void MexBaseFilter<InVoxelType, OutVoxelType>::ImportMatlabInputToItkImage() {
  
  // note that:
  //
  // 1) in ITK we have X,Y,Z indices, while in Matlab we have R,C,S
  //
  // 2) matrices in ITK are read by columns, while in Matlab
  // they are read by rows 
  //
  // So imagine we have this (2, 3) matrix in Matlab, in the NRRD
  //
  //   a b   |
  //   c d   | y-axis (resolution 1.0)
  //   e f   |
  //   ---
  //   x-axis (resolution 0.5)
  //
  //   [nrrd.axis.size] = [3 2 1]
  //
  // The C-style array is going to be (reading by rows)
  //
  //   im = [a c e b d f]
  //
  // ITK is going to read by colums
  //
  //   a c e   |
  //   b d f   | y-axis (resolution 0.5)
  //   -----
  //   x-axis (resolution 1.0)
  //
  // Note that the matrix has been transposed, but this is not a
  // problem, because the resolution values have been "transposed"
  // too
  //
  // Having the matrix transposed may make us feel a bit uneasy, but
  // it has the advantage that Matlab and ITK can use the same C-style
  // array, without having to rearrange its elements

  // get pointer to input segmentation mask
  const InVoxelType *im = (InVoxelType *)mxGetData(nrrd.getData());

  // init the filter that will act as an interface between the Matlab
  // image array and the ITK filter
  importFilter = ImportFilterType::New();

  // create ITK image to hold the segmentation mask
  typename ImportFilterType::RegionType region;
  typename ImportFilterType::SizeType size;
  typename ImportFilterType::IndexType start;
  typename ImportFilterType::SpacingType spacing;
  typename InImageType::PointType origin;

  // get image parameters for each dimension
  for (mwIndex i = 0; i < Dimension; ++i) {
    // the region of interest is the whole image
    start[i] = 0;
    size[i] = nrrd.getSize()[i];
    spacing[CAST2MWSIZE(i)] = nrrd.getSpacing()[i];
    origin[CAST2MWSIZE(i)] = nrrd.getMin()[i] 
      + (nrrd.getSpacing()[i] / 2.0); // note that in NRRD, "min" is the
                                      // edge of the voxel, while in
                                      // ITK, "origin" is the centre
                                      // of the voxel

  }
  region.SetIndex(start);
  region.SetSize(size);

  // pass input region parameters to the import filter
  importFilter->SetRegion(region);
  importFilter->SetSpacing(spacing);
  importFilter->SetOrigin(origin);

  // pass pointer to Matlab image to the import filter, and tell it to
  // NOT attempt to delete the memory when it's destructor is
  // called. This is important, because the input image still has to
  // live in Matlab's memory after running the filter
  const bool importImageFilterWillOwnTheBuffer = false;
  importFilter->SetImportPointer(const_cast<InVoxelType *>(im), 
				 mxGetNumberOfElements(nrrd.getData()), 
				 importImageFilterWillOwnTheBuffer);

  importFilter->Update();
  
}

template <class InVoxelType, class OutVoxelType>
void MexBaseFilter<InVoxelType, OutVoxelType>::FilterSetup() {

  // pass image to filter
  filter->SetInput(importFilter->GetOutput());

}

template <class InVoxelType, class OutVoxelType>
void MexBaseFilter<InVoxelType, OutVoxelType>::RunFilter() {
  
  // run filter
  filter->Update();
  
}

template <class InVoxelType, class OutVoxelType>
void MexBaseFilter<InVoxelType, OutVoxelType>::CopyAllFilterOutputsToMatlab() {
  
  // by default, we assume that all filters produce at least 1 main
  // output
  this->CopyFilterImageOutputToMatlab();

  // prevent the user from asking for too many output arguments
  if (nargout > 1) {
    mexErrMsgTxt("Too many output arguments");
  }

}

template <class InVoxelType, class OutVoxelType>
void MexBaseFilter<InVoxelType, OutVoxelType>::CopyFilterImageOutputToMatlab() {

  // if the input image is empty, create empty segmentation mask for
  // output, and we don't need to do any further processing
  if (nrrd.getR() == 0 || nrrd.getC() == 0) {
    argOut[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
    return;
  }
  
  // convert output data type to output class ID
  mxClassID outputVoxelClassId = mxUNKNOWN_CLASS;
  if (TypeIsBool< OutVoxelType >::value) {
    outputVoxelClassId = mxLOGICAL_CLASS;
  } else if (TypeIsUint8< OutVoxelType >::value) {
    outputVoxelClassId = mxUINT8_CLASS;
  } else if (TypeIsUint16< OutVoxelType >::value) {
    outputVoxelClassId = mxUINT16_CLASS;
  } else if (TypeIsFloat< OutVoxelType >::value) {
    outputVoxelClassId = mxSINGLE_CLASS;
  } else if (TypeIsDouble< OutVoxelType >::value) {
    outputVoxelClassId = mxDOUBLE_CLASS;
  } else {
    mexErrMsgTxt("Assertion fail: Unrecognised output voxel type");
  }
  
  // create output matrix for Matlab's result
  argOut[0] = (mxArray *)mxCreateNumericArray( nrrd.getNdim(), 
					       nrrd.getDims(),
					       outputVoxelClassId,
					       mxREAL);
  if (argOut[0] == NULL) {
    mexErrMsgTxt("Cannot allocate memory for output matrix");
  }
  OutVoxelType *imOutp =  (OutVoxelType *)mxGetData(argOut[0]);
  
  // populate output image
  typedef itk::ImageRegionConstIterator< OutImageType > OutConstIteratorType;

  OutConstIteratorType citer(filter->GetOutput(), 
			     filter->GetOutput()->GetLargestPossibleRegion());
  mwIndex i = 0;
  for (citer.GoToBegin(), i = 0; i < nrrd.numEl(); ++citer, ++i) {
    imOutp[i] = (OutVoxelType)citer.Get();
  }
  
}

/*
 * Instantiate filter with all the input/output combinations that it
 * accepts. This is necessary for the linker. The alternative is to
 * have all the code in the header files, but this makes compilation
 * slower and maybe the executable larger
 */

#define FILTERINST(T1, T2)			\
  template class MexBaseFilter<T1, T2>;

FILTERINST(mxLogical, mxLogical)
FILTERINST(mxLogical, uint8_T)
FILTERINST(mxLogical, int8_T)
FILTERINST(mxLogical, uint16_T)
FILTERINST(mxLogical, int16_T)
FILTERINST(mxLogical, int32_T)
FILTERINST(mxLogical, int64_T)
FILTERINST(mxLogical, float)
FILTERINST(mxLogical, double)

FILTERINST(uint8_T, mxLogical)
FILTERINST(uint8_T, uint8_T)
FILTERINST(uint8_T, int8_T)
FILTERINST(uint8_T, uint16_T)
FILTERINST(uint8_T, int16_T)
FILTERINST(uint8_T, int32_T)
FILTERINST(uint8_T, int64_T)
FILTERINST(uint8_T, float)
FILTERINST(uint8_T, double)

FILTERINST(int8_T, mxLogical)
FILTERINST(int8_T, uint8_T)
FILTERINST(int8_T, int8_T)
FILTERINST(int8_T, uint16_T)
FILTERINST(int8_T, int16_T)
FILTERINST(int8_T, int32_T)
FILTERINST(int8_T, int64_T)
FILTERINST(int8_T, float)
FILTERINST(int8_T, double)

FILTERINST(uint16_T, mxLogical)
FILTERINST(uint16_T, uint8_T)
FILTERINST(uint16_T, int8_T)
FILTERINST(uint16_T, uint16_T)
FILTERINST(uint16_T, int16_T)
FILTERINST(uint16_T, int32_T)
FILTERINST(uint16_T, int64_T)
FILTERINST(uint16_T, float)
FILTERINST(uint16_T, double)

FILTERINST(int16_T, mxLogical)
FILTERINST(int16_T, uint8_T)
FILTERINST(int16_T, int8_T)
FILTERINST(int16_T, uint16_T)
FILTERINST(int16_T, int16_T)
FILTERINST(int16_T, int32_T)
FILTERINST(int16_T, int64_T)
FILTERINST(int16_T, float)
FILTERINST(int16_T, double)

FILTERINST(int32_T, mxLogical)
FILTERINST(int32_T, uint8_T)
FILTERINST(int32_T, int8_T)
FILTERINST(int32_T, uint16_T)
FILTERINST(int32_T, int16_T)
FILTERINST(int32_T, int32_T)
FILTERINST(int32_T, int64_T)
FILTERINST(int32_T, float)
FILTERINST(int32_T, double)

FILTERINST(int64_T, mxLogical)
FILTERINST(int64_T, uint8_T)
FILTERINST(int64_T, int8_T)
FILTERINST(int64_T, uint16_T)
FILTERINST(int64_T, int16_T)
FILTERINST(int64_T, int32_T)
FILTERINST(int64_T, int64_T)
FILTERINST(int64_T, float)
FILTERINST(int64_T, double)

FILTERINST(float, mxLogical)
FILTERINST(float, uint8_T)
FILTERINST(float, int8_T)
FILTERINST(float, uint16_T)
FILTERINST(float, int16_T)
FILTERINST(float, int32_T)
FILTERINST(float, int64_T)
FILTERINST(float, float)
FILTERINST(float, double)

FILTERINST(double, mxLogical)
FILTERINST(double, uint8_T)
FILTERINST(double, int8_T)
FILTERINST(double, uint16_T)
FILTERINST(double, int16_T)
FILTERINST(double, int32_T)
FILTERINST(double, int64_T)
FILTERINST(double, float)
FILTERINST(double, double)

#undef FILTERINST

#endif /* MEXBASEFILTER_CPP */
