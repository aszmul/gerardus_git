/* CgalClosestTriFacet.cpp
 *
 * CGAL_CLOSEST_TRIFACET  Closest triangular facet of a mesh to a point in 3D
 *
 *  Note that this Matlab implementation is 54 times slower than the
 *  C++ MEX implementation cgal_closest_trifacet() in the Right Ventricle
 *  example of matlab/test/cgal_closest_trifacet.m.
 *
 * [IDX, D, P] = cgal_closest_trifacet(TRI, X, XI)
 *
 *   TRI is a 3-column matrix. Each row contains the 3 nodes that form one
 *   triangular facet in the mesh.
 *
 *   X is a 3-column matrix. X(i, :) contains the xyz-coordinates of the
 *   i-th node in the mesh.
 *
 *   XI is a 3-column matrix. XI(i, :) are the xyz-coordinates of a test
 *   point. This function finds the facet TRI(j,:) that is closest to
 *   XI(i,:).
 *
 *   IDX is a vector with one element per point in XI. IDX(i) is the index
 *   of the closest facet to XI(:,i). For instance, to obtain the nodes of
 *   the facet closest to XI(i,:), run TRI(IDX(i), :).
 *
 *   D is a vector with the same length as IDX. D(i) is the distance of
 *   point XI(i,:) to the mesh, or equivalently, to the closest facet.
 *
 *   P is a 3-column matrix. P(i,:) are the xyz-coordinates of the closest
 *   point in the mesh to XI(i,:).
 *
 * See also: closest_trifacet (an inefficient Matlab implementation that
 * mirrors this function)
 *
 */

 /*
  * Author: Ramon Casero <rcasero@gmail.com>
  * Copyright © 2013 University of Oxford
  * Version: 0.3.1
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

#ifndef CGALCLOSESTFACETTOPOINT
#define CGALCLOSESTFACETTOPOINT

/* mex headers */
#include <mex.h>

/* C++ headers */
#include <iostream>

/* Gerardus headers */
#include "MatlabImportFilter.h"
#include "MatlabExportFilter.h"

/* CGAL headers */
#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>

typedef CGAL::Simple_cartesian<double>            K;
typedef K::FT                                     FT;
typedef CGAL::Point_3<K>                          Point;
typedef K::Segment_3                              Segment;
typedef CGAL::Triangle_3<K>                       Triangle; // size 72 byte
typedef std::vector<Triangle>::iterator           Iterator; // size  8 byte
typedef CGAL::AABB_triangle_primitive<K,Iterator> Primitive;
typedef CGAL::AABB_traits<K, Primitive>           AABB_triangle_traits;
typedef CGAL::AABB_tree<AABB_triangle_traits>     Tree;
typedef Tree::Object_and_primitive_id             Object_and_primitive_id;
typedef Tree::Point_and_primitive_id              Point_and_primitive_id;

/*
 * mexFunction(): entry point for the mex function
 */
