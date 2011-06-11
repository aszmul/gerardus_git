/*
 * im2imat.cpp
 *
 * IM2IMAT  Local neighbourhood mean intensity matrix between segmentation
 * voxels
 *
 * A = IM2IMAT(IM)
 *
 *   IM is an image volume with dimensions (R, C, S).
 *
 *   A is a sparse matrix with dimensions (R*C*S, R*C*S), where element
 *   (i,j) is the mean intensity between voxels with linear indices i and j.
 *
 *   Voxels with an Inf intensity are skipped.
 *
 *   This function has a slow Matlab implementation (using loops), but a
 *   fast MEX version is provided too. To compile it in a 64 bit
 *   architecture, run
 *
 *   >> mex -largeArrayDims im2imat.cpp
 *
 *   To compile in a 32 bit architecture, use
 *
 *   >> mex im2imat.cpp
 *
 * See also: seg2dmat.
 */
/*
 * Author: Ramon Casero <rcasero@gmail.com>
 * Copyright © 2010-2011 University of Oxford
 * Version: 0.1.1.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mex.h"
#include "matrix.h"

#include <iostream>

// entry point for the MEX file
void mexFunction(int nlhs, // number of expected outputs
		 mxArray *plhs[], // array of pointers to outputs
		 int nrhs, // number of inputs
		 const mxArray *prhs[] // array of pointers to inputs
		 ) {
  // Syntax:
  //
  // A = IMG_ADJACENCY_DISTANCE(IM)

  // variables
  mwSize R, C, S; // number of rows, cols and slices of input image
  double *im; // pointer to the image volume
  double *out; // pointer to the output adjacency-distance sparse matrix

  // check arguments
  if (nrhs != 1) {
    mexErrMsgTxt("1 input argument required.");
  }
  if (nlhs > 1) {
    mexErrMsgTxt("Maximum of 1 output argument allowed.");
  }

  // get image size
  const mwSize *sz = mxGetDimensions(prhs[0]);
  mwSize ndim = mxGetNumberOfDimensions(prhs[0]);
  if (ndim == 2) { // 2D image
    R = sz[0];
    C = sz[1];
    S = 1;
  } else if (ndim == 3) { // 3D image volume
    R = sz[0];
    C = sz[1];
    S = sz[2];
  } else {
    mexErrMsgTxt("Input argument has to be a 2D image or 3D image volume");
  }

  if (R < 3 | C < 3 | S < 3) {
    mexErrMsgTxt("Image volume size must be at least (3, 3, 3)");
  }

  // pointer to input image, for convenience
  if (!mxIsDouble(prhs[0])) {
    mexErrMsgTxt("Input image array must be of type double");
  }
  im = mxGetPr(prhs[0]);

  // count number of non infinite voxels in the image
  mwSize nvox = 0;
  for (mwSize idx = 0; idx < R*C*S; ++idx) {
    if (!mxIsInf(im[idx])) {nvox++;}
  }

  // create sparse matrix for the output
  // each voxel can be connected to up to 26 voxels (imagine a
  // (3,3,3)-cube of voxels, with our voxel in the middle). But a 
  // voxel will be seldom connected to so many voxels, so we are going
  // to allocate memory only for half the space
  plhs[0] = (mxArray *)mxCreateSparse(R*C*S, R*C*S, R*C*S*26, mxREAL);
  if (!plhs[0]) {mexErrMsgTxt("Not enough memory for output");}

  // pointer to the output matrix
  out = (double *)mxGetPr(plhs[0]);

  // pointer to the values in the sparse matrix (mean voxel intensity)
  double *pr = mxGetPr(plhs[0]);
  if (!pr) {
    mexErrMsgTxt("Error loading vector pr from sparse matrix");
  }

  // pointer to vector jc in the sparse matrix (cumsum of number of
  // voxels connected to current voxel). If the sparse matrix has n
  // columns, then jc has n+1 elements. Each one tells how many matrix
  // entries are nonzero so far. For example, if column 0 has 3
  // nonzero entries and column 1 has 2, then jc=[0 3 5]
  mwIndex *jc = mxGetJc(plhs[0]);
  if (!jc) {
    mexErrMsgTxt("Error loading vector jc from sparse matrix");
  }
  
  // pointer to the connected voxel indices
  //
  // in this case, indices follow the C++ convention
  mwIndex *ir = mxGetIr(plhs[0]);
  if (!ir) {
    mexErrMsgTxt("Error loading vector ir from sparse matrix");
  }

  // init jc so that we can add 1 every time we find a node connection
  for (mwSize idx = 0; idx <= R*C*S; ++idx) {
    jc[idx] = 0;
  }
  
  // loop voxels searching for voxels connected to them
  //
  // here the indices follow the C convention, i.e. first index is 0,
  // and will be converted to Matlab's convention (first index is 1)
  // when necessary
  //
  // because of the way Jc is defined, we are going to store the
  // connectivity value for the first node in the second element of
  // jc[], for the 2nd node in the 3rd element, and so on
  mwSize outidx = 0; // index for ir, pr
  mwSize nedg = 0; // number of edges or connections between voxels
  mwSize idx = 0; // linear index for voxels
  mwSize nnidx = 0; // linear index for adjacent voxels
  mwSize RC = R*C; // aux variable
  for (mwSize s = 0; s < S; ++s) {
    for (mwSize c = 0; c < C; ++c) {
      for (mwSize r = 0; r < R; ++r) {
	
	// get linear index of voxel from multiple indices
	idx = RC*s + R*c + r;

	// if current voxels is Inf, we don't include it in the
	// graph, so just skip to next iteration
	if (mxIsInf(im[idx])) {continue;}

	// examine 26 voxels surrounding the current voxel; if a
	// neighbour voxel is nonzero, we say that it is connected to
	// the current voxel, and has to be added to the sparse matrix
	for (mwSize nns = std::max((long int)0, (long int)s-1);
	     nns <= std::min(S-1, s+1); ++nns) {
	  for (mwSize nnc = std::max((long int)0, (long int)c-1);
	       nnc <= std::min(C-1, c+1); ++nnc) {
	    for (mwSize nnr = std::max((long int)0, (long int)r-1);
		 nnr <= std::min(R-1, r+1); ++nnr) {
	      
	      // don't connect current voxel to itself
	      if (nns == s && nnc == c && nnr == r) {continue;}

	      // get linear index of voxel from multiple indices
	      nnidx = RC*nns + R*nnc + nnr;

	      // skip connected voxels that are Inf
	      if (mxIsInf(im[nnidx])) {continue;}

	      // the edge weight between the current voxel and the
	      // connected voxel is the mean between their node values
	      pr[outidx] = (im[nnidx] + im[idx]) * 0.5;

	      // instead of computing jc directly, first we are going
	      // to see just how many voxels are connected to the
	      // current voxel
	      if (im[nnidx]) {
		jc[idx+1] += 1; //idx+1 to correct the node id for Matlab
	      }

	      // record the voxel connection (C++ convention, not Matlab's)
	      ir[outidx] = nnidx;

	      // advance one position in ir, pr
	      outidx++;

	    }
	  }
	}
	
	// save value of the number of connections we have
	nedg = outidx;

      }
    }
  }
				
  // now vector jc contains the number of voxels connected to each
  // voxel (e.g. [0 4 1 0 2]). However, we need instead the
  // accumulated vector (e.g. [0 4 5 0 7])
  for (mwSize idx = 1; idx <= R*C*S; ++idx) {
    jc[idx] += jc[idx-1];
  }
  


  // correct the number of non-zero entries in the sparse matrix
  mxSetNzmax(plhs[0], nedg);
  
};
