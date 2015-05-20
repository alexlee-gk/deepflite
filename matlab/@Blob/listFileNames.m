function listFileNames(fout, blobNames, trainInd, testInd)
% writes three files, named h5source_train.txt, h5source_test.txt, and
% h5source_single.txt, each of which contains a list of corresponding
% filenames
%
% blobNames    : cell array (or string) of data fields
% trainInd      : indices of the training data files
% testInd       : indices of the test data files



    % if only a single blob name was given, convert it to a cell array
    if ~iscell(blobNames)
        blobNames = {blobNames};
    end

    for j=1:length(blobNames)
        blobName = blobNames{j};
        
        % get the path without the filename
        filePath = fileparts(strrep(fout, '%blob', blobName));

        % print training file names
        fp = fopen(fullfile(filePath, 'h5source_train.txt'),'w');
        for i=1:length(trainInd)
            fprintf(fp, [strrep(strrep(fout, '%blob', blobName), '%id', num2str(trainInd(i))) '\n']);
        end
        fclose(fp);


        % print test file names
        fp = fopen(fullfile(filePath, 'h5source_test.txt'),'w');
        for i=1:length(testInd)
            fprintf(fp, [strrep(strrep(fout, '%blob', blobName), '%id', num2str(testInd(i))) '\n']);
        end
        fclose(fp);
    end
