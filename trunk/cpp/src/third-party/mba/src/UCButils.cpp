//===========================================================================
// File modified by Ramón Casero <rcasero@gmail.com> for project Gerardus
//===========================================================================
// SINTEF Multilevel B-spline Approximation library - version 1.1
//
// Copyright (C) 2000-2005 SINTEF ICT, Applied Mathematics, Norway.
//
// This program is free software; you can redistribute it and/or          
// modify it under the terms of the GNU General Public License            
// as published by the Free Software Foundation version 2 of the License. 
//
// This program is distributed in the hope that it will be useful,        
// but WITHOUT ANY WARRANTY; without even the implied warranty of         
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
// GNU General Public License for more details.                           
//
// You should have received a copy of the GNU General Public License      
// along with this program; if not, write to the Free Software            
// Foundation, Inc.,                                                      
// 59 Temple Place - Suite 330,                                           
// Boston, MA  02111-1307, USA.                                           
//
// Contact information: e-mail: tor.dokken@sintef.no                      
// SINTEF ICT, Department of Applied Mathematics,                         
// P.O. Box 124 Blindern,                                                 
// 0314 Oslo, Norway.                                                     
//
// Other licenses are also available for this software, notably licenses
// for:
// - Building commercial software.                                        
// - Building software whose source code you wish to keep private.        
//===========================================================================
#include <UCButils.h>

#ifdef MBA_DEBUG
#include <MBAclock.h>
#endif

#ifdef WIN32
#include <minmax.h>
#endif

#include <iostream>
#include <fstream>
#include <math.h>
#include <algorithm>
using namespace std;

#include <stdio.h>
#include <string>



void UCBspl::printVRMLgrid(const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV, double scale) {

  
#ifdef MBA_DEBUG
  cout << "printSampleVRML to: " << filename << endl; 
#endif
  
  bool bscale = false;
  if (scale != 1.0)
    bscale = true;
  
  ofstream os(filename);
  
  os << "#VRML V1.0 ascii" << endl;
  os << endl;
  os << "Separator {" << endl; 
  os << "    ShapeHints {" << endl;
  os << "       vertexOrdering  COUNTERCLOCKWISE" << endl;
  os << "       shapeType       SOLID" << endl;
  os << "       faceType        CONVEX" << endl;
  os << "       creaseAngle     30.0" << endl;
  os << "    }" << endl; 
  
  os << "    Separator {" << endl; 
  os << "       Coordinate3 {" << endl;
  os << "           point       [" << endl;
  
  double umin = surf.umin();
  double vmin = surf.vmin();
  double du = (surf.umax() - umin)/(double)(noU-1);
  double dv = (surf.vmax() - vmin)/(double)(noV-1);  
    
  int i,j;
  for (j=0; j<noV; j++) {
    double v = vmin + (double)j*dv;
    for(i=0; i<noU; i++) {
      double u = umin + (double)i*du;
      double z = surf.f(u,v);
      if (bscale)
        z *= scale;
      
      os << u - umin << " " << v - vmin << " " << z; 
      
      if(i < noU-1 || j < noV-1) 
        os << ",\n";
    }
  }
  os << "]\n";
  os << "}\n"; 
  
  os << " IndexedFaceSet {\n";
  os << "    coordIndex [\n" << flush;
  
  for(j=0; j<noV-1; j++) {
    for(i=0; i<noU-1; i++) {
      os << j*noU+i << ", " << j*noU+i+1 << ", " << (j+1)*noU+i+1 << ", " << (j+1)*noU+i << ", " << -1 << "," << endl;
    }
    os << "\n";
  }
  os << "       ]\n"; 
  os << "    }\n"; 
  os << "  }\n"; 
  
    
  os << "}\n"; 
}

void UCBspl::printGNUgrid (const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV) {
  
#ifdef MBA_DEBUG
  cout << "Printing grid to: " << filename << endl;
#endif
  
  double du = (surf.umax() - surf.umin())/(double)(noU-1);
  double dv = (surf.vmax() - surf.vmin())/(double)(noV-1);  
  ofstream ofile(filename);
  
  for (int j = 0; j < noV; j++) {      
    double v = surf.vmin() + j*dv;
    for (int i = 0; i < noU; i++) {
      double u = surf.umin() + i*du;
      
      ofile << surf.f(u,v) << "\n";
    }
    ofile << endl;
  }  
}

