/* ItkImFilter.cpp
 *
 * ITK_IMFILTER: Run ITK filter on a 2D or 3D image
 *
 * This MEX function is a multiple-purpose wrapper to be able to run
 * all ITK filters that inherit from itk::ImageToImageFilter on a
 * Matlab 2D image or 3D image volume.
 *
 * B = ITK_IMFILTER(TYPE, A, [FILTER PARAMETERS])
 *
 *   TYPE is a string with the filter we want to run. See below for a whole
 *   list of options.
 *
 *   A is a 2D matrix or 3D volume with the image or
 *   segmentation. Currently, A can be of any of the following
 *   Matlab classes:
 *
 *     boolean
 *     double
 *     single
 *     int8
 *     uint8
 *     int16
 *     uint16
 *     int32
 *     int64
 *
 *   A can also be a SCI NRRD struct, A = nrrd, with the following fields:
 *
 *     nrrd.data: 2D or 3D array with the image or segmentation, as above
 *     nrrd.axis: 3x1 struct array with fields:
 *       nnrd.axis.size:    number of voxels in the image
 *       nnrd.axis.spacing: voxel size, image resolution
 *       nnrd.axis.min:     real world coordinates of "left" edge of first voxel
 *       nnrd.axis.max:     ignored
 *       nnrd.axis.center:  ignored
 *       nnrd.axis.label:   ignored
 *       nnrd.axis.unit:    ignored
 *
 *   (An SCI NRRD struct is the output of Matlab's function scinrrd_load(),
 *   also available from Gerardus.)
 *
 *   [FILTER PARAMETERS] is an optional list of parameters, specific for
 *   each filter. See below for details.
 *
 *   B has the same size as the image in A, and contains the filtered image
 *   or segmentation mask. It's type depends on the type of A and the filter
 *   used, and it's computed automatically.
 *
 *
 * Supported filters:
 * -------------------------------------------------------------------------
 *
 * B = ITK_IMFILTER('skel', A)
 *
 *   (itk::BinaryThinningImageFilter3D) Skeletonize a binary mask
 *
 *   B has the same size and class as A
 *
 * [B, NV] = ITK_IMFILTER('dandist', A)
 *
 *   (itk::DanielssonDistanceMapImageFilter) Compute unsigned distance map
 *   for a binary mask. Distance values are given in voxel coordinates
 *
 *   B has the same size as A. B has a type large enough to store the
 *   maximum distance in the image. The largest available type is double. If
 *   this is not enough, a warning message is displayed, and double is used
 *   as the output type.
 *
 *   NV has the same size as A. Each element has the index of the
 *   closest foreground voxel. For example, NV(4) = 7 means that voxel
 *   4 is the closest foreground voxel to voxel 7.
 *
 * B = ITK_IMFILTER('maudist', A)
 *
 *   (itk::SignedMaurerDistanceMapImageFilter) Compute signed distance map
 *   for a binary mask. Distance values are given in real world coordinates,
 *   if the input image is given as an NRRD struct, or in voxel units, if
 *   the input image is a normal array. 
 *
 *   The output type is always double.
 *
 * B = ITK_IMFILTER('bwdilate', A, RADIUS, FOREGROUND)
 * B = ITK_IMFILTER('bwerode', A, RADIUS, FOREGROUND)
 *
 *   (itk::BinaryDilateImageFilter). Binary dilation. The structuring
 *   element is a ball.
 *   (itk::BinaryErodeImageFilter). Binary erosion. The structuring
 *   element is a ball.
 *
 *   RADIUS is a scalar with the radius of the ball in voxel units. If a
 *   non-integer number is provided, then floor(RADIUS) is used. By default,
 *   RADIUS = 0 and no dilation is performed.
 *
 *   FOREGROUND is a scalar. Voxels with that value will be the only ones
 *   dilated. By default, FOREGROUND is the maximum value allowed for the
 *   type, e.g. FOREGROUND=255 if the image is uint8. This is the default in
 *   ITK, so we respect it even if we would rather have 1 as the default.
 *
 *
 *
 * itk_imfilter() must be compiled before it can be used from Matlab.
 * If Gerardus' root directory is e.g. ~/gerardus, type from a linux
 * shell
 *
 *    $ cd ~/gerardus/matlab
 *    $ mkdir bin
 *    $ cd bin
 *    $ cmake ..
 *    $ make install
 *
 */

 /*
  * Author: Ramon Casero <rcasero@gmail.com>
  * Copyright © 2011 University of Oxford
  * Version: 0.5.2
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

#ifndef ITKIMFILTER_CPP
#define ITKIMFILTER_CPP

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

/* mex headers */
#include <mex.h>

