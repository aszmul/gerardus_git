function nrrd = scinrrd_surface_interpolation(nrrd, x, PARAM, INTERP, KLIM)
% SCINRRD_SURFACE_INTERPOLATION  Interpolate a surface and create a
% segmentation mask from a scattered set of points
%
% NRRD = SCINRRD_SURFACE_INTERPOLATION(NRRD0, X)
%
%   NRRD0 is the SCI NRRD struct that contains the image.
%
%   X is a 3-row matrix. Each column has the coordinates of a point that
%   belongs to the surface we want to interpolate.
%
%   NRRD is a SCI NRRD struct with a segmentation of the surface that
%   interpolates the points in X.
%
% NRRD = SCINRRD_SURFACE_INTERPOLATION(NRRD0, X, PARAM, INTERP, KLIM)
%
%   PARAM is a string with the method used to parametrize the surface and
%   X. For options, see help surface_interpolation.m.
%
%   INTERP is a string with the interpolation method. For options, see help
%   surface_interpolation.m.
%
%   KLIM is a scalar factor for the extension of the interpolation domain.
%   By default, KLIM=1 and the interpolation domain is a rectangle that
%   tightly contains X. Sections of the interpolated surface that protude
%   from the image volume are removed.
%
%
%   Note on SCI NRRD: Software applications developed at the University of
%   Utah Scientific Computing and Imaging (SCI) Institute, e.g. Seg3D,
%   internally use NRRD volumes to store medical data.
%
%   When label volumes (segmentation masks) are saved to a Matlab file
%   (.mat), they use a struct called "scirunnrrd" to store all the NRRD
%   information:
%
%   >>  scirunnrrd
%
%   scirunnrrd = 
%
%          data: [4-D uint8]
%          axis: [4x1 struct]
%      property: []

% Author: Ramon Casero <rcasero@gmail.com>
% Copyright © 2010-2011 University of Oxford
% Version: 0.3
% $Rev$
% $Date$
% 
% University of Oxford means the Chancellor, Masters and Scholars of
% the University of Oxford, having an administrative office at
% Wellington Square, Oxford OX1 2JD, UK. 
%
% This file is part of Gerardus.
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details. The offer of this
% program under the terms of the License is subject to the License
% being interpreted in accordance with English Law and subject to any
% action against the University of Oxford being under the jurisdiction
% of the English Courts.
%
% You should have received a copy of the GNU General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>.

% check arguments
error(nargchk(2, 5, nargin, 'struct'));
error(nargoutchk(0, 1, nargout, 'struct'));

% defaults
if (nargin < 3 || isempty(PARAM))
    PARAM='xy';
end
if (nargin < 4 || isempty(INTERP))
    INTERP='tps';
end
if (nargin < 5 || isempty(KLIM))
    KLIM=1;
end

% extract data volume and convert to boolean to save space
nrrd.data = boolean(nrrd.data);

% get voxel size
res = [nrrd.axis.spacing];

%% compute interpolating surface
y = surface_interpolation(x, PARAM, INTERP, res, KLIM);

%% map interpolated surface points to voxels

% convert real world coordinates to indices
idx = round(scinrrd_world2index(y, nrrd.axis));

% remove points outside the volume
badidx = isnan(sum(idx, 2));
idx = idx(~badidx, :);

% construct new segmentation mask. Note that zz has type double.
nrrd.data = nrrd.data * 0;
nrrd.data(sub2ind(size(nrrd.data), ...
    idx(:, 1), idx(:, 2), floor(idx(:, 3)))) = 1;
nrrd.data(sub2ind(size(nrrd.data), ...
    idx(:, 1), idx(:, 2), ceil(idx(:, 3)))) = 1;
