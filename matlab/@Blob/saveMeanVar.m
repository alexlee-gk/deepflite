function saveMeanVar(fout, blobName, mu, sigma2)

    fullpath = fullfile(fileparts(strrep(fout, '%blob', blobName)), 'norm_data.mat');
    save(fullpath, 'mu', 'sigma2');
