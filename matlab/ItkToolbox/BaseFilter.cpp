/* 
 * BaseFilter.cpp
 *
 * BaseFilter<InVoxelType, OutVoxelType, FilterType>: This is where
 * the code to actually run the filter on the image lives.
 *
 * Instead of having a function (e.g. runFilter), we have the code in
 * the constructor of class FilterFactory.
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
  * Version: 0.1.0
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

#ifndef BASEFILTER_CPP
#define BASEFILTER_CPP

/* C++ headers */
#include <iostream>
#include <vector>

/* mex headers */
#include <mex.h>

/* ITK headers */
#include "itkImage.h"
#include "itkBinaryThinningImageFilter3D.h"
#include "itkDanielssonDistanceMapImageFilter.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"

/* Gerardus headers */
#import "NrrdImage.hpp"
#import "BaseFilter.hpp"

/*
 * parseInputTypeToTemplate()
 * parseOutputTypeToTemplate< InVoxelType >()
 * parseFilterTypeToTemplate< InVoxelType, OutVoxelType >()
 *
 * These functions are used to be able to map between the input/output
 * data types that are only know at run-time, and the input/output
 * data templates that ITK requires and must be know at compilation
 * time.
 *
 * To avoid a nesting nightmare:
 *
 * switch FilterType {
 *   switch InputDataType {
 *     switch OutputDataType {
 *     }
 *   }
 * }
 *
 * we split the conversion in 3 steps. The first function,
 * parseInputTypeToTemplate() reads the input data type and the
 * filter type, and instantiates
 * parseOutputTypeToTemplate<InVoxelType>() for each input data
 * type.
 *
 * This way, we have effectively mapped the input type from a run-time
 * variable to a set of compilation time templates.
 *
 * Then parseOutputTypeToTemplate<InVoxelType>() decides on the
 * output data type depending on the filter and the input, and
 * instantiates one
 * parseFilterTypeToTemplate<InVoxelType, OutVoxelType>() for each
 * output data type.
 *
 * This way, we have instantiated all combinations of input/output
 * data types, without having to code them explicitly.
 *
 * Finally, parseFilterTypeToTemplate<InVoxelType, OutVoxelType>()
 * checks that the requested filter is implemented and maps the filter
 * variable to all filter templates.
 *
 * Note that the actual filtering is done in the constructor of
 * BaseFilter, that is instantiated from
 * parseFilterTypeToTemplate.
 */

// parseFilterTypeToTemplate<InVoxelType, OutVoxelType>()
template <class InVoxelType, class OutVoxelType>
void parseFilterTypeToTemplate(char *filter,
			       NrrdImage nrrd,
			       int nargout,
			       mxArray** &argOut) {

  // image type definitions
  typedef double TScalarType; // data type for scalars
  typedef itk::Image< InVoxelType, Dimension > 
    InImageType;
  typedef itk::Image< OutVoxelType, Dimension > 
    OutImageType;


  // convert run-time filter string to template
  if (!strcmp(filter, "skel")) {
    BaseFilter<InVoxelType, OutVoxelType, 
      itk::BinaryThinningImageFilter3D< InImageType, OutImageType >
      > filterFactory(filter, nrrd, nargout, argOut);
  }  else if (!strcmp(filter, "dandist")) {
    BaseFilter<InVoxelType, OutVoxelType, 
      itk::DanielssonDistanceMapImageFilter< InImageType, OutImageType >
      > filterFactory(filter, nrrd, nargout, argOut);
  }  else if (!strcmp(filter, "maudist")) {
    BaseFilter<InVoxelType, OutVoxelType, 
      itk::SignedMaurerDistanceMapImageFilter< InImageType, OutImageType >
      > filterFactory(filter, nrrd, nargout, argOut);
  } else {
    mexErrMsgTxt("Filter type not implemented");
  }
  
}

