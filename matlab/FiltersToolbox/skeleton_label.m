function [sk, cc] = skeleton_label(sk, im, res)
% SKELETON_LABEL  Give each branch of a skeleton a different label, and
% sort the voxels within each branch
%
% [LAB, CC] = SKELETON_LABEL(SK)
%
%   SK is a 3D segmentation mask. SK is assumed to be a skeleton resulting
%   from some kind of thinning algorithm, e.g. the C++ program
%   skeletonize3DSegmentation provided by Gerardus. That is, the
%   segmentation looks like a series of 1 voxel-thick branches connected
%   between them (cycles are allowed).
%
%   LAB is an image of the same dimensions of SK where the value of each
%   voxel is the label of the branch it belongs to.
%
%   Voxels in bifurcations are labelled as the nearest branch. Voxels
%   completely surrounded by other bifurcation voxels are assigned the
%   nearest label arbitrarily. Some voxels may remain unlabelled, e.g. if
%   they are not connected to any others in the segmentation.
%
%   CC is a struct like those provided by Matlab's function bwconncomp().
%   Each vector in CC.PixelIdxList{i} has more or less the list of image
%   indices of the voxels with label i. The reason why it's not exactly the
%   same is because:
%
%     - We impose each branch to have two termination voxels. Termination
%     voxels can be leaves or bifurcation points. Bifurcation points can be
%     shared by more than one branch, so in that case they need to be
%     repeated in CC, while in SK the bifurcation voxel is assigned to only
%     one branch.
%
%     - Some bifurcation voxels are completely surrounded by other
%     bifurcation voxels. The former are not included in CC, but they are
%     given a label in SK.
%
% ... = SKELETON_LABEL(SK, IM, RES)
%
%   IM is the original 3D the skeleton SK was computed from. In this case,
%   LAB has the labelling of IM. If you want to extract the labelling for
%   SK, just run
%
%     >> LAB .* SK
%
%   RES is a 3-vector with the voxel size as [row, column, slice]. By
%   default, RES=[1 1 1].

% Author: Ramon Casero <rcasero@gmail.com>
% Copyright © 2011 University of Oxford
% Version: 0.3.0
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
error(nargchk(1, 3, nargin, 'struct'));
error(nargoutchk(0, 2, nargout, 'struct'));

% defaults
if (nargin < 2)
    im = [];
end
if (nargin < 3 || isempty(res))
    res = [1 1 1];
end

%% Label skeleton voxels

% get sparse matrix of distances between voxels. To label the skeleton we
% don't care about the actual distances, just need to know which voxels are
% connected to others. Actual distances will be needed if we have to
% segment the original segmentation, though
[dsk, dictsk, idictsk] = seg2dmat(sk, 'seg');

% compute degree of each voxel
deg = sum(dsk > 0, 2);

% get distance matrix index of the bifurcation voxels
idx = deg >= 3;

% convert distance matrix index to image index
idx = idictsk(idx);

% remove bifurcation voxels from image
sk(idx) = 0;

% get connected components in the image
cc = bwconncomp(sk);

% set amount of memory allowed for labels
req_bits = ceil(log2(cc.NumObjects));
if (req_bits == 1)
    lab_class = 'bool';
elseif (req_bits < 8)
    lab_class = 'uint8';
elseif (req_bits < 16)
    lab_class = 'uint16';
elseif (req_bits < 32)
    lab_class = 'uint32';
elseif (req_bits < 64)
    lab_class = 'uint64';
else
    lab_class = 'uint64';
    warning('Too many labels. There are more than 2^64 labels. Using uint64, some labels will be lost');
end
if ~strcmp(lab_class, class(sk)) % we need a different class than the input image so that we can represent all labels
    sk = zeros(size(sk), lab_class);
else
    sk = sk * 0;
end

% give each voxel in the image its label
for lab = 1:cc.NumObjects
    sk(cc.PixelIdxList{lab}) = lab;
end

% keep log of voxels that have been given a label
withlab = false(size(deg));
withlab(deg < 3) = true;

% loop each bifurcation voxel (working with matrix indices, not image
% indices)
for v = find(deg >= 3)'
    % get its neighbours that are not bifurcation voxels
    vn = find(dsk(v, :));
    vn = vn(deg(vn) < 3);
    
    % if this is a bifurcation voxel that is completely surroundered by
    % other bifurcation voxels, we are not going to label it
    if ~isempty(vn)
        % get all its neighbour's labels
        vnlab = sk(idictsk(vn));
        
        % add the bifurcation point to each branch that it finishes
        for I = 1:length(vnlab)
            cc.PixelIdxList{vnlab(I)} = ...
                [cc.PixelIdxList{vnlab(I)}; idictsk(v)];
            sk(idictsk(v)) = vnlab(I);
        end
        
        % record this in the log
        withlab(v) = true;
    end