// rcasero: method to print the grid to a CSV file
void UCBspl::printCSVgrid (const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV) {
  
#ifdef MBA_DEBUG
  cout << "Printing grid to: " << filename << endl;
#endif
  
  double du = (surf.umax() - surf.umin())/(double)(noU-1);
  double dv = (surf.vmax() - surf.vmin())/(double)(noV-1);  
  ofstream ofile(filename);
  
  for (int i = 0; i < noU; i++) {
    double u = surf.umin() + i*du;
    for (int j = 0; j < noV; j++) {      
      double v = surf.vmin() + j*dv;
      
      if (i == 0) {
	ofile << surf.f(u,v);
      } else {
	ofile << "," << surf.f(u,v);
      }
    }
    ofile << endl;
  }  
}


void UCBspl::printVTKtriangleStrips(const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV, double scale) {
  
  cout << "Printing grid to vtk poly data file with triangle strips...." << endl;

  double umin = surf.umin();
  double vmin = surf.vmin();
  double umax = surf.umax();
  double vmax = surf.vmax();
  double du = (umax - umin)/(double)(noU-1);
  double dv = (vmax - vmin)/(double)(noV-1);  
  int noPoints = noU*noV;

  ofstream os(filename);

  os << "# vtk DataFile Version 2.0" << endl;
  os << "This file was generated by class UCButils (triangle strips)" << endl;
  os << "ASCII" << endl;
  os << "DATASET POLYDATA" << endl;
  os << "POINTS " << noPoints << " float\n";

  int i,j;
  for (j = 0; j < noV; j++) {
    double v = vmin + j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + i*du;      
      os << u << " " << v << " " << surf.f(u,v)*scale << '\n';
    }
  }

  int noStrips = noV-1;
  int size = (1 + noU*2)*(noV - 1);
  os << "TRIANGLE_STRIPS " << noStrips << " " << size << endl;

  const int izoff = noU + 1; 
  int indz = 0;
  for (j = 0; j < noV-1; j++) {
    os << 2*noU << " ";  
    os << indz+noU << " "; 
    for (int i = 0; i < noU-1; i++) {
			
      os << indz << " "; 

      os << indz + izoff << " ";
			indz++;
    }
		os << indz << '\n';
    indz++;
  }

  double gx,gy,gz;
  os << "POINT_DATA " << noPoints << '\n';
  os << "NORMALS normals float" << '\n';
  for (j = 0; j < noV; j++) {
    double v = vmin + j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + i*du;
      surf.normalVector(u,v,gx,gy,gz);
      os << gx << " " << gy << " " << gz << '\n';
    }
  }
}




void UCBspl::printVTKgrid(const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV, double scale) {

  cout << "Printing grid to vtk structured points file......" << endl;

  double umin = surf.umin();
  double vmin = surf.vmin();
  double umax = surf.umax();
  double vmax = surf.vmax();
  double du = (umax - umin)/(double)(noU-1);
  double dv = (vmax - vmin)/(double)(noV-1);  

  ofstream os(filename);

  os << "# vtk DataFile Version 2.0" << endl;
  os << "This file was generated by class UCButils (structured points)" << endl;
  os << "ASCII" << endl;
  os << "DATASET STRUCTURED_POINTS" << endl;
  os << "DIMENSIONS " << noU << " " << noV << " " << 1 << endl;
  os << "ORIGIN " << umin << " " << vmin << " " << 999 << endl;
  os << "SPACING " << du << " " << dv << " " << 999 << endl;
  os << "POINT_DATA " << noU*noV*1 << endl;
  os << "SCALARS volume_scalars float" << endl;
  os << "LOOKUP_TABLE default" << endl;

  
  
  int i,j;
  for (j = 0; j < noV; j++) {
    double v = vmin + j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + i*du;      
      os << surf.f(u,v)*scale << '\n';
    }
  }
}