/* C++ headers */
#include <iostream>
#include <cmath>
#include <matrix.h>
#include <vector>

/* ITK headers */
#include "itkImage.h"

/* Gerardus headers */
#include "MexBaseFilter.hpp"
#include "MexBinaryErodeImageFilter.hpp"
#include "MexBinaryDilateImageFilter.hpp"
#include "MexBinaryThinningImageFilter3D.hpp"
#include "MexDanielssonDistanceMapImageFilter.hpp"
#include "MexSignedMaurerDistanceMapImageFilter.hpp"
#include "NrrdImage.hpp"
/* End Gerardus headers (DO NOT DELETE THIS COMMENT) */

/*
 * Argument Parsers
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
 * parseInputTypeToTemplate() reads the input data type and the filter
 * type, and instantiates parseOutputTypeToTemplate<InVoxelType>() for
 * each input data type.
 *
 * This way, we have effectively mapped the input type from a run-time
 * variable to a set of compilation time templates.
 *
 * Then parseOutputTypeToTemplate<InVoxelType>() decides on the output
 * data type depending on the filter and the input, and instantiates
 * one parseFilterTypeAndRun<InVoxelType, OutVoxelType>() for each
 * output data type.
 *
 * This way, we have instantiated all combinations of input/output
 * data types, without having to code them explicitly.
 *
 * Finally, parseFilterTypeAndRun<InVoxelType, OutVoxelType>() checks
 * that the requested filter is implemented, maps the filter variable,
 * and runs the actual filtering, using derived filter classes from
 * MexBaseFilter.
 */

// list of supported filters. It has to be an enum so that we can pass
// it as a template constant parameter
enum SupportedFilter {
  nMexBinaryThinningImageFilter3D,
  nMexDanielssonDistanceMapImageFilter,
  nMexSignedMaurerDistanceMapImageFilter,
  nMexBinaryDilateImageFilter,
  nMexBinaryErodeImageFilter
};

/* 
 * partial explicit specialization to choose which filter we want to
 * instantiate
 */

// general declaration
template <SupportedFilter filterEnum, class InVoxelType, class OutVoxelType>
class FilterSelector {
public:
  FilterSelector(const NrrdImage &, int, mxArray**, 
		 const int, const mxArray**,
		 MexBaseFilter<InVoxelType, OutVoxelType> *&) {
    // #error Assertion fail: non-supported filter has been instantiated
    mexErrMsgTxt("Assertion fail: Filter not supported");
  }
};

// select filter MexBinaryThinningImageFilter3D
template <class InVoxelType, class OutVoxelType>
class FilterSelector<nMexBinaryThinningImageFilter3D, InVoxelType, OutVoxelType> {
public:
  FilterSelector(const NrrdImage & nrrd, 
		 int nargout, mxArray** argOut,
		 const int nargin, const mxArray** argIn,
		 MexBaseFilter<InVoxelType, OutVoxelType> *&filter) {
    filter = new MexBinaryThinningImageFilter3D<InVoxelType, 
						OutVoxelType>(nrrd, nargout, argOut);
  }
};

