function blobsNorm = normalize(blobs, mu, sigma2)
% if mean and variance are not given, use the mean and variance of the
% input blobs. Note, sigma2 is vector of variances, not covariance matrix

    if nargin == 1
        [mu, sigma2] = Blob.meanVar(blobs);
    end
    
    % if only a single blob was given, store it into an array first
    if length(blobs) == 1
        blobs(1) = blobs;
    end
    
    % simply assume sigma is invertible
    sigmainv = sigma2.^(-1/2);
    
    blobsNorm(length(blobs)) = Blob();
    for i=1:length(blobs)
        blobsNorm(i) = Blob([blobs(i).name '_norm'], blobs(i).id);
        
        blobsNorm(i).data = bsxfun(@times, sigmainv, bsxfun(@minus, blobs(i).data, mu));
    end
    
    % for convenience, return just the object (without object array) if
    % only one blob was given.
    if length(blobs) == 1
        blobsNorm = blobsNorm(1);
    end
    
end

