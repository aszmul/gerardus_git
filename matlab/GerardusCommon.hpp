/*
 * GerardusCommon.hpp
 *
 * Miscellaneous functions of general use.
 */

 /*
  * Author: Ramon Casero <rcasero@gmail.com>
  * Copyright © 2011 University of Oxford
  * Version: 0.5.0
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

#ifndef GERARDUSCOMMON_HPP
#define GERARDUSCOMMON_HPP

/* mex headers */
#include <mex.h>

/* C++ headers */
//#import <vector>

/* ITK headers */
#include "itkOffset.h"

/* 
 * CAST2MWSIZE(): macro to cast to mwSize type. This definition is
 *                necessary for ITK v3.20.0 to avoid an error when
 *                trying to compile
 *                itk::FixedArray::operator[](unsigned __int64) for
 *                Windows 64 bit, but * maybe we can remove it when
 *                ITK v4.0.0 is released
 */
#ifdef _WIN64
#define CAST2MWSIZE(x) static_cast<unsigned long>(x)
#else
#define CAST2MWSIZE(x) static_cast<mwSize>(x)
#endif

/*
 * sub2ind(): function that converts r, c, s indices to linear indices
 *            in a 3D array (same as Matlab's function sub2ind(),
 *            although in Matlab indices start at 1, and in C++, they
 *            start at 0)
 *
 * R, C, S: size of the array in rows, columns and slices, respectively
 * rcs: subindices to be converted
 * r, c, s: subindices to be converted
 *
 */
mwIndex sub2ind(mwSize R, mwSize C, mwSize S, itk::Offset<3> rcs);
mwIndex sub2ind(mwSize R, mwSize C, mwSize S, mwIndex r, mwIndex c, mwIndex s);

/*
 * ind2sub(): function that converts linear indices in a 3D array to
 *            r, c, s indices (same as Matlab's function ind2sub(),
 *            although in Matlab indices start at 1, and in C++, they
 *            start at 0)
 *
 * R, C, S: size of the array in rows, columns and slices, respectively
 * rcs: subindices to be converted
 *
 */
itk::Offset<3> ind2sub_itkOffset(mwSize R, mwSize C, mwSize S, mwIndex idx);

/*
 * Block of functions to allow testing of template types
 */
template< class T1, class T2 >
struct TypesAreEqual
{ static const bool value = false; };

template< class T >
struct TypesAreEqual< T, T >
{ static const bool value = true; };

template< class T >
struct TypeIsBool
{ static const bool value = false; };

template<>
struct TypeIsBool< mxLogical >
{ static const bool value = true; };

template< class T >
struct TypeIsUint8
{ static const bool value = false; };

template<>
struct TypeIsUint8< uint8_T >
{ static const bool value = true; };

template< class T >
struct TypeIsInt8
{ static const bool value = false; };

template<>
struct TypeIsInt8< int8_T >
{ static const bool value = true; };

template< class T >
struct TypeIsUint16
{ static const bool value = false; };

template<>
struct TypeIsUint16< uint16_T >
{ static const bool value = true; };

template< class T >
struct TypeIsInt16
{ static const bool value = false; };

template<>
struct TypeIsInt16< int16_T >
{ static const bool value = true; };

template< class T >
struct TypeIsInt32
{ static const bool value = false; };

template<>
struct TypeIsInt32< int32_T >
{ static const bool value = true; };

template< class T >
struct TypeIsInt64
{ static const bool value = false; };

template<>
struct TypeIsInt64< int64_T >
{ static const bool value = true; };

template< class T >
struct TypeIsSignedLong
{ static const bool value = false; };

template<>
struct TypeIsSignedLong< signed long >
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

/**
 * Debugging class to get a string with the type of a template, e.g.
 *   std::cout << print_T<OffsetType>() << std::endl;
 * returns
 *   OffsetType = std::string print_T() 
 *   [with T = itk::Offset<3u>, std::string = std::basic_string<char>]
 */
template<typename T>
std::string print_T() {
  return __PRETTY_FUNCTION__;
}

#endif /* GERARDUSCOMMON_HPP */
