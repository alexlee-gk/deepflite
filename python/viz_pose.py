import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from mpl_toolkits.mplot3d import art3d
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('posefiles', type=str, nargs='+')
    args = parser.parse_args()

    Ts = []
    for posefile in args.posefiles:
        with open(posefile) as fp:
            T0_inv = None
            for line in fp:
                T = np.array([float(e) for e in line.split(" ")])
                T = T.reshape((4,4))
                if T0_inv is None:
                    T0_inv = np.linalg.inv(T.copy())
                Ts.append(T0_inv.dot(T))
    Ts = np.array(Ts)

    # plt.ion()
     
    fig = plt.figure('3d plot')
    fig.clear()

    ax = fig.add_subplot(111, projection='3d')
    ax.set_aspect('equal')

    ax.scatter(Ts[:,0,3], Ts[:,1,3], Ts[:,2,3])

    x_axes = []
    y_axes = []
    z_axes = []
    for T in Ts:
        x_axes.append(np.array([T[:3,3], T[:3,3] + T[:3,0] * 10]))
        y_axes.append(np.array([T[:3,3], T[:3,3] + T[:3,1] * 10]))
        z_axes.append(np.array([T[:3,3], T[:3,3] + T[:3,2] * 10]))
    ax.add_collection(art3d.Line3DCollection(x_axes, colors=(1,0,0), lw=1))
    ax.add_collection(art3d.Line3DCollection(y_axes, colors=(0,1,0), lw=1))
    ax.add_collection(art3d.Line3DCollection(z_axes, colors=(0,0,1), lw=1))

    max_pts = Ts[:,:3,3].max(axis=0)
    min_pts = Ts[:,:3,3].min(axis=0)
    max_range = (max_pts - min_pts).max()
    center = 0.5*(max_pts + min_pts)
    ax.set_xlim(center[0] - 0.5*max_range, center[0] + 0.5*max_range)
    ax.set_ylim(center[1] - 0.5*max_range, center[1] + 0.5*max_range)
    ax.set_zlim(center[2] - 0.5*max_range, center[2] + 0.5*max_range)

    x_origin_axes = [np.array([[0,0,0], [100,0,0]])]
    y_origin_axes = [np.array([[0,0,0], [0,100,0]])]
    z_origin_axes = [np.array([[0,0,0], [0,0,100]])]
    ax.add_collection(art3d.Line3DCollection(x_origin_axes, colors=(1,0,0), lw=1))
    ax.add_collection(art3d.Line3DCollection(y_origin_axes, colors=(0,1,0), lw=1))
    ax.add_collection(art3d.Line3DCollection(z_origin_axes, colors=(0,0,1), lw=1))

    plt.show()

if __name__ == '__main__':
    main()