void UCBspl::printIRAPgrid(const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV) {
  
#ifdef MBA_DEBUG
  cout << "Printing grid to Irap grid file (format may be wrong): " << filename << endl;
#endif
  
  double du = (surf.umax() - surf.umin())/(double)(noU-1);
  double dv = (surf.vmax() - surf.vmin())/(double)(noV-1);  
  ofstream ofile(filename);
  ofile << noU << " " << noV << " " << du << " " << dv << endl;
  ofile << surf.umin() << " " << surf.umax() << " " << surf.vmin() << " " << surf.vmax() << endl;
  
  
  for (int j = 0; j < noV; j++) {
    double v = surf.vmin() + j*dv;
    for (int i = 0; i < noU; i++) {
      double u = surf.umin() + i*du;
      
      ofile << surf.f(u,v) << "\n";
    }
  }
}

void UCBspl::printGLgrid(const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV) {
  
#ifdef MBA_DEBUG
  cout << "Printing GridStrips......" << endl;
#endif
  
  double umin = surf.umin();
  double vmin = surf.vmin();
  double umax = surf.umax();
  double vmax = surf.vmax();
  double du = (umax - umin)/(double)(noU-1);
  double dv = (vmax - vmin)/(double)(noV-1);  
  
  ofstream os(filename);
  
  os << noU << " " << noV << " " << umin << " " << vmin << " " << du << " " << dv << endl;
  
  int i,j;
  for (j = 0; j < noV; j++) {
    double v = vmin + j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + i*du;      
      os << surf.f(u,v) << "\n";
    }
  }
  
  double gx,gy,gz;
  
  for (j = 0; j < noV; j++) {
    double v = vmin + (double)j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + (double)i*du;
      surf.normalVector(u,v,gx,gy,gz);
      os << gx << " " << gy << " " << gz << endl;
    }
  }
}


void UCBspl::printGLgridBin(const char filename[], const UCBspl::SplineSurface& surf, int noU, int noV,
                              const std::vector<double>& U, const std::vector<double>& V, const std::vector<double>& Z, 
                              double scale) {
  
#ifdef MBA_DEBUG
  cout << "Printing GridStrips (binary)......" << endl;
#endif
  

  double umin = surf.umin();
  double vmin = surf.vmin();
  double umax = surf.umax();
  double vmax = surf.vmax();
  double du = (umax - umin)/(double)(noU-1);
  double dv = (vmax - vmin)/(double)(noV-1);  
  
  FILE* fp = fopen(filename,"wb");
  
  fwrite(&noU,sizeof(int),1,fp);
  fwrite(&noV,sizeof(int),1,fp);
  fwrite(&umin,sizeof(double),1,fp);
  fwrite(&vmin,sizeof(double),1,fp);
  fwrite(&du,sizeof(double),1,fp);
  fwrite(&dv,sizeof(double),1,fp);
  
  int i,j;
  for (j = 0; j < noV; j++) {
    double v = vmin + j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + i*du;      
      double z = surf.f(u,v) * scale;
      fwrite(&z,sizeof(double),1,fp);
    }
  }
  
  double gx,gy,gz;
  
  for (j = 0; j < noV; j++) {
    double v = vmin + (double)j*dv;
    for (i = 0; i < noU; i++) {
      double u = umin + (double)i*du;
      surf.normalVector(u,v,gx,gy,gz);
      fwrite(&gx,sizeof(double),1,fp);
      fwrite(&gy,sizeof(double),1,fp);
      fwrite(&gz,sizeof(double),1,fp);
    }
  }
  
  int noPoints = U.size();
  fwrite(&noPoints,sizeof(int),1,fp);
  
#ifdef MBA_DEBUG
  cout << "No. points = " << noPoints << endl;
#endif

  for (i = 0; i < noPoints; i++) {
    double u = U[i];
    double v = V[i];
    double z = Z[i] * scale;
    
    fwrite(&u,sizeof(double),1,fp);
    fwrite(&v,sizeof(double),1,fp);
    fwrite(&z,sizeof(double),1,fp);
  }
  
  fclose(fp);
}


void UCBspl::saveSplineSurface(const char filename[], const UCBspl::SplineSurface& surf) {
  
#ifdef MBA_DEBUG
  cout << "Writing spline surface to ascii file: " << filename << endl;
#endif
  
  ofstream os(filename);
  
  os << surf.umin() << '\n';
  os << surf.vmin() << '\n';
  os << surf.umax() << '\n';
  os << surf.vmax() << '\n';
    
    
  
  const GenMatrix<UCBspl_real>& PHI = *surf.getCoefficients();
  int noX = PHI.noX();
  int noY = PHI.noY(); 
  
  os << noX << " " << noY << '\n';
  for (int i = 0; i < noX; i++) {
    for (int j = 0; j < noY; j++) {
      os << PHI(i-1,j-1) << '\n';
    }
  }
}

