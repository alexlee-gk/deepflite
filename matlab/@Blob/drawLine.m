function drawLine(this, imagePoints, colorChannel)
% coords  : 2x2xN image coordinates. The coordinates are on the first
%           axis, and the points on the second

if nargin < 3
    colorChannel = 1;
end

N = size(this.data,4);

% draw lines
for k=1:N
    rpts = linspace(imagePoints(2,1,k), imagePoints(2,2,k),1000);
    cpts = linspace(imagePoints(1,1,k), imagePoints(1,2,k),1000);
    indexch1 = unique(sub2ind([128, 256, 3, N], min(max(round(rpts),1),128), min(max(round(cpts),1),256), 1*ones(1,1000), k*ones(1,1000)));
    indexch2 = unique(sub2ind([128, 256, 3, N], min(max(round(rpts),1),128), min(max(round(cpts),1),256), 2*ones(1,1000), k*ones(1,1000)));
    indexch3 = unique(sub2ind([128, 256, 3, N], min(max(round(rpts),1),128), min(max(round(cpts),1),256), 3*ones(1,1000), k*ones(1,1000)));
    this.data(indexch1) = 1;
    this.data(indexch2) = 0;
    this.data(indexch3) = 0;    
    
end
