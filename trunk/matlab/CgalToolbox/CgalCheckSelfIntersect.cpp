/* CgalCheckSelfIntersect.cpp
 *
 * CGAL_CHECK_SELF_INTERSECT Check for self-intersections in a
 * triangular mesh
 *
 * This function checks whether each triangle in a mesh intersects any
 * other triangle. Finding self-intersections is useful to detect
 * topological problems.
 *
 * C = cgal_check_self_intersect(TRI, X)
 *
 *   TRI is a 3-column matrix. Each row contains the 3 nodes that form one
 *   triangular facet in the mesh.
 *
 *   X is a 3-column matrix. X(i, :) contains the xyz-coordinates of the
 *   i-th node in the mesh.
 *
 *   C is a vector with one element per triangle in TRI. It gives a
 *   count of the number of times TRI(I,:) causes a self-intersection
 *   in the mesh.
 *
 * This function uses an AABB tree component [1] to efficiently
 * perform the intersection queries. However, as the CGAL
 * documentation notes, "this component is not suited to the problem
 * of finding all intersecting pairs of objects", so there's probably
 * room for improvement.
 *
 * [1] http://www.cgal.org/Manual/latest/doc_html/cgal_manual/AABB_tree/Chapter_main.html
 */

 /*
  * Author: Ramon Casero <rcasero@gmail.com>
  * Copyright © 2013 University of Oxford
  * Version: 0.4.0
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

#ifndef CGALCHECKSELFINTERSECT
#define CGALCHECKSELFINTERSECT

/* mex headers */
#include <mex.h>

/* C++ headers */
#include <iostream>

/* Gerardus headers */
#include "MatlabImportFilter.h"
#include "MatlabExportFilter.h"

/* CGAL headers */
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>

