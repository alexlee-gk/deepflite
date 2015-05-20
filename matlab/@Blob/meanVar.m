function [mu, sigma2] = meanVar(blobs)
% returns the mean and the variance of the input blobs
%
% blobs     : array of blobs (or single blob handle). Does not work for
%             image blobs. Assumes that each blob has the same number of
%             samples!

    if blobs(1).dim > 2
        error('Cannot find the mean of an image')
    end

    if length(blobs) == 1
        blobs(1) = blobs;
    end
    
    
    bigBlob = cat(2, blobs.data);
    
    mu = mean(bigBlob,2);
    sigma2 = var(bigBlob,[],2);