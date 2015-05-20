function out = getCaffe(blobs, ind)
% returns single array of data in Caffe format WxHxCxN
%
% blobs     : array of blobs (or single blob) to be converted
% ind       : data indices, default 1:len
%
% out       : cell array of WxHxCxN arrays

    if nargin == 1
        ind = 1:blobs(1).length();
    end

    
    out = cell(length(blobs),1);
    for i=1:length(blobs)
        if blobs(i).dim() == 2
            out{i} = permute(single(blobs(i).data(:,ind)), [3 4 1 2]);
        else
            out{i} = permute(single(blobs(i).data(:,:,:,ind)), [2 1 3 4]);
        end
    end
    