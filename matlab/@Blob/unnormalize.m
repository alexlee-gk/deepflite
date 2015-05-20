function blobs_out = unnormalize(blobs_in, mu, sigma2)
    
    % if only a single blob was given, store it into an array first
    if length(blobs_in) == 1
        blobs_in(1) = blobs_in;
    end
    
    blobs_out(length(blobs_in)) = Blob();
    for i=1:length(blobs_in)
        blobs_out(i) = Blob(blobs_in(i).name(1:end-5), blobs_in(i).id);
        blobs_out(i).data = bsxfun(@plus, bsxfun(@times, sigma2.^(1/2), blobs_in(i).data), mu);
    end
    
    % for convenience, return just the object (without object array) if
    % only one blob was given.
    if length(blobs_in) == 1
        blobs_out = blobs_out(1);
    end
    
end

