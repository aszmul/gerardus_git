/*
 * SignedMaurerFilter.cpp
 *
 * Code that is specific to the SignedMaurerDistanceMapImageFilter
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

#ifndef SIGNEDMAURERFILTER_CPP
#define SIGNEDMAURERFILTER_CPP

#import "SignedMaurerFilter.hpp"

/* 
 * SignedMaurerFilter : BaseFilter
 */
template <class InVoxelType, class OutVoxelType>
void SignedMaurerFilter<InVoxelType, 
			OutVoxelType>::SetSpecificFilterParameters() {
  
  std::cout << "SignedMaurerFilter::SetSpecificFilterParameters" 
	    << std::endl;////////////////

  // compute distances using real world coordinates, instead of voxel
  // indices
  this->filter->SetUseImageSpacing(true);

  // give output as actual distances, instead of squared distances
  this->filter->SquaredDistanceOff();

}

/*
 * Filter exclusions: input/output data type combinations that are not
 * allowed for this filter
 */

#define EXCLUDEFILTER(T1, T2)						\
  SignedMaurerFilter<T1, T2>::SignedMaurerFilter			\
  (char *, NrrdImage, int, mxArray**) {					\
    mexErrMsgTxt("Invalid input or output image type for itk::SignedMaurerDistanceMapFilter");}

EXCLUDEFILTER(bool, bool);
EXCLUDEFILTER(bool, uint8_T)
EXCLUDEFILTER(bool, int8_T)
EXCLUDEFILTER(bool, uint16_T)
EXCLUDEFILTER(bool, int16_T)
EXCLUDEFILTER(bool, int32_T)
EXCLUDEFILTER(bool, int64_T)
EXCLUDEFILTER(bool, float)

EXCLUDEFILTER(uint8_T, bool);
EXCLUDEFILTER(uint8_T, uint8_T)
EXCLUDEFILTER(uint8_T, int8_T)
EXCLUDEFILTER(uint8_T, uint16_T)
EXCLUDEFILTER(uint8_T, int16_T)
EXCLUDEFILTER(uint8_T, int32_T)
EXCLUDEFILTER(uint8_T, int64_T)
EXCLUDEFILTER(uint8_T, float)

EXCLUDEFILTER(int8_T, bool);
EXCLUDEFILTER(int8_T, uint8_T)
EXCLUDEFILTER(int8_T, int8_T)
EXCLUDEFILTER(int8_T, uint16_T)
EXCLUDEFILTER(int8_T, int16_T)
EXCLUDEFILTER(int8_T, int32_T)
EXCLUDEFILTER(int8_T, int64_T)
EXCLUDEFILTER(int8_T, float)

EXCLUDEFILTER(uint16_T, bool);
EXCLUDEFILTER(uint16_T, uint8_T)
EXCLUDEFILTER(uint16_T, int8_T)
EXCLUDEFILTER(uint16_T, uint16_T)
EXCLUDEFILTER(uint16_T, int16_T)
EXCLUDEFILTER(uint16_T, int32_T)
EXCLUDEFILTER(uint16_T, int64_T)
EXCLUDEFILTER(uint16_T, float)

EXCLUDEFILTER(int16_T, bool);
EXCLUDEFILTER(int16_T, uint8_T)
EXCLUDEFILTER(int16_T, int8_T)
EXCLUDEFILTER(int16_T, uint16_T)
EXCLUDEFILTER(int16_T, int16_T)
EXCLUDEFILTER(int16_T, int32_T)
EXCLUDEFILTER(int16_T, int64_T)
EXCLUDEFILTER(int16_T, float)

EXCLUDEFILTER(int32_T, bool);
EXCLUDEFILTER(int32_T, uint8_T)
EXCLUDEFILTER(int32_T, int8_T)
EXCLUDEFILTER(int32_T, uint16_T)
EXCLUDEFILTER(int32_T, int16_T)
EXCLUDEFILTER(int32_T, int32_T)
EXCLUDEFILTER(int32_T, int64_T)
EXCLUDEFILTER(int32_T, float)

EXCLUDEFILTER(int64_T, bool);
EXCLUDEFILTER(int64_T, uint8_T)
EXCLUDEFILTER(int64_T, int8_T)
EXCLUDEFILTER(int64_T, uint16_T)
EXCLUDEFILTER(int64_T, int16_T)
EXCLUDEFILTER(int64_T, int32_T)
EXCLUDEFILTER(int64_T, int64_T)
EXCLUDEFILTER(int64_T, float)

EXCLUDEFILTER(float, bool);
EXCLUDEFILTER(float, uint8_T)
EXCLUDEFILTER(float, int8_T)
EXCLUDEFILTER(float, uint16_T)
EXCLUDEFILTER(float, int16_T)
EXCLUDEFILTER(float, int32_T)
EXCLUDEFILTER(float, int64_T)
EXCLUDEFILTER(float, float)

EXCLUDEFILTER(double, bool);
EXCLUDEFILTER(double, uint8_T)
EXCLUDEFILTER(double, int8_T)
EXCLUDEFILTER(double, uint16_T)
EXCLUDEFILTER(double, int16_T)
EXCLUDEFILTER(double, int32_T)
EXCLUDEFILTER(double, int64_T)
EXCLUDEFILTER(double, float)

#undef EXCLUDEFILTER

/*
 * Instantiate filter with all the input/output combinations that it
 * accepts. This is necessary for the linker. The alternative is to
 * have all the code in the header files, but this makes compilation
 * slower and maybe the executable larger
 */

#define FILTERINST(T1, T2)						\
  template class SignedMaurerFilter<T1, T2>;				\

FILTERINST(bool, double)
FILTERINST(uint8_T, double)
FILTERINST(int8_T, double)
FILTERINST(uint16_T, double)
FILTERINST(int16_T, double)
FILTERINST(int32_T, double)
FILTERINST(int64_T, double)
FILTERINST(float, double)
FILTERINST(double, double)

#undef FILTERINST

#endif /* SIGNEDMAURERFILTER_CPP */
