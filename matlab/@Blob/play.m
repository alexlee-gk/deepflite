function play(this, frameRate)
% shows the image sequence
%
% this      : Blob instance of image type. HxWx3xframes
% frameRate : frame rate, default 50

   
    if nargin < 2
        frameRate = 50;
    end
    
    mov = immovie(this.data);


    % play
    implay(mov, frameRate);
