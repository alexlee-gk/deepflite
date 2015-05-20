function catCaffe(this, blob)
% concatenates along last dimension.
%
% this  : handle to blob object
% blob  : an array in form WxHxCxN. For non-image blobs, W=H=1.


    dim = size(blob);
    if dim(1) == 1 && dim(2) == 1
        this.data = cat(2, this.data, squeeze(permute(blob, [3 4 1 2])));
    else
       this.data = cat(4, this.data, permute(blob, [2 1 3 4]));
    end
