#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <sys/time.h>
#include <stdlib.h>

#include "boost/program_options.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/thread.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "AprilTags/TagDetector.h"
#include "AprilTags/Tag36h11.h"

#include "utils.h"

using namespace std;
namespace po = boost::program_options;
namespace ptime = boost::posix_time;
namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

map<int, vector<cv::Point3f> > getObjectPointsMap(double tag_size, double box_size) {
	double tag_hsize = tag_size / 2.0;
	double box_hsize = box_size / 2.0;
    map<int, vector<cv::Point3f> > object_points_map = {
    		{0, {cv::Point3d(-tag_hsize, -tag_hsize,  box_hsize),
    			 cv::Point3d( tag_hsize, -tag_hsize,  box_hsize),
    			 cv::Point3d( tag_hsize,  tag_hsize,  box_hsize),
    			 cv::Point3d(-tag_hsize,  tag_hsize,  box_hsize)}},
			{1, {cv::Point3d( tag_hsize,  box_hsize, -tag_hsize),
				 cv::Point3d(-tag_hsize,  box_hsize, -tag_hsize),
				 cv::Point3d(-tag_hsize,  box_hsize,  tag_hsize),
				 cv::Point3d( tag_hsize,  box_hsize,  tag_hsize)}},
			{2, {cv::Point3d(-box_hsize,  tag_hsize, -tag_hsize),
				 cv::Point3d(-box_hsize, -tag_hsize, -tag_hsize),
				 cv::Point3d(-box_hsize, -tag_hsize,  tag_hsize),
				 cv::Point3d(-box_hsize,  tag_hsize,  tag_hsize)}},
			{3, {cv::Point3d(-tag_hsize, -box_hsize, -tag_hsize),
				 cv::Point3d( tag_hsize, -box_hsize, -tag_hsize),
				 cv::Point3d( tag_hsize, -box_hsize,  tag_hsize),
				 cv::Point3d(-tag_hsize, -box_hsize,  tag_hsize)}},
			{4, {cv::Point3d( box_hsize, -tag_hsize, -tag_hsize),
				 cv::Point3d( box_hsize,  tag_hsize, -tag_hsize),
				 cv::Point3d( box_hsize,  tag_hsize,  tag_hsize),
				 cv::Point3d( box_hsize, -tag_hsize,  tag_hsize)}}
    };
    return object_points_map;
}

Eigen::Matrix4d getTransform(const vector<cv::Point3f>& object_points, const vector<cv::Point2f>& image_points, Eigen::Matrix3d camera_matrix, vector<double> dist_coeffs) {
	cv::Mat rvec, tvec;
	cv::Matx33f cameraMatrix(
						     camera_matrix(0,0), camera_matrix(0,1), camera_matrix(0,2),
						     camera_matrix(1,0), camera_matrix(1,1), camera_matrix(1,2),
						     camera_matrix(2,0), camera_matrix(2,1), camera_matrix(2,2));
	cv::Vec4f distParam(dist_coeffs[0], dist_coeffs[1], dist_coeffs[2], dist_coeffs[3]);
	cv::solvePnP(object_points, image_points, cameraMatrix, distParam, rvec, tvec);
	cv::Matx33d r;
	cv::Rodrigues(rvec, r);
	Eigen::Matrix3d wRo;
	wRo << r(0,0), r(0,1), r(0,2), r(1,0), r(1,1), r(1,2), r(2,0), r(2,1), r(2,2);

	Eigen::Matrix4d T;
	T.topLeftCorner(3,3) = wRo;
	T.col(3).head(3) << tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2);
	T.row(3) << 0,0,0,1;

	return T;
}

Eigen::Matrix4d getBoxTransform(const vector<AprilTags::TagDetection>& detections, const map<int, vector<cv::Point3f> >& object_points_map, Eigen::Matrix3d camera_matrix, vector<double> dist_coeffs) {
	vector<cv::Point3f> object_points;
	vector<cv::Point2f> image_points;
	for (auto detection : detections) {
		if (object_points_map.find(detection.id) == object_points_map.end()) {
			continue;
		}
		const vector<cv::Point3f>& detection_object_points = object_points_map.at(detection.id);
		object_points.insert(object_points.end(), detection_object_points.begin(), detection_object_points.end());
		image_points.push_back(cv::Point2f(detection.p[0].first, detection.p[0].second));
		image_points.push_back(cv::Point2f(detection.p[1].first, detection.p[1].second));
		image_points.push_back(cv::Point2f(detection.p[2].first, detection.p[2].second));
		image_points.push_back(cv::Point2f(detection.p[3].first, detection.p[3].second));
	}
	return getTransform(object_points, image_points, camera_matrix, dist_coeffs);
}

