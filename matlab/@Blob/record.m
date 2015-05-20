function record(this, frameRate, filename)


    % save movie
    writeObj = VideoWriter(filename);
    writeObj.FrameRate = frameRate;
    open(writeObj);
    writeObj.writeVideo(single(max(min(this.data,1),0)));
    close(writeObj);