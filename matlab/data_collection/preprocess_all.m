datapath = '/media/data__/tuomas/NIPS2015';
datafile = fullfile(datapath, 'flashlog/15-05-18_19-06-34.bin-48147.mat');
blob_path = fullfile(datapath, '%blob/blob%id.h5');



%% PREPROCESS THE flashlog DATA
% get the blobs
ind0 = 0;
batch_size = 1000;
blobs = preprocess_flashlog(datafile,ind0,batch_size);




% data and EKF blobs
blobnames = {'data', 'EKF'};
% blobnames = {'IMU', 'mag', 'baro', 'input', 'GPS', 'EKF'};
for j=1:length(blobnames)
    
    % find the mean and variance
    [mean, var] = Blob.meanVar(blobs.(blobnames{j}));
    blobs.([blobnames{j} '_norm']) = Blob.normalize(blobs.(blobnames{j}), mean, var);
    
    % save the original and normalized blobs
    Blob.saveH5(blob_path, blobs.(blobnames{j}));
	Blob.saveH5(blob_path, blobs.([blobnames{j} '_norm']));
    
    % save the mean and variance
    Blob.saveMeanVar(blob_path, ([blobnames{j} '_norm']), mean, var);
end

%% PREPROCESS VIDEO




%% generate clip and list source files for caffe

clip = Blob('clip', 1);
clip.data = [0 ones(1, batch_size-1)];
Blob.saveH5(blob_path, clip);
Blob.listFileNames(blob_path, {'clip'}, 1, 1);

%%

Blob.listFileNames(blob_path, {'data_norm', 'EKF_norm'}, 1:3, 4);