// select filter MexDanielssonDistanceMapImageFilter
template <class InVoxelType, class OutVoxelType>
class FilterSelector<nMexDanielssonDistanceMapImageFilter, InVoxelType, OutVoxelType> {
public:
  FilterSelector(const NrrdImage & nrrd, 
		 int nargout, mxArray** argOut,
		 const int nargin, const mxArray** argIn,
		 MexBaseFilter<InVoxelType, OutVoxelType> *&filter) {
    filter = new MexDanielssonDistanceMapImageFilter<InVoxelType, 
						     OutVoxelType>(nrrd, nargout, argOut);
  }
};

// select filter MexSignedMaurerDistanceMapImageFilter
template <class InVoxelType, class OutVoxelType>
class FilterSelector<nMexSignedMaurerDistanceMapImageFilter, InVoxelType, OutVoxelType> {
public:
  FilterSelector(const NrrdImage & nrrd, 
		 int nargout, mxArray** argOut,
		 const int nargin, const mxArray** argIn,
		 MexBaseFilter<InVoxelType, OutVoxelType> *&filter) {
    filter = new MexSignedMaurerDistanceMapImageFilter<InVoxelType, 
						       OutVoxelType>(nrrd, nargout, argOut);
  }
};

// select filter MexBinaryDilateImageFilter
template <class InVoxelType, class OutVoxelType>
class FilterSelector<nMexBinaryDilateImageFilter, InVoxelType, OutVoxelType> {
public:
  FilterSelector(const NrrdImage & nrrd, 
		 int nargout, mxArray** argOut,
		 const int nargin, const mxArray** argIn,
		 MexBaseFilter<InVoxelType, OutVoxelType> *&filter) {
    filter = new MexBinaryDilateImageFilter<InVoxelType, 
					    OutVoxelType>(nrrd, nargout, argOut,
							  nargin, argIn);
  }
};

// select filter MexBinaryErodeImageFilter
template <class InVoxelType, class OutVoxelType>
class FilterSelector<nMexBinaryErodeImageFilter, InVoxelType, OutVoxelType> {
public:
  FilterSelector(const NrrdImage & nrrd, 
		 int nargout, mxArray** argOut,
		 const int nargin, const mxArray** argIn,
		 MexBaseFilter<InVoxelType, OutVoxelType> *&filter) {
    filter = new MexBinaryErodeImageFilter<InVoxelType, 
					   OutVoxelType>(nrrd, nargout, argOut,
							  nargin, argIn);
  }
};

/*
 * call the batch of methods that create the filter, set it up,
 * connect it to the Matlab inputs and outputs, read parameters, and
 * perform the actual filtering
 *
 */

// runFilter<SupportedFilter, InVoxelType, OutVoxelType>()
template <SupportedFilter filterEnum, class InVoxelType, class OutVoxelType>
void runFilter(const int nargin, const mxArray** argIn,
	       int nargout, mxArray** argOut,
	       const NrrdImage &nrrd) {

  // pointer to the filter object (we are using polymorphism)
  MexBaseFilter<InVoxelType, OutVoxelType> *filter = NULL;
  
  // select the appropriate filter
  FilterSelector<filterEnum, InVoxelType, OutVoxelType> 
    filterSelector(nrrd, nargout, argOut, nargin, argIn, filter);
  if (filter == NULL) {
    mexErrMsgTxt("Assertion fail: filter is NULL in runFilter()");
  }

  // check number of output arguments
  filter->CheckNumberOfOutputs();
  
  // set up and run filter
  filter->GraftMatlabInputBufferIntoItkImportFilter();
  filter->FilterBasicSetup();
  filter->FilterAdvancedSetup();
  filter->MummifyFilterOutput(0);
  filter->RunFilter();
  filter->ExportOtherFilterOutputsToMatlab();

  // call the destructor of the filter object, so that it can free up
  // the memory buffers that it doesn't pass to Matlab
  delete filter;

  // successful exit
  return;
  
}

/*
 * series of parser functions that convert input run-time variables to
 * compilation-time templates
 */

// parseOutputTypeToTemplate<InVoxelType>()
template <SupportedFilter filterEnum, class InVoxelType>
void parseOutputTypeToTemplate(const int nargin,
			       const mxArray** argIn,
			       int nargout,
			       mxArray** argOut,
			       const NrrdImage &nrrd) {

  // establish output voxel type according to the filter
  if (filterEnum == nMexBinaryThinningImageFilter3D) {

    runFilter<nMexBinaryThinningImageFilter3D,
	      InVoxelType, 
	      InVoxelType>(nargin, argIn, nargout, argOut, nrrd);

  } else if (filterEnum == nMexDanielssonDistanceMapImageFilter) {

    // find how many bits we need to represent the maximum distance
    // that two voxels can have between them (in voxel units)
    mwSize nbit = (mwSize)ceil(log(nrrd.maxVoxDistance()) / log(2.0));
    
    // select an output voxel size enough to save the maximum distance
    // value
    if (nbit <= 2) {
      runFilter<nMexDanielssonDistanceMapImageFilter,
		InVoxelType, 
		mxLogical>(nargin, argIn, nargout, argOut, nrrd);
    } else if (nbit <= 8) {
      runFilter<nMexDanielssonDistanceMapImageFilter,
		InVoxelType, 
		uint8_T>(nargin, argIn, nargout, argOut, nrrd);
    } else if (nbit <= 16) {
      runFilter<nMexDanielssonDistanceMapImageFilter,
		InVoxelType, 
		uint16_T>(nargin, argIn, nargout, argOut, nrrd);
    } else if (nbit <= 128) {
      runFilter<nMexDanielssonDistanceMapImageFilter,
		InVoxelType, 
		float>(nargin, argIn, nargout, argOut, nrrd);
    } else {
      runFilter<nMexDanielssonDistanceMapImageFilter,
		InVoxelType, 
		double>(nargin, argIn, nargout, argOut, nrrd);
    }
    
  } else if (filterEnum == nMexSignedMaurerDistanceMapImageFilter) {
    
    runFilter<nMexSignedMaurerDistanceMapImageFilter,
	      InVoxelType, 
	      double>(nargin, argIn, nargout, argOut, nrrd);

  } else if (filterEnum == nMexBinaryDilateImageFilter) {

    runFilter<nMexBinaryDilateImageFilter,
	      InVoxelType, 
	      InVoxelType>(nargin, argIn, nargout, argOut, nrrd);

  } else if (filterEnum == nMexBinaryErodeImageFilter) {

    runFilter<nMexBinaryErodeImageFilter,
	      InVoxelType, 
	      InVoxelType>(nargin, argIn, nargout, argOut, nrrd);

    /* Insertion point: parseOutputTypeToTemplate (DO NOT DELETE THIS COMMENT) */

  } else {
    mexErrMsgTxt("Filter type not implemented");
  }

}

// list of filters incompatible with certain input types
#define INVALIDINPUTTYPE(filterEnum, InVoxelType)			\
  template <>								\
  void parseOutputTypeToTemplate<filterEnum,				\
				 InVoxelType>(const int, const mxArray**, \
					      int, mxArray**,		\
					      const NrrdImage &) {	\
    mexErrMsgTxt("Input type incompatible with this filter");		\
}

INVALIDINPUTTYPE(nMexBinaryThinningImageFilter3D, mxLogical)
INVALIDINPUTTYPE(nMexSignedMaurerDistanceMapImageFilter, mxLogical)
/* Insertion point: INVALIDINPUTTYPE */

#undef INVALIDINPUTTYPE

