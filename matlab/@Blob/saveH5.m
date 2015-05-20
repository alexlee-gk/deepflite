function saveH5(fout, blobs)
% saves image blobs as W*H*C*N matrices. Images should be normalized (e.g. to lie between 0 and 1) beforehand
% if output file exists, simply replace if without warnings
%
% hBlob      : blob object(s). Can be an array of objects
% fout       : output file name. Can use %blob and %id for blob name an id

    % if only a single blob was given, store it into an array first
    if length(blobs) == 1
        blobs(1) = blobs;
    end
    

    % iterate over the blobs
    for i=1:length(blobs)
       
        filePath = strrep(strrep(fout, '%blob', blobs(i).name), '%id', num2str(blobs(i).id));
        
        % if the output path does not exist, create it
        if ~exist(fileparts(filePath), 'dir')   
            mkdir(fileparts(filePath));
        end
        
        % if output file exists, simply replace if without warnings
        if exist(filePath, 'file')
            delete(filePath);
        end  
        
        if blobs(i).dim() == 2
            data = permute(single(blobs(i).data), [3 4 1 2]);
        else
            data = permute(single(blobs(i).data), [2 1 3 4]);
        end
        
        % create hd5 file
        h5create(filePath, ['/' blobs(i).name], size(data), 'Datatype', 'single');

        % write the data
        h5write(filePath, ['/' blobs(i).name], data);
    end
    
end

 