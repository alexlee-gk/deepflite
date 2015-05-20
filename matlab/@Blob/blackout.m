function out = blackout(this)
% replace random images with noise



dim = size(this.data);
blackout = [];
while length(blackout) < dim(4);
    blackout = [blackout zeros(1, floor(rand(1)*200)) ones(1,floor(rand(1)*50))];
end

blackout = blackout(1:dim(4));

out = Blob([this.name '_blackout'], this.id);
out.data = this.data;

out.data(:,:,:,logical(blackout)) = rand(dim(1), dim(2), dim(3), sum(blackout));

