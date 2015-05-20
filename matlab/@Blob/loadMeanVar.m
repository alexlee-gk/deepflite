function [mu, sigma2] = loadMeanVar(fout, blobName)

    fullpath = fullfile(fileparts(strrep(fout, '%blob', blobName)), 'norm_data.mat');
    normData = load(fullpath);
    mu = normData.mu;
    sigma2 = normData.sigma2;
  