/*
 * NrrdImage.hpp
 *
 * NrrdImage: class to parse an image that follows the SCI NRRD format
 * obtained from saving an image in Seg3D to a Matlab file
 */

 /*
  * Author: Ramon Casero <rcasero@gmail.com>
  * Copyright © 2011 University of Oxford
  * Version: 0.2.1
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

#ifndef NRRDIMAGE_HPP
#define NRRDIMAGE_HPP

/* C++ headers */
#include <cmath>

/*
 * Global variables and declarations
 */
class NrrdImage {
private:
  
  static const unsigned int Dimension = 3; // image dimension (we assume
                                           // a 3D volume also for 2D images)

  mxArray *data; // pointer to the image data in Matlab format
  mwSize ndim; // number of elements in the dimensions array dims
  mwSize *dims; // dimensions array
  std::vector<mwSize> size; // number of voxels in each dimension
  std::vector<double> spacing; // voxel size in each dimension
  std::vector<double> min; // real world coordinates of the image origin

public:
  NrrdImage(const mxArray * nrrd);
  NrrdImage() {;}
  mxArray * getData() {return data;}
  std::vector<mwSize> getSize() {return size;}
  std::vector<double> getSpacing() {return spacing;}
  std::vector<double> getMin() {return min;}
  mwSize getR() {return size[0];}
  mwSize getC() {return size[1];}
  mwSize getS() {return size[2];}
  double getDr() {return spacing[0];}
  double getDc() {return spacing[1];}
  double getDs() {return spacing[2];}
  double getDx() {return spacing[1];}
  double getDy() {return spacing[0];}
  double getDz() {return spacing[2];}
  double getMinR() {return min[0];}
  double getMinC() {return min[1];}
  double getMinS() {return min[2];}
  double getMinX() {return min[1];}
  double getMinY() {return min[0];}
  double getMinZ() {return min[2];}
  mwSize getNdim() {return ndim;}
  mwSize *getDims() {return dims;}
  double maxVoxDistance();
  mwSize numEl();
};

#endif /* NRRDIMAGE_HPP */