void drawPose(const Eigen::Matrix4d& T, cv::Mat& image, Eigen::Matrix3d camera_matrix, double axis_length=100) {
	Eigen::MatrixXd camera_matrix_homogeneous(3,4);
	camera_matrix_homogeneous.leftCols(3) = camera_matrix;
	camera_matrix_homogeneous.col(3) = Eigen::Vector3d::Zero();
	vector<Eigen::Vector3d> axes_object_points = {Eigen::Vector3d::Zero(), axis_length * Eigen::Vector3d::UnitX(), axis_length * Eigen::Vector3d::UnitY(), axis_length * Eigen::Vector3d::UnitZ()};
	vector<cv::Point2f> axes_image_points;
	for (auto& object_point : axes_object_points) {
		Eigen::Vector4d object_points_h;
		object_points_h.topRows(3) = object_point;
		object_points_h(3) = 1;
		Eigen::Vector3d image_point_h = camera_matrix_homogeneous * T * object_points_h;
		image_point_h /= image_point_h(2);
		axes_image_points.push_back(cv::Point2f(image_point_h(0), image_point_h(1)));
	}
	cv::line(image, axes_image_points[0], axes_image_points[1], cv::Scalar(0,0,255,0), 5);
	cv::line(image, axes_image_points[0], axes_image_points[2], cv::Scalar(0,255,0,0), 5);
	cv::line(image, axes_image_points[0], axes_image_points[3], cv::Scalar(255,0,0,0), 5);
}

vector<double> string_to_vector(string s) {
	stringstream ss(s);
	string element;
    vector<double> vec;
	while (ss >> element) {
		vec.push_back(atof(element.c_str()));
	}
	return vec;
}

int main(int argc, char* argv[]) {
	int exposure;
	int gain;
	int brightness;
	string input;
	string calib_file;
	double tag_size;
	double box_size;
	po::options_description desc("Allowed options");
	desc.add_options()
				("help", "produce help message")
				("exposure", po::value<int>(&exposure)->default_value(-1))
				("gain", po::value<int>(&gain)->default_value(-1))
				("brightness", po::value<int>(&brightness)->default_value(-1))
				("input,i", po::value<string>(&input), "base name for input (and output) files")
				("calib-file,c", po::value<string>(&calib_file), "intrinsic calibration file")
				("tag-size", po::value<double>(&tag_size)->default_value(120))
				("box-size", po::value<double>(&box_size)->default_value(150))
				;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return 1;
	}

	pt::ptree tree;
	pt::read_json(input + "info.json", tree);

	vector<string> camera_ids;
	vector<string> video_filenames;
	vector<vector<string> > timestamps;
	for (pt::ptree::value_type &v : tree.get_child("cameras")) {
		string camera_id = v.first;
		camera_ids.push_back(camera_id);
		pt::ptree tcamera = (pt::ptree) v.second;
		video_filenames.push_back(tcamera.get<string>("video.filename"));
		vector<string> timestamp;
		for (pt::ptree::value_type &v : tcamera.get_child("video.timestamps")) {
			timestamp.push_back(v.second.get_value<string>());
		}
		timestamps.push_back(timestamp);
	}

	map<string, pair<Eigen::Matrix3d, vector<double> > > calib_map;
	pt::read_json(calib_file, tree);
	for (pt::ptree::value_type &v : tree.get_child("cameras")) {
		string camera_id = v.first;
		pt::ptree tcamera = (pt::ptree) v.second;
		Eigen::Matrix3d camera_matrix = Eigen::Map<Eigen::Matrix<double,3,3,Eigen::RowMajor> >(string_to_vector(tcamera.get<string>("camera_matrix")).data());
		vector<double> dist_coeffs = string_to_vector(tcamera.get<string>("dist_coeffs"));
		calib_map[camera_id] = pair<Eigen::Matrix3d, vector<double> >(camera_matrix, dist_coeffs);
	}

	const int n_cameras = camera_ids.size();

    map<int, vector<cv::Point3f> > object_points_map = getObjectPointsMap(tag_size, box_size);

	AprilTags::TagDetector tag_detector(AprilTags::tagCodes36h11);
	vector<AprilTags::TagDetection> detections;
	Eigen::IOFormat matrix_format(Eigen::StreamPrecision, Eigen::DontAlignCols, " ", " ");
	bool detect = true;
	bool done = false;
	char key = (char) 0;
	for (int i = 0; i < n_cameras; i++) {
		cv::VideoCapture capture(fs::path(input).parent_path().string() + "/" + video_filenames[i]);
		if (calib_map.find(camera_ids[i]) == calib_map.end()) {
			cout << "Error: calibration for camera " << camera_ids[i] << " is missing" << endl;
		}
		Eigen::Matrix3d camera_matrix = calib_map[camera_ids[i]].first;
		vector<double> dist_coeffs = calib_map[camera_ids[i]].second;
		cv::Mat image, image_gray;
	    string filename = input + "pose" + camera_ids[i] + ".txt";
	    ofstream fp_pose;
	    fp_pose.open(filename.c_str());
		while (capture.isOpened()) {
			capture >> image;
			if (image.rows == 0 || image.cols == 0) {
				break;
			}

			// detect April tags (requires a gray scale image)
			if (detect) {
				cv::cvtColor(image, image_gray, CV_BGR2GRAY);
				detections = tag_detector.extractTags(image_gray);

				if (detections.size()) {
					auto T = getBoxTransform(detections, object_points_map, camera_matrix, dist_coeffs);
					fp_pose << T.format(matrix_format) << endl;
					drawPose(T, image, camera_matrix);
				}
				for (auto &detection : detections) {
					detection.draw(image);
				}
			}
			key = (char) cv::waitKey(1);
			cv::imshow("image", image);
			if (key == 'd') {
				detect = !detect;
			}
			if (key == 'n') {
				break;
			}
			if (key == 'q' || key == 27) {
				done = true;
				break;
			}
		}
		fp_pose.close();
		capture.release();
		if (done) {
			break;
		}
	}
	return 0;
}
