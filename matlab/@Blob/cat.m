function blobout = cat(blobs)
% creates a new blob with the data concateated along last dim

blobout  = Blob(blobs(1).name, blobs(1).id);

data{length(blobs)} = [];

for i=1:length(blobs)
    data{i} = blobs(i).data;
end

dim = size(blobs(1).data);
blobout.data = cat(length(dim), data{:});