// parseOutputTypeToTemplate<InVoxelType>()
template <class InVoxelType>
void parseOutputTypeToTemplate(char *filter,
			       NrrdImage nrrd,
			       int nargout,
			       mxArray** &argOut) {

  // make it easier to remember the different cases for the output
  // voxel type
  enum OutVoxelType {
    SAME, BOOL, UINT8, UINT16, SINGLE, DOUBLE
  };

  // establish output voxel type according to the filter
  OutVoxelType outVoxelType = DOUBLE;
  if (!strcmp(filter, "skel")) {

    outVoxelType = SAME;

  } else if (!strcmp(filter, "dandist")) {
    // find how many bits we need to represent the maximum distance
    // that two voxels can have between them (in voxel units)
    mwSize nbit = (mwSize)ceil(log2(nrrd.maxVoxDistance()));

    // select an output voxel size enough to save the maximum distance
    // value
    if (nbit <= 2) {
      outVoxelType = BOOL;
    } else if (nbit <= 8) {
      outVoxelType = UINT8;
    } else if (nbit <= 16) {
      outVoxelType = UINT16;
    } else if (nbit <= 128) {
      outVoxelType = SINGLE;
    } else {
      outVoxelType = DOUBLE;
    }
    
  } else if (!strcmp(filter, "maudist")) {

    outVoxelType = DOUBLE;

  } else {
    mexErrMsgTxt("Filter type not implemented");
  }

  switch(outVoxelType) {
  case SAME:
    parseFilterTypeToTemplate<InVoxelType, 
      InVoxelType>(filter, nrrd, nargout, argOut);
    break;
  case BOOL:
    parseFilterTypeToTemplate<InVoxelType, 
      bool>(filter, nrrd, nargout, argOut);
    break;
  case UINT8:
    parseFilterTypeToTemplate<InVoxelType, 
      uint8_T>(filter, nrrd, nargout, argOut);
    break;
  case UINT16:
    parseFilterTypeToTemplate<InVoxelType, 
      uint16_T>(filter, nrrd, nargout, argOut);
    break;
  case SINGLE:
    parseFilterTypeToTemplate<InVoxelType, 
      float>(filter, nrrd, nargout, argOut);
    break;
  case DOUBLE:
    parseFilterTypeToTemplate<InVoxelType, 
      double>(filter, nrrd, nargout, argOut);
    break;
  default:
    mexErrMsgTxt("Invalid output type.");
    break;
  }
}

// parseInputTypeToTemplate()
void parseInputTypeToTemplate(mxClassID inputVoxelClassId, 
			      char *filter,
			      NrrdImage nrrd,
			      int nargout,
			      mxArray** &argOut) {
  
  switch(inputVoxelClassId)  { // swith input image type
  case mxLOGICAL_CLASS:
    parseOutputTypeToTemplate<bool>(filter, nrrd, nargout, argOut);
    break;
  case mxDOUBLE_CLASS:
    parseOutputTypeToTemplate<double>(filter, nrrd, nargout, argOut);
    break;
  case mxSINGLE_CLASS:
    parseOutputTypeToTemplate<float>(filter, nrrd, nargout, argOut);
    break;
  case mxINT8_CLASS:
    parseOutputTypeToTemplate<int8_T>(filter, nrrd, nargout, argOut);
    break;
  case mxUINT8_CLASS:
    parseOutputTypeToTemplate<uint8_T>(filter, nrrd, nargout, argOut);
    break;
  case mxINT16_CLASS:
    parseOutputTypeToTemplate<int16_T>(filter, nrrd, nargout, argOut);
    break;
  case mxUINT16_CLASS:
    parseOutputTypeToTemplate<uint16_T>(filter, nrrd, nargout, argOut);
    break;
  case mxINT32_CLASS:
    parseOutputTypeToTemplate<int32_T>(filter, nrrd, nargout, argOut);
    break;
  // case mxUINT32_CLASS:
  //   break;
  case mxINT64_CLASS:
    parseOutputTypeToTemplate<int64_T>(filter, nrrd, nargout, argOut);
    break;
  // case mxUINT64_CLASS:
  //   break;
  case mxUNKNOWN_CLASS:
    mexErrMsgTxt("Input matrix has unknown type.");
    break;
  default:
    mexErrMsgTxt("Input matrix has invalid type.");
    break;
  }
}

/*
 * BaseFilter<InVoxelType, OutVoxelType, FilterType>: This is where
 * the code to actually run the filter on the image lives.
 *
 * Instead of having a function (e.g. runFilter), we have the code in
 * the constructor of class FilterFactory.
 *
 * The reason is that template explicit specialization is only
 * possible in classes, not in functions. We need explicit
 * specialization to prevent the compiler from compiling certain
 * input/output image data types for some filters that don't accept
 * them.
 */

