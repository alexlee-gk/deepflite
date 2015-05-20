function out = preprocess_flashlog(filename, ind0, batch_size)
% loads the flashlog given as an mat file and stores the data into blobs
% The data is resampled with a constant sampling time 20ms by using ZOH
%
% filename : mat file containing all the onboard data
% ind0     : ind0+1 is the index of the first output blob
%
% out      : check the end of this file

data = load(filename);


time_ind = 2; % onboard time
t0 = data.RCOU(100,time_ind);% time of takeoff just eyeball this
tend = data.RCOU(end, time_ind);

%% augment the GPS data with the onboard time. Use linenumber to sync
% first make sure all the GPS linenumbers are between the min and max line
% number of IMU
data.GPS = data.GPS(data.GPS(:,1)>=data.IMU(1,1) & data.GPS(:,1)<=data.IMU(end,1),:);
GPS_timestamps = interp1(data.IMU(:,1), data.IMU(:,2), data.GPS(:,1));
data.GPS_label = {data.GPS_label{1}, 'TimeMS', data.GPS_label{2:end}}';
data.GPS = [data.GPS(:,1) GPS_timestamps data.GPS(:,2:end)];

%% There might be duplicate EKF timestamps. Get rid of those
[~, I] = unique(data.EKF1(:,time_ind));
data.EKF1 = data.EKF1(I,:);


% %% plot relative altitude to check the timestamps were set correctly
% plot(GPS(:,time_ind), GPS(:,10));
% hold on
% plot(EKF1(:,time_ind), -EKF1(:,11));

%% get the data indeces
% helper function to find the right data indeces
fun_find_data_labels =  @(data_labels, search_labels) arrayfun(@(label)find(strcmp(data_labels, label)), search_labels);

IMU_ind = fun_find_data_labels(data.IMU_label, {'AccX','AccY','AccZ', 'GyrX','GyrY','GyrZ'});
IMU2_ind = fun_find_data_labels(data.IMU2_label, {'AccX','AccY','AccZ', 'GyrX','GyrY','GyrZ'});
MAG_ind = fun_find_data_labels(data.MAG_label, {'MagX','MagY','MagZ'});
MAG2_ind = fun_find_data_labels(data.MAG2_label, {'MagX','MagY','MagZ'});
BARO_ind = fun_find_data_labels(data.BARO_label, {'Press'});
% we should use only the raw pressure reading
% BARO_alt = find_data_label(BARO_label, {'Alt'}); 
RCOU_ind  = fun_find_data_labels(data.RCOU_label, {'Chan1','Chan2','Chan3','Chan4'});
GPS_ind = fun_find_data_labels(data.GPS_label, {'Lat','Lng','Alt','Spd', 'VZ'});


EKF_att_ind = fun_find_data_labels(data.EKF1_label, {'Roll','Pitch','Yaw'});
EKF_vel_ind = fun_find_data_labels(data.EKF1_label, {'VN','VE','VD'});
EKF_pos_ind = fun_find_data_labels(data.EKF1_label, {'PN','PE','PD'});
EKF_ind = [EKF_att_ind EKF_vel_ind EKF_pos_ind];

%% organize the data into blobs

% assume the state estimator is run at a constant rate using the most
% recent measurement as an input
dt = 20; % milliseconds
t = t0:dt:tend; % new sampling times


% function for resampling data. 
fun_resample = @(data, data_inds, t)cell2mat(arrayfun(@(data_ind)interp1(data(:, time_ind), data(:, data_ind), t, 'previous')', data_inds, 'UniformOutput', false));

% Resample, Use the latest available measurement
IMU_resampled = fun_resample(data.IMU, IMU_ind, t);
IMU2_resampled = fun_resample(data.IMU2, IMU2_ind, t);
MAG_resampled = fun_resample(data.MAG, MAG_ind, t);
MAG2_resampled = fun_resample(data.MAG2, MAG2_ind, t);
BARO_resampled = fun_resample(data.BARO, BARO_ind, t);
RCOU_resampled = fun_resample(data.RCOU, RCOU_ind, t);
GPS_resampled = fun_resample(data.GPS, GPS_ind, t);
EKF_resampled = fun_resample(data.EKF1, EKF_ind, t);

% figure
% plot(IMU(:, time_ind), IMU(:,IMU_acc_ind(1)),'r');
% hold on
% plot(t0:dt:tend, imu_resampled,'b');

% TODO: do we want have the first batch start when we take off or the last
% batch end when we land?
Nbatches = floor(length(t)/batch_size);
IMU_blob(Nbatches,1) = Blob();
MAG_blob(Nbatches,1) = Blob();
BARO_blob(Nbatches,1) = Blob();
INPUT_blob(Nbatches,1) = Blob();
GPS_blob(Nbatches,1) = Blob();
EKF_blob(Nbatches,1) = Blob();

% this blob contains all the input blobs
data_blob(Nbatches,1) = Blob();

% create blobs
for i=1:Nbatches    
    data_ind = (i-1)*batch_size+1:i*batch_size;
    IMU_blob(i) = Blob('IMU', ind0+i);
    IMU_blob(i).data = (IMU_resampled(data_ind,:)' + IMU2_resampled(data_ind,:)')/2;
    
    MAG_blob(i) = Blob('mag', ind0+i);
    MAG_blob(i).data = (MAG_resampled(data_ind,:)' + MAG2_resampled(data_ind,:)')/2;
    
    BARO_blob(i) = Blob('baro', ind0+i);
    BARO_blob(i).data = BARO_resampled(data_ind,:)';
    
    INPUT_blob(i) = Blob('input', ind0+i);
    INPUT_blob(i).data = RCOU_resampled(data_ind,:)';
    
    GPS_blob(i) = Blob('GPS', ind0+i); 
    GPS_blob(i).data = GPS_resampled(data_ind,:)';
    
    EKF_blob(i) = Blob('EKF', ind0+i);
    EKF_blob(i).data = EKF_resampled(data_ind, :)'; 

    data_blob(i) = Blob('data', ind0+i);
    data_blob(i).data = [IMU_blob(i).data; MAG_blob(i).data; BARO_blob(i).data; INPUT_blob(i).data];
end

out.IMU = IMU_blob;
out.mag = MAG_blob;
out.baro = BARO_blob;
out.input = INPUT_blob;
out.GPS = GPS_blob;
out.EKF = EKF_blob;
out.data = data_blob;

out.IMU_labels = data.IMU_label(IMU_ind);
out.mag_labels = data.MAG_label(MAG_ind);
out.baro_labels = data.BARO_label(BARO_ind);
out.input_labels = data.RCOU_label(RCOU_ind);
out.GPS_labels = data.GPS_label(GPS_ind);
out.EKF_labels = data.EKF1_label(EKF_ind);

out.data_labels = [out.IMU_labels; out.mag_labels; out.baro_labels; out.input_labels];