void mexFunction(int nlhs, mxArray *plhs[], 
		 int nrhs, const mxArray *prhs[]) {

  // interface to deal with input arguments from Matlab
  enum InputIndexType {IN_TRI, IN_X, IN_XI, InputIndexType_MAX};
  MatlabImportFilter::Pointer matlabImport = MatlabImportFilter::New();
  matlabImport->ConnectToMatlabFunctionInput(nrhs, prhs);

  // check that we have at least a filter name and input image
  matlabImport->CheckNumberOfArguments(3, InputIndexType_MAX);

  // register the inputs for this function at the import filter
  typedef MatlabImportFilter::MatlabInputPointer MatlabInputPointer;
  MatlabInputPointer inTRI = matlabImport->RegisterInput(IN_TRI, "TRI");
  MatlabInputPointer inX   = matlabImport->RegisterInput(IN_X, "X");
  MatlabInputPointer inXI  = matlabImport->RegisterInput(IN_XI, "XI");

  // interface to deal with outputs to Matlab
  enum OutputIndexType {OUT_IDX, OUT_D, OUT_P, OutputIndexType_MAX};
  MatlabExportFilter::Pointer matlabExport = MatlabExportFilter::New();
  matlabExport->ConnectToMatlabFunctionOutput(nlhs, plhs);

  // check number of outputs the user is asking for
  matlabExport->CheckNumberOfArguments(0, OutputIndexType_MAX);

  // register the outputs for this function at the export filter
  typedef MatlabExportFilter::MatlabOutputPointer MatlabOutputPointer;
  MatlabOutputPointer outIDX = matlabExport->RegisterOutput(OUT_IDX, "IDX");
  MatlabOutputPointer outD = matlabExport->RegisterOutput(OUT_D, "D");
  MatlabOutputPointer outP = matlabExport->RegisterOutput(OUT_P, "P");

  // if any of the inputs is empty, the output is empty too
  if (mxIsEmpty(prhs[IN_TRI]) || mxIsEmpty(prhs[IN_X]) || mxIsEmpty(prhs[IN_XI])) {
    matlabExport->CopyEmptyArrayToMatlab(outIDX);
    matlabExport->CopyEmptyArrayToMatlab(outD);
    matlabExport->CopyEmptyArrayToMatlab(outP);
    return;
  }

  // default coordinates are NaN values, so that the user can spot
  // whether there was any problem reading them
  Point def(mxGetNaN(), mxGetNaN(), mxGetNaN());

  // get size of input matrix
  mwSize nrowsTri = mxGetM(prhs[IN_TRI]);
  mwSize nrowsXi = mxGetM(prhs[IN_XI]);
  mwSize ncolsTri = mxGetN(prhs[IN_TRI]);
  mwSize ncolsX = mxGetN(prhs[IN_X]);
  mwSize ncolsXi = mxGetN(prhs[IN_XI]);
  if ((ncolsTri != 3) || (ncolsX != 3) || (ncolsXi != 3)) {
    mexErrMsgTxt("All input arguments must have 3 columns");
  }

  // read triangular mesh from function
  std::vector<Triangle> triangles(nrowsTri);
  mwIndex v0, v1, v2; // indices of the 3 vertices of each triangle
  Point x0, x1, x2; // coordinates of the 3 vertices of each triangle

  Iterator it = triangles.begin();
  for (mwIndex i = 0; i < nrowsTri; ++i, ++it) {

    // exit if user pressed Ctrl+C
    ctrlcCheckPoint(__FILE__, __LINE__);

    // get indices of the 3 vertices of each triangle. These indices
    // follow Matlab's convention v0 = 1, 2, ..., n
    v0 = matlabImport->ReadScalarFromMatlab<mwIndex>(inTRI, i, 0, mxGetNaN());
    v1 = matlabImport->ReadScalarFromMatlab<mwIndex>(inTRI, i, 1, mxGetNaN());
    v2 = matlabImport->ReadScalarFromMatlab<mwIndex>(inTRI, i, 2, mxGetNaN());
    if (mxIsNaN(v0) || mxIsNaN(v1) || mxIsNaN(v2)) {
      mexErrMsgTxt("Parameter TRI: Vertex index is NaN");
    }
    
    // get coordinates of the 3 vertices (substracting 1 so that
    // indices follow the C++ convention 0, 1, ..., n-1)
    x0 = matlabImport->ReadRowVectorFromMatlab<void, Point>(inX, v0 - 1, def);
    x1 = matlabImport->ReadRowVectorFromMatlab<void, Point>(inX, v1 - 1, def);
    x2 = matlabImport->ReadRowVectorFromMatlab<void, Point>(inX, v2 - 1, def);

    // add triangle to the vector of triangles in the surface
    triangles[i] = Triangle(x0, x1, x2);

    // // debug: show the memory address of each triangle in the std::vector
    // std::cout << "tri mem address: " << &(triangles[i]) << std::endl;

  }

  // // debug: show the memory address of each triangle in the std::vector
  // for (it = triangles.begin(); it != triangles.end(); ++it) {
  //   std::cout << "tri mem address: " << &(*(it)) << std::endl;
  // }

  // construct AABB tree
  Tree tree(triangles.begin(),triangles.end());

  // construct internal data structure to accelerate distance queries
  if (!tree.accelerate_distance_queries()) {
    mexErrMsgTxt("Not enough memory to accelerate distance queries");
  }

  // allocate memory for the outputs and get pointers to the corresponding buffers
  double *f = NULL;
  double *d = NULL;
  double *p = NULL;
  if (outIDX->isRequested) {
    f = matlabExport->AllocateColumnVectorInMatlab<double>(outIDX, nrowsXi);
  }
  if (outD->isRequested) {
    d = matlabExport->AllocateColumnVectorInMatlab<double>(outD, nrowsXi);
  }
  if (outP->isRequested) {
    p = matlabExport->AllocateMatrixInMatlab<double>(outP, nrowsXi, 3);
  }

  // loop every point to compute its distance to, intersection with
  // and closest facet of the surface
  Point xi; // test point coordinates
  for (mwIndex i = 0; i < nrowsXi; ++i) {
    
    // exit if user pressed Ctrl+C
    ctrlcCheckPoint(__FILE__, __LINE__);
    
    // get point coordinates to be tested
    xi = matlabImport->ReadRowVectorFromMatlab<void, Point>(inXI, i, def);

    // // debug: print coordinates of point being tested
    // std::cout << "point = " << xi << std::endl;
    
    // computes closest point and closest facet
    Point_and_primitive_id pp = tree.closest_point_and_primitive(xi);

    // closest facet
    if (outIDX->isRequested) {
      f[i] = &(*pp.second) - &(triangles[0]) + 1;
    }

    // // debug: show the memory address of the returned facet
    // std::cout << "facet mem address: " << &(*pp.second) << std::endl;

    // computes distance from query point to closest triangle
    if (outD->isRequested) {
      d[i] = 0;
      d[i] += (pp.first[0]-xi[0])*(pp.first[0]-xi[0]);
      d[i] += (pp.first[1]-xi[1])*(pp.first[1]-xi[1]);
      d[i] += (pp.first[2]-xi[2])*(pp.first[2]-xi[2]);
      d[i] = sqrt(d[i]);
    }

    // closest point on the surface to the testing point
    if (outP->isRequested) {
      p[i] = pp.first[0];
      p[i + nrowsXi] = pp.first[1];
      p[i + 2*nrowsXi] = pp.first[2];
    }

  }
  
}

#endif /* CGALCLOSESTFACETTOPOINT */
