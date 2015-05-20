function blob = readH5(fin, id, blobName)
% creates a new blob object(s) from HDF5 file
% fin   : input path string. use %blob and %id for blob name and id. 
% id    : blob id(s)
% blob  : blob object or array of blobs if multiple id's were given


    blob(length(id)) = Blob();
    for i=1:length(id)
        % create empty Blob object
        blob(i) = Blob(blobName, id(i));

        fullpath = strrep(strrep(fin, '%blob', blobName), '%id', num2str(id(i)));

        % read in the blob, permute and convert it into matlab form
        data = h5read(fullpath, ['/' blobName]);
        
        dim = size(data);
        % permute if the blob is an image
        if dim(1) == 1 && dim(2) == 1
            blob(i).data = reshape(data, [], dim(4));
        else
            blob(i).data = permute(data, [2 1 3 4]);
        end
    end
    
    % for convenience, return just the object (without object array) if
    % only one blob id was given.
    if length(id) == 1
        blob = blob(1);
    end
    
end


        
