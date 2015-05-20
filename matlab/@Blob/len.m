function len = len(blob)
% retuns the number of samples or throws error if fields are inconsistent

    dim = size(blob.data);
    len = dim(end);