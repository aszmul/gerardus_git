function stats = scinrrd_seg2label_stats(nrrd, cc, d, dict)
% SCINRRD_SEG2LABEL_STATS  Shape stats for each object in a multi-label
% segmentation; objects can be straightened with an skeleton or medial line
% before computing the stats
%
%   This function was developed to measure the dimensions of different
%   objects found in a segmentation. 
%
%   Each voxel in the input segmentation has to be labelled as belonging to
%   the different objects or the background.
%
%   This function then computes Principal Component Analysis (PCA) on the
%   voxels of each object, to estimate the variance in the 3 principal
%   directions, var(1), var(2) and var(3).
%
%   If the object is e.g. a cylindrical vessel with elliptical
%   cross-section, the length and the 2 main diamters of the cylinder can
%   be estimated as
%
%      L = sqrt(12 * var(1))
%      d1 = sqrt(16 * var(2))
%      d2 = sqrt(16 * var(3))
%
%   Because vessels are quite often curved or bent, and this gives
%   misleading values of variance, the function can also straighten the
%   objects using a skeleton or medial line prior to computing PCA.
%
% STATS = SCINRRD_SEG2LABEL_STATS(NRRD)
%
%   NRRD is an SCI NRRD struct with a labelled segmentation mask.
%
%   All voxels in nrrd.data with value 0 belong to the background. All
%   voxels with value 1 belong to object 1, value 2 corresponds to object
%   2, and so on.
%
%   STATS is a struct with the shape parameters computed for each object in
%   the segmentation. The only measure provided currently is
%
%     STATS.VAR: Variance in the three principal components of the cloud
%                of voxels that belong to each object. These are the
%                ordered eigenvalues obtained from computing Principal
%                Component Analysis on the voxel coordinates.
%
%   If an object has no voxels, the corresponding STATS values are NaN.
%
% STATS = SCINRRD_SEG2LABEL_STATS(NRRD, CC)
%
%   If CC is provided, then the function straightens each object before
%   computing PCA.
%
%   CC is a struct produced by function skeleton_label() with the list of
%   skeleton voxels that belongs to each object, and the parameterization
%   vector for the skeleton.
%
%   The labels can be created, e.g.
%
%     >> nrrd = seg;
%     >> [nrrd.data, cc] = skeleton_label(sk, seg.data, [seg.axis.spacing]);
%
%     where seg is an NRRD struct with a binary segmentation, and sk is the
%     corresponding skeleton, that can be computed using
%
%     >> sk = itk_imfilter('skel', seg.data);
%
% STATS = SCINRRD_SEG2LABEL_STATS(..., D, DICT)
%
%   D is the sparse distance matrix for adjacent voxels in the
%   segmentation. DICT is the dictionary vector to convert between image
%   linear indices and distance matrix indices. They can be computed using
%
%     >> [d, dict] = seg2dmat(seg.data, 'seg', [seg.axis.spacing]);
%
%   If they are not provided externally, they are computed internally.
%
% See also: skeleton_label, seg2dmat, scinrrd_seg2voxel_stats.

% Author: Ramon Casero <rcasero@gmail.com>
% Copyright © 2011 University of Oxford
% Version: 0.2.0
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
error(nargchk(1, 4, nargin, 'struct'));
error(nargoutchk(0, 1, nargout, 'struct'));

% defaults
if (nargin < 2)
    cc = [];
end
if (nargin < 4 || isempty(d) || isempty(dict))
    % compute distance matrix for the whole image segmentation; this is a
    % very sparse matrix because only adjacent voxels get a connection
    [d, dict] = seg2dmat(nrrd.data, 'seg', [nrrd.axis.spacing]);
end

% number of objects
N = max(nrrd.data(:));
if (~isempty(cc) && (N ~= cc.NumObjects))
    error('If CC is provided, then it must have one element per object in NRRD')
end

% init matrix to store the eigenvalues of each branch
eigd = nan(3, N);

% loop every branch
for I = 1:N
    
    %% find the 3 closest skeleton points for each branch voxel
    
    % list of voxels in current branch
    br = find(nrrd.data == I);
    
    % if there are no voxels, skip this branch
    if (isempty(br))
        continue
    end
        
    % coordinates of branch voxels
    [r, c, s] = ind2sub(size(nrrd.data), br);
    xi = scinrrd_index2world([r, c, s], nrrd.axis)';
    
    % straighten all branch voxels using a local rigid transformation
    if (~isempty(cc))
        
        % list of voxels that are part of the skeleton in the branch
        sk = cc.PixelIdxList{I};
        
        % coordinates of skeleton voxels
        [r, c, s] = ind2sub(size(nrrd.data), sk);
        x = scinrrd_index2world([r, c, s], nrrd.axis)';
    
        % create a distance matrix for all the voxels in the branch (this
        % includes the skeleton)
        aux = dict(br);
        dbr = d(aux, aux);
        dictbr = sparse(br, ones(length(br), 1), 1:length(br));
        
        % convert image indices to distance matrix indices
        sk = dictbr(sk);
        
        % compute distance from each branch voxels to all skeleton points
        dbrsk = dijkstra(dbr, sk);
        
        % keep only the closest skeleton point association
        [~, idx] = sort(dbrsk, 1, 'ascend');
        idx = idx(1, :);
        
        % create a straightened section of the skeleton of the same length
        % and with the same spacing between voxels
        y = [cc.PixelParam{I}' ; zeros(2, length(sk))];
        
        % straighten branch voxels
        yi = pts_local_rigid(x', y', xi', idx)';

    else % don't straighten objects
        
        yi = xi;
        
    end
    
    %% compute statistics about the shape of the branch
    
    % compute eigenvalues of branch
    [~, eigd(:, I)] = pts_pca(yi);
    
end

% create output struct
stats.var = eigd;