// if we use a non-exact kernel (e.g. CGAL::Simple_cartesian<double>),
// then the intersections detected by CGAL will suffer from
// imprescissions and spurious intersections will show up, e.g. a
// point intersection will be given as a segment intersection where
// the segment has a tiny length. Thus, it is important to use
// CGAL::Exact_predicates_exact_constructions_kernel, which requires
// the GMP and MPFR libraries
typedef CGAL::Exact_predicates_exact_constructions_kernel    K;
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
  enum InputIndexType {IN_TRI, IN_X, IN_ITRI, InputIndexType_MAX};
  MatlabImportFilter::Pointer matlabImport = MatlabImportFilter::New();
  matlabImport->ConnectToMatlabFunctionInput(nrhs, prhs);

  // check the number of input arguments
  matlabImport->CheckNumberOfArguments(2, InputIndexType_MAX);

  // register the inputs for this function at the import filter
  typedef MatlabImportFilter::MatlabInputPointer MatlabInputPointer;
  MatlabInputPointer inTRI = matlabImport->RegisterInput(IN_TRI, "TRI");
  MatlabInputPointer inX = matlabImport->RegisterInput(IN_X, "X");
  MatlabInputPointer inITRI = matlabImport->RegisterInput(IN_ITRI, "ITRI");

  // interface to deal with outputs to Matlab
  enum OutputIndexType {OUT_C, OutputIndexType_MAX};
  MatlabExportFilter::Pointer matlabExport = MatlabExportFilter::New();
  matlabExport->ConnectToMatlabFunctionOutput(nlhs, plhs);

  // check number of outputs the user is asking for
  matlabExport->CheckNumberOfArguments(0, OutputIndexType_MAX);

  // register the outputs for this function at the export filter
  typedef MatlabExportFilter::MatlabOutputPointer MatlabOutputPointer;
  MatlabOutputPointer outC = matlabExport->RegisterOutput(OUT_C, "C");

  // if any of the inputs is empty, the output is empty too
  if (mxIsEmpty(prhs[IN_TRI]) || mxIsEmpty(prhs[IN_X])) {
    matlabExport->CopyEmptyArrayToMatlab(outC);
    return;
  }

  // default coordinates are NaN values, so that the user can spot
  // whether there was any problem reading them
  Point def(mxGetNaN(), mxGetNaN(), mxGetNaN());

  // get size of input matrix
  mwSize nrowsTri = mxGetM(prhs[IN_TRI]);
  mwSize ncolsTri = mxGetN(prhs[IN_TRI]);
  mwSize ncolsX = mxGetN(prhs[IN_X]);
  if ((ncolsTri != 3) || (ncolsX != 3)) {
    mexErrMsgTxt("All input arguments must have 3 columns");
  }

  // get list of triangle indices the user wants to check
  // intersections for. By default, we check all triangles
  std::vector<mwIndex> itriDef(nrowsTri);
  for (mwSize i = 0; i < nrowsTri; ++i) {
    itriDef[i] = i + 1;
  }
  std::vector<mwIndex> itri = matlabImport->
    ReadRowVectorFromMatlab<mwIndex, std::vector<mwIndex> >(inITRI, itriDef);

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

  // initialise outputs
  double *n = matlabExport->AllocateColumnVectorInMatlab<double>(outC, nrowsTri);

  // // DEBUG:
  // mwSize triNum = 0;

  // initialize variable to keep a list of intersections for the
  // current triangle
  std::list<Object_and_primitive_id> intersections;

  // loop every facet to see whether it intersects the mesh
  for (std::vector<mwIndex>::iterator itri_it = itri.begin(); itri_it != itri.end(); ++itri_it) {

    // triangle index (converting Matlab index convention 1, 2, 3,... to C++ 0, 1, 2,...
    mwIndex idx = *itri_it - 1;

    // iterator pointer to the triangle
    Iterator it = triangles.begin();
    it += idx;

    // // DEBUG:
    // std::cout << ++triNum << ": Triangle = " << *it << std::endl;

    // exit if user pressed Ctrl+C
    ctrlcCheckPoint(__FILE__, __LINE__);

    // if the triangle is degenerated, trying to find intersections
    // will produce a segfault. To avoid it, we just count one
    // intersection for this triangle and skip to the next one
    if (
	(it->vertex(0) == it->vertex(1))
	|| (it->vertex(0) == it->vertex(2))
	|| (it->vertex(1) == it->vertex(2))
	) {
      // // DEBUG:
      // mexWarnMsgTxt("Degenerate triangle found, skipping:");
      // std::cerr << it->vertex(0) << ", " << it->vertex(1) << ", " << it->vertex(2) << std::endl;

      // add one to the count of self intersections
      n[idx] += 1;

      continue;
    }

    // computes all intersections with segment query (as pairs object - primitive_id)
    intersections.clear();
    tree.all_intersections(*it, std::back_inserter(intersections));

    // // DEBUG:
    // std::cout << "\tTotal number of intersections found for current triangle = " 
    // 	      << intersections.size() << std::endl;

    // loop all intersections
    for (std::list<Object_and_primitive_id>::iterator itx = intersections.begin();
	 itx != intersections.end(); ++itx) {

      // two triangles sharing a vertex or edge are detected by CGAL
      // as intersecting. Of course, in those case we cannot talk
      // about triangles overlapping. In this block of code, be
      // identify what kind of intersection we have. The
      // self-intersections that will be considered as actual
      // self-intersections that make the mesh non-topological are:
      //
      // 1) all triangle-type intersections: This is the current
      //    triangle being parallel to another triangle, bigger or
      //    smaller. Note that an intersection will always be detected
      //    between the current triangle and itself. But it's easier
      //    to simply count and then discount that intersection than
      //    having to check the validity of each triangle-intersection
      //
      // 2) segment-type intersections where a triangle intersects
      //    another triangle. This excludes the intersections that are
      //    simply the triangle sharing an edge with its neighbours
      //
      // 3) point-type intersections where the triangle just touches
      //    another triangle. This excludes the intersections that are
      //    simply the triangle sharing a vertex with its neighbours
      Object_and_primitive_id op = *itx;
      CGAL::Object object = op.first;
      Triangle triangle;
      Segment segment;
      Point point;

      // 1) triangle intersection
      if(CGAL::assign(triangle, object)) {

	// add one to the count of self intersections
	n[idx] += 1;

	// // DEBUG:
	// std::cout << "\tTriangle: " << triangle << std::endl;
	// std::cout << "\t\tMesh self-intersection detected" << std::endl;

      } // end: triangle intersection

      // 2) segment intersection
      if(CGAL::assign(segment, object)) {

	// // DEBUG:
	// std::cout << "\tSegment: " << segment << std::endl;

	// for convenience, end points of the segment
	Point pa = segment[0];
	Point pb = segment[1];

	// for convenience, the three vertices of the current triangle
	Point vA = it->vertex(0);
	Point vB = it->vertex(1);
	Point vC = it->vertex(2);

	// for convenience, the three vertices of the intersected triangle
	Triangle intersectedTrianglePointer = *(op.second);
	Point vX = intersectedTrianglePointer.vertex(0);
	Point vY = intersectedTrianglePointer.vertex(1);
	Point vZ = intersectedTrianglePointer.vertex(2);

	// CGAL will detect as intersections the edges between the
	// current triangle and each neighbour. We need to detect and
	// disregard these cases
	if (
	    ((pa == vA) || (pa == vB) || (pa == vC))
	    && ((pb == vA) || (pb == vB) || (pb == vC))
	    && ((pa == vX) || (pa == vY) || (pa == vZ))
	    && ((pb == vX) || (pb == vY) || (pb == vZ))
	    ) {

	//   // DEBUG:
	//   std::cout << "\t\tDisregarding intersection: it's a valid edge between current triangle and neighbour" << std::endl;

	} else {

	//   // DEBUG:
	//   std::cout << "\t\tMesh self-intersection detected" << std::endl;

	  // add one to the count of self intersections
	  n[idx] += 1;
	}
	
      } // end: segment intersection

      // 3) point intersection
      if(CGAL::assign(point, object)) {

	// // DEBUG:
	// std::cout << "\tPoint: " << point << std::endl;

	// for convenience, the three vertices of the triangle
	Point vA = it->vertex(0);
	Point vB = it->vertex(1);
	Point vC = it->vertex(2);

	// for convenience, the three vertices of the intersected triangle
	Triangle intersectedTrianglePointer = *(op.second);
	Point vX = intersectedTrianglePointer.vertex(0);
	Point vY = intersectedTrianglePointer.vertex(1);
	Point vZ = intersectedTrianglePointer.vertex(2);

	// CGAL will detect as intersections the vertices shared by
	// the current triangle and each neighbour. We need to detect
	// and disregard those cases
	if (
	    ((point == vA) || (point == vB) || (point == vC))
	    && ((point == vX) || (point == vY) || (point == vZ))
	    ){

	//   // DEBUG:
	//   std::cout << "\t\tDisregarding intersection: it's a valid vertex shared by current triangle and neighbour" << std::endl;

	} else {

	//   // DEBUG:
	//   std::cout << "\t\tMesh self-intersection detected" << std::endl;

	  // add one to the count of self intersections
	  n[idx] += 1;
	}

      } // end: point intersection
      
    } // end: loop all intersections
    
    // substract one intersection, because each triangle will intersect itself
    n[idx] -= 1;

  }
  
}

#endif /* CGALCHECKSELFINTERSECT */
