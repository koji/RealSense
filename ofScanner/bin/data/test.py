from open3d import *

if __name__=="__main__":
    pcd = read_point_cloud('test.pcd')
    draw_geometries([pcd])