// parseInputTypeToTemplate()
template <SupportedFilter filterEnum>
void parseInputTypeToTemplate(const int nargin,
			      const mxArray** argIn,
			      int nargout,
			      mxArray** argOut) {
  
  // read image and its parameters, whether it's in NRRD format, or
  // just a 2D or 3D array
  //
  // we need to do this here, and not later at the filter parsing
  // stage, because in some cases the output type will depend on the
  // size of the image volume
  NrrdImage nrrd(argIn[1]);

  // input image type
  mxClassID inputVoxelClassId = mxGetClassID(nrrd.getData());

  switch(inputVoxelClassId)  { // swith input image type
  case mxLOGICAL_CLASS:
    parseOutputTypeToTemplate<filterEnum, mxLogical>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxDOUBLE_CLASS:
    parseOutputTypeToTemplate<filterEnum, double>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxSINGLE_CLASS:
    parseOutputTypeToTemplate<filterEnum, float>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxINT8_CLASS:
    parseOutputTypeToTemplate<filterEnum, int8_T>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxUINT8_CLASS:
    parseOutputTypeToTemplate<filterEnum, uint8_T>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxINT16_CLASS:
    parseOutputTypeToTemplate<filterEnum, int16_T>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxUINT16_CLASS:
    parseOutputTypeToTemplate<filterEnum, uint16_T>(nargin, argIn, nargout, argOut, nrrd);
    break;
  case mxINT32_CLASS:
    parseOutputTypeToTemplate<filterEnum, int32_T>(nargin, argIn, nargout, argOut, nrrd);
    break;
  // case mxUINT32_CLASS:
  //   break;
  case mxINT64_CLASS:
    parseOutputTypeToTemplate<filterEnum, int64_T>(nargin, argIn, nargout, argOut, nrrd);
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

  // exit successfully
  return;

}

// parseFilterTypeToTemplate()
void parseFilterTypeToTemplate(const int nargin,
			       const mxArray** argIn,
			       int nargout,
			       mxArray** argOut) {

  // get type of filter
  char *filterName = mxArrayToString(argIn[0]);
  if (filterName == NULL) {
    mexErrMsgTxt("Invalid FILTER string");
  }

  // macro that returns true if the string in x is either the short or
  // long name of the filter type T
#define ISFILTER(x, T)							\
  !strcmp(x, T<std::string, std::string>::shortname.c_str())		\
    || !strcmp(x, T<std::string, std::string>::longname.c_str())

  // convert run-time filter string to template
  if (ISFILTER(filterName, MexBinaryThinningImageFilter3D)) {
    
    parseInputTypeToTemplate<nMexBinaryThinningImageFilter3D>(nargin, argIn,
							      nargout, argOut);

  }  else if (ISFILTER(filterName, MexDanielssonDistanceMapImageFilter)) {
    
    parseInputTypeToTemplate<nMexDanielssonDistanceMapImageFilter>(nargin, argIn,
								   nargout, argOut);

  }  else if (ISFILTER(filterName, MexSignedMaurerDistanceMapImageFilter)) {

    parseInputTypeToTemplate<nMexSignedMaurerDistanceMapImageFilter>(nargin, argIn,
								     nargout, argOut);

  } else if (ISFILTER(filterName, MexBinaryDilateImageFilter)) {

    parseInputTypeToTemplate<nMexBinaryDilateImageFilter>(nargin, argIn,
							  nargout, argOut);

  } else if (ISFILTER(filterName, MexBinaryErodeImageFilter)) {

    parseInputTypeToTemplate<nMexBinaryErodeImageFilter>(nargin, argIn,
							 nargout, argOut);

    /* Insertion point: parseFilterTypeToTemplate (DO NOT DELETE THIS COMMENT) */

  } else {
    mexErrMsgTxt("Filter type not implemented");
  }

#undef ISFILTER

  // exit successfully
  return;

}

/*
 * mexFunction(): entry point for the mex function
 */
void mexFunction(int nlhs, mxArray *plhs[], 
		 int nrhs, const mxArray *prhs[]) {
  // check number of input and output arguments
  if (nrhs < 2) {
    mexErrMsgTxt("Not enough input arguments");
  }

  // run filter (this function starts a cascade of functions designed
  // to translate the run-time type variables like inputVoxelClassId
  // to templates, so that we don't need to nest lots of "switch" or
  // "if" statements)
  parseFilterTypeToTemplate(nrhs,
			    prhs,
			    nlhs,
			    plhs);

  // exit successfully
  return;

}

#endif /* ITKIMFILTER_CPP */
