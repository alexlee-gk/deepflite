function blobOut = imResize(blobIn, h, w, newName)
% blob    : images to be resized
% h       : new height
% w       : new width
% newName : name of the resized blob
% blobOut : resized images


if nargin < 4
    blobOut = Blob([blobIn.name '_resized'], blobIn.id);
else
    blobOut = Blob(newName, blobIn.id);
end


imCell = arrayfun(@(frame)imresize(blobIn.data(:,:,:,frame), [h w]), 1:blobIn.len(),...
    'UniformOutput', false);

blobOut.data = cat(4, imCell{:});