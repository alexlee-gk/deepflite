% General purpose blob object for saving data blobs in h5 file format. Stores
% image data in MATLAB format h x w x c x N and other data (e.g. state) 
% in c x N arrays. use getCaffe() to get the blobs in caffe format,
% that is, WxHxCxN and 1x1xCxN
classdef Blob < handle
    properties
        data        % array containing the data
        name
        id          % id (integer) of this blob
    end
    
    methods
        function blob = Blob(name, id)
            if nargin > 0
                blob.name = name;
                blob.id = id;
                blob.data = [];
            end
        end
    end
    methods (Static)
        
        blob         = readH5(fin, id, blobName);
        [mu, sigma2] = meanVar(blobs);
        blobsNorm    = normalize(blobs, mu, sigma2);
        blobs_out    = unnormalize(blobs_in, mu, sigma2)
        out          = getCaffe(blobs, ind)
        
        saveH5(fout, blobs);
        listFileNames(fout, blobNames, trainInd, testInd, singleInd);
        saveMeanVar(fout, blobName, mean, var)
        [mu, sigma2] = loadMeanVar(fout, blobName)
        
        blotout = cat(blobs);
    end
end
    