// Constructor: where the actual filtering code lives
template <class InVoxelType, class OutVoxelType, class FilterType>
BaseFilter<InVoxelType, OutVoxelType, FilterType>::BaseFilter
(char *filterName, NrrdImage &nrrd, int _nargout, mxArray** &argOut) 
  : nargout(_nargout) {
  
  // if the input image is empty, create empty segmentation mask for
  // output. We don't need to do any processing
  if (nrrd.getR() == 0 || nrrd.getC() == 0) {
    argOut[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
    return;
  }
  
  // get pointer to input segmentation mask
  const InVoxelType *im = (InVoxelType *)mxGetPr(nrrd.getData());

  // image type definitions
  typedef double TScalarType; // data type for scalars
  typedef itk::Image< InVoxelType, Dimension > 
    InImageType;
  typedef itk::Image< OutVoxelType, Dimension > 
    OutImageType;
  typedef itk::ImageRegionIterator< InImageType > 
    InIteratorType;
  typedef itk::ImageRegionConstIterator< OutImageType > 
    OutConstIteratorType;
  
  // create ITK image to hold the segmentation mask
  typename InImageType::Pointer image = InImageType::New();
  typename InImageType::RegionType region;
  typename InImageType::IndexType start;
  typename InImageType::SizeType size; 
  typename InImageType::SpacingType spacing;
  
  // note that:
  //
  // 1) in ITK we have X,Y,Z indices, while in Matlab we have R,C,S
  //
  // 2) matrices in ITK are read by columns, while in Matlab
  // they are read by rows, 
  //
  // So both things cancel each other.
  for (mwIndex i = 0; i < Dimension; ++i) {
    start[i] = nrrd.getMin()[i];
    size[i] = nrrd.getSize()[i];
    spacing[i] = nrrd.getSpacing()[i];
  }
  region.SetIndex(start);
  region.SetSize(size);
  image->SetRegions(region);
  image->SetSpacing(spacing);
  image->Allocate();
  image->Update();
  
  // loop through every voxel in the image and copy it to the ITK image
  InIteratorType iter(image, image->GetLargestPossibleRegion());
  mwIndex i = 0; // index for the input image
  for (iter.GoToBegin(), i=0; !iter.IsAtEnd(); ++iter, ++i) {
    iter.Set(im[i]);
    
    // Note: If you need to debug this loop, take into account that
    //   std::cout << im[i] << std::endl;
    // doesn't work as expected if im is e.g. a 
    // (uint8_T *) pointer. Instead, you need to do
    //   std::cout << (double)im[i] << std::endl;
    
  }
  
  // instantiate filter
  typename FilterType::Pointer filter = FilterType::New();
  
  // pass any parameters specific to this filter. We have to do it
  // with explicit specialization of class FilterParamFactory to
  // prevent the compiler from compiling e.g. filter->SetAlpha1(0.3)
  // for filters that don't declare that member function
  FilterParamFactory<InVoxelType, OutVoxelType, 
    FilterType> filterParam(filter);

  // run filter on input image
  filter->SetInput(image);
  filter->Update();
  
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
  
  std::cout << "nargout = " << nargout << std::endl;

  // create output matrix for Matlab's result
  argOut[0] = (mxArray *)mxCreateNumericArray(nrrd.getNdim(), nrrd.getDims(),
					  outputVoxelClassId,
					  mxREAL);
  if (argOut[0] == NULL) {
    mexErrMsgTxt("Cannot allocate memory for output matrix");
  }
  OutVoxelType *imOutp =  (OutVoxelType *)mxGetPr(argOut[0]);
  
  // populate output image
  OutConstIteratorType citer(filter->GetOutput(), 
			     filter->GetOutput()->GetLargestPossibleRegion());
  for (citer.GoToBegin(), i=0; i < nrrd.numEl(); ++citer, ++i) {
    imOutp[i] = (OutVoxelType)citer.Get();
  }
  
}

// some filters don't accept certain input or output types. File
// FilterExclusions.cpp uses explicit template specialization to tell
// the compiler to compile an empty constructor in those cases
#import "FilterExclusions.cpp"

#endif /* BASEFILTER_CPP */