end

% loop any voxels that have been left without a label, and assign them one
% of a neighbour's
for v = find(~withlab)'
    % get its neighbours that are not bifurcation voxels
    vn = find(dsk(v, :));
    
    % get its first neighbour's label
    vnlab = sk(idictsk(vn(1)));
    
    % if the label is 0, that means that this voxel is surrounded by voxels
    % that have not been labelled yet, and we leave it unlabelled
    if vnlab
        % give it the neighbour's label (we need every voxel in the tree to
        % have a label, otherwise we cannot extend the segmentation)
        % but don't add it to the list of voxels in the branch (we want
        % every branch to contain no more than one bifurcation point)
        sk(idictsk(v)) = vnlab;
%         cc.PixelIdxList{vnlab} = [cc.PixelIdxList{vnlab}; idictsk(v)];
        
        % record this in the log
        withlab(v) = true;
    end
end

%% Label all voxels in the original segmentation, not only skeleton

if (~isempty(im))
    
    % now we need the distances between neighbours for the whole segmentation
    [d, dict] = seg2dmat(im, 'seg', res);
    
    % for each original segmentation voxel, find the closest skeleton voxel
    nn = graph_nn(d, [], dict(sk(:)>0));
    
    % label each segmentation voxel with the same label as the corresponding
    % skeleton voxel
    sk(im(:)>0) = sk(idictsk(nn));
    
    % free up memory
    clear d dict    
end

% % Debug (2D image) %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% 
% hold off
% imagesc(sk)
% axis xy
% 
% % find tree leaves: convert matrix indices to image linear indices
% idx = idictsk(deg == 1);
% 
% % convert image linear indices to row, col, slice
% [r, c, s] = ind2sub(size(sk), idx);
% 
% % plot leaves
% hold on
% plot(c, r, 'ro')
% 
% % find tree bifurcations: convert matrix indices to image linear indices
% idx = idictsk(deg >= 3);
% 
% % convert image linear indices to row, col, slice
% [r, c, s] = ind2sub(size(sk), idx);
% 
% % plot leaves
% plot(c, r, 'go')
% 
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Sort the voxels in each branch

% loop each branch in the skeleton
for I = 1:cc.NumObjects
    % list of voxels in current branch
    br = cc.PixelIdxList{I};
    
    % number of voxels in the current branch
    N = length(br);
    
    % find the two extreme voxels of the current branch
    idx = dictsk(br(deg(dictsk(br)) ~= 2));
    
    if (length(idx) ~= 2)
        error('Assertion fail: Each branch is expected to have two termination nodes')
    end

    % create a distance matrix for only the voxels in the branch
    dbr = 0 * dsk;
    aux = dictsk(br);
    dbr(aux, aux) = dsk(aux, aux);

    % save the initial extreme points for later
    v0 = idx(1);
    v1 = idx(2);
    
    % compute shortest from the extreme voxel to every other voxel in the
    % branch, and reuse variable
    [dbr, p] = dijkstra(dbr, v0);
    
    % convert Inf values in dbr to 0
    dbr(isinf(dbr)) = 0;
    
    % get the voxel that is furthest from the origin
    [~, idx] = max(dbr);
    
    if (idx ~= v1)
        error('Assertion fail: there is a voxel in the branch furthest from an extreme point than the other extreme point');
    end
    
    % backtrack the whole branch in order from the furthest point to the
    % original extreme point
    cc.PixelIdxList{I}(:) = 0;
    cc.PixelIdxList{I}(1) = idictsk(v1);
    J = 2;
    while (idx ~= v0)
        idx = p(idx);
        cc.PixelIdxList{I}(J) = idictsk(idx);
        J = J + 1;
    end
    
    if any(cc.PixelIdxList{I} == 0)
        error(['Assertion fail: We have lost at least 1 voxel in branch ' num2str(I)])
    end
    if (N ~= length(cc.PixelIdxList{I}))
        error(['Assertion fail: Sorting has changed the number of voxels in branch ' num2str(I)])
    end
    
end

% % Debug (2D image) %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% 
% for I = 1:cc.NumObjects
%     % list of voxels in current branch
%     br = cc.PixelIdxList{I};
%     
%     % convert image linear indices to image r, c, s indices
%     [r, c, s] = ind2sub(size(sk), br);
%     
%     % plot branch in order
%     plot(c, r, 'w')
% end
% 
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
