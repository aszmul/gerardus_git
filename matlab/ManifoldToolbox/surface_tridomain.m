function [tri, uv] = surface_tridomain(gridtype, inctype, inc, uvmin, uvmax, k)
% SURFACE_TRIDOMAIN  Triangular mesh to cover a planar or spherical domain
%
%   This function creates a triangular mesh on a planar (XY) or spherical
%   domain.
%
%   It can be used together with surface_param() and surface_interp() to
%   compute an interpolated open or closed surface.
%
% -------------------------------------------------------------------------
% Rectangular domains for open surfaces:
% -------------------------------------------------------------------------
%
% [TRI, UV] = surface_tridomain('rect', 'step', DELTA, UVMIN, UVMAX)
% [TRI, UV] = surface_tridomain('rect', 'num', N, UVMIN, UVMAX)
%
%   There are two alternatives to decide the number of grid points:
%
%     DELTA is a scalar or 2-vector with the spacing between grid points,
%     in the U and V coordinates.
%
%     N is a scalar or 2-vector with the number of grid points in the U and
%     V coordinates.
%
%   UVMIN, UVMAX are 2-vectors with the minimum and maximum coordinates of
%   the grid.
%
%   TRI is a 3-column matrix with a surface triangulation. Each row has the
%   indices of 3 vertices forming a spherical triangle.
%
%   UV is a 2-column matrix with the XY coordinates of the mesh vertices.
%
%   The mesh can be visualised with
%
%     trisurf(tri, uv(:, 1), uv(:, 2), zeros(size(uv, 1), 1))
%
% ... = surface_tridomain(..., K)
%
%   K is a scalar or 2-vector with the factor by which the domain size is
%   multiplied in the U and V coordinates. By default, K=[1 1].
%
% -------------------------------------------------------------------------
% Spherical domains for closed surfaces, constant angular step:
% -------------------------------------------------------------------------
%
% [TRI, UV] = surface_tridomain('sphang', 'step', DELTA)
% [TRI, UV] = surface_tridomain('sphang', 'num', N)
%
%   Similarly to rectangular parametrizations, DELTA is the angular spacing
%   for latitude and longitude, and N is the number of points.
%
%   There must be at least N=3 points in each coordinate.
%
%   TRI is a 3-column matrix with a surface triangulation. Each row has the
%   indices of 3 vertices forming a spherical triangle.
%
%   UV is a 2-column matrix with the spherical coordinates, UV=[LAT LON] of
%   the mesh vertices.
%
%   The mesh can be visualised with
%
%     [x, y, z] = sph2cart(uv(:, 2), uv(:, 1), 1);
%     trisurf(tri, x, y, z)
%
%
% See also: surface_param, surface_interp.

% Author: Ramon Casero <rcasero@gmail.com>
% Copyright © 2013 University of Oxford
% Version: 0.1.0
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
switch gridtype
    case 'rect'
        narginchk(5, 6);
    case 'sphang'
        narginchk(3, 3);
    otherwise
        error('Grid method not implemented')
end
nargoutchk(0, 2);

% defaults
if (nargin < 6 || isempty(k))
    k = [1 1];
end

% more input argument checks
if (strcmp(gridtype, 'rect'))
    if (~isvector(uvmin) || length(uvmin) ~= 2)
        error('UVMIN must be a 2-vector')
    end
    if (~isvector(uvmax) || length(uvmax) ~= 2)
        error('UVMAX must be a 2-vector')
    end
end
if (~isvector(inc) || length(inc) < 1 || length(inc) > 2)
    error('DELTA or N must be a scalar or 2-vector')
end
if (isscalar(inc))
    inc = [inc inc];
end
if (isscalar(k))
    k = [k k];
end

% type of grid
switch gridtype
    
    case 'rect'
        
        % if the user provided number of points for the grid, convert it to
        % step size
        if strcmp(inctype, 'num')
            inc = (uvmax - uvmin) ./ (inc - 1);
        end

        % box size and centroid
        sz = uvmax - uvmin;
        uvm = mean([uvmax; uvmin], 1);
        
        % extend the box if necessary
        uvmin = uvm - sz/2.*k;
        uvmax = uvm + sz/2.*k;
        
        % generate grid for the embedding box
        [vi, ui] = ndgrid(...
            uvmin(2):inc(2):uvmax(2), ...
            uvmin(1):inc(1):uvmax(1));
        
        % create triangles:
        % v1 ----- v3    |
        %    |  /        | y-coordinate
        %    | /         | 
        % v2 |/          |
        %
        % ---------------
        %   x-coordinate
        v = reshape(1:numel(ui), size(ui));
        v2 = v(2:end, 1:end-1);
        v3 = v(1:end-1, 2:end);
        v1 = v(1:end-1, 1:end-1);
        tri = [v1(:) v2(:) v3(:)];

        % create triangles:
        %       /| v3    |
        %      / |       | periodic
        %     /  |       | (longitude)
        % v2 ----- v4    |
        %
        % ---------------
        %   non-periodic (latitude)
        v4 = v(2:end, 2:end);
        tri = [tri; [v2(:) v3(:) v4(:)]];
        
        % create output coordinates
        uv = [ui(:) vi(:)];
        
    case 'sphang'
        
        % if the user provided number of points for the grid, convert it to
        % step size
        if strcmp(inctype, 'num')
            inc = [pi 2*pi] ./ (inc - [1 0]);
        end

        % create grid of angular values
        [loni, lati] = ndgrid(...
            -pi:inc(2):pi, ...
            -pi/2:inc(1):pi/2);
        
        % remove all the points that map to the south and north poles
        lati(:, [1 end]) = [];
        loni(:, [1 end]) = [];

        % create output with the coordinates of the vertices
        uv = [lati(:) loni(:)];
        
        % create triangles:
        % v1 ----- v3    |
        %    |  /        | periodic
        %    | /         | (longitude)
        % v2 |/          |
        %
        % ---------------
        %   non-periodic (latitude)
        v = reshape(1:numel(loni), size(loni));
        v2 = v([2:end 1], 1:end-1);
        v3 = v(:, 2:end);
        v1 = v(:, 1:end-1);
        tri = [v1(:) v2(:) v3(:)];
        
        % create triangles:
        %       /| v3    |
        %      / |       | periodic
        %     /  |       | (longitude)
        % v2 ----- v4    |
        %
        % ---------------
        %   non-periodic (latitude)
        v4 = v([2:end 1], 2:end);
        tri = [tri; [v2(:) v3(:) v4(:)]];
        
        % add south and north poles
        uv = [uv; -pi/2 0; pi/2 0];
        
        % add triangles to the south pole
        idx = size(uv, 1) - 1;
        tri = [tri; [v(:, 1) v([2:end 1], 1) idx*ones(size(v, 1), 1)]];
        
        % add triangles to the north pole
        idx = size(uv, 1);
        tri = [tri; [v(:, end) v([2:end 1], end) idx*ones(size(v, 1), 1)]];  % ui: tri
        
end