void UCBspl::saveSplineSurfaceBin(const char filename[], const UCBspl::SplineSurface& surf) {
  
#ifdef MBA_DEBUG
  cout << "Writing spline surface to binary file: " << filename << endl;
#endif
  
  
  FILE* fp = fopen(filename,"wb");
  
  double umin,vmin,umax,vmax;
  surf.getDomain(umin,vmin,umax,vmax);
  fwrite(&umin,sizeof(double),1,fp);
  fwrite(&vmin,sizeof(double),1,fp);
  fwrite(&umax,sizeof(double),1,fp);
  fwrite(&vmax,sizeof(double),1,fp);
  
  
    
  const GenMatrix<UCBspl_real>& PHI = *surf.getCoefficients();
  int noX = PHI.noX();
  int noY = PHI.noY(); 
  
  fwrite(&noX,sizeof(int),1,fp);
  fwrite(&noY,sizeof(int),1,fp);
  
  for (int i = 0; i < noX; i++) {
    for (int j = 0; j < noY; j++) {
      UCBspl_real coeff = PHI(i-1,j-1);
      fwrite(&coeff,sizeof(UCBspl_real),1,fp);
    }
  }
  
  fclose(fp);
}


void UCBspl::readSplineSurface(const char filename[], UCBspl::SplineSurface& surf) {
  
#ifdef MBA_DEBUG
  cout << "Reading spline surface surface from ascii file: " << filename << endl;
#endif
  
  ifstream is(filename);
  if (!is) {
    cout << "ERROR: File does not exist (or empty?)\n" << endl;
    exit(-1);
  }
  
  double umin, vmin, umax, vmax;
  is >> umin;
  is >> vmin;
  is >> umax;
  is >> vmax;
  
  double offset;
  is >> offset;
  
  int cont;
  is >> cont;
  
  int noX, noY;
  is >> noX;
  is >> noY;
#ifdef MBA_DEBUG
  cout << "Size of surface = " << noX << " X " << noY << endl;
#endif
  
  boost::shared_ptr<GenMatrixType> PHI(new GenMatrix<UCBspl_real>(noX, noY));
  for (int i = 0; i < noX; i++) {
    for (int j = 0; j < noY; j++) {
      is >> (*PHI)(i-1,j-1);
    }
  }
  
  
  surf.init(PHI, umin, vmin, umax, vmax);
}


void UCBspl::readSplineSurfaceBin(const char filename[], UCBspl::SplineSurface& surf) {

#ifdef MBA_DEBUG
  cout << "Reading spline surface surface from binary file: " << filename << endl;
#endif
  
  FILE* fp = fopen(filename,"rb");
  
  if (fp == NULL) {
    cout << "ERROR: File does not exist (or empty?)\n" << endl;
    exit(-1);
  }
  
#ifdef MBA_DEBUG
  MBAclock rolex;
#endif
  double umin, vmin, umax, vmax;
  fread(&umin,sizeof(double),1,fp);
  fread(&vmin,sizeof(double),1,fp);
  fread(&umax,sizeof(double),1,fp);
  fread(&vmax,sizeof(double),1,fp);
  
  double offset;
  fread(&offset,sizeof(double),1,fp);
  
  int cont;
  fread(&cont,sizeof(int),1,fp);
  
  
  int noX, noY;
  fread(&noX,sizeof(int),1,fp);
  fread(&noY,sizeof(int),1,fp);
  
#ifdef MBA_DEBUG
  cout << "Size of surface = " << noX << " X " << noY << endl;
#endif
  
  boost::shared_ptr<GenMatrixType> PHI(new GenMatrix<UCBspl_real>(noX, noY));
  UCBspl_real coef;
  for (int i = 0; i < noX; i++) {
    for (int j = 0; j < noY; j++) {
      fread(&coef,sizeof(UCBspl_real),1,fp);
      (*PHI)(i-1,j-1) = coef;
    }
  }
  fclose(fp);
#ifdef MBA_DEBUG
  cout << "Time used on reading data = " << rolex.getInterval() << endl;
#endif
  
  surf.init(PHI, umin, vmin, umax, vmax);
}

 
