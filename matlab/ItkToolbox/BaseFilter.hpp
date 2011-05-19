/* 
 * BaseFilter.hpp
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
  * Version: 0.1.2
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

#ifndef BASEFILTER_HPP
#define BASEFILTER_HPP

/* Gerardus headers */
#import "NrrdImage.hpp"

/* Global variables */
static const unsigned int Dimension = 3; // volume data dimension
                                         // (3D volume)

/*
 * Block of functions to allow testing of template types
 */
template< class T >
struct TypeIsBool
{ static const bool value = false; };

template<>
struct TypeIsBool< bool >
{ static const bool value = true; };

template< class T >
struct TypeIsUint8
{ static const bool value = false; };

template<>
struct TypeIsUint8< uint8_T >
{ static const bool value = true; };

template< class T >
struct TypeIsUint16
{ static const bool value = false; };

template<>
struct TypeIsUint16< uint16_T >
{ static const bool value = true; };

template< class T >
struct TypeIsFloat
{ static const bool value = false; };

template<>
struct TypeIsFloat< float >
{ static const bool value = true; };

template< class T >
struct TypeIsDouble
{ static const bool value = false; };

template<>
struct TypeIsDouble< double >
{ static const bool value = true; };

/* 
 * BaseFilter
 */
template <class InVoxelType, class OutVoxelType, class FilterType>
class BaseFilter {
private:
  typedef double TScalarType; // data type for scalars
  typedef itk::Image< InVoxelType, Dimension > InImageType;
  typedef itk::Image< OutVoxelType, Dimension > OutImageType;
  
protected:
  typename InImageType::Pointer image;
  typename FilterType::Pointer filter;
  NrrdImage nrrd;
  int nargout;
  mxArray** argOut;

public:
  BaseFilter(char *filterType, NrrdImage &_nrrd, 
		int _nargout, mxArray** &_argOut);
  virtual void CopyMatlabInputsToFilter();
  virtual void FilterSetup();
  virtual void RunFilter();
  virtual void CopyFilterOutputsToMatlab();
};

#endif /* BASEFILTER_HPP */
