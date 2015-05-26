#include <iostream>
#include <vector>
#include <string>

#include "boost/program_options.hpp"
#include "boost/thread.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <Eigen/Dense>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/eigen.hpp"

#include "AprilTags/TagDetector.h"
#include "AprilTags/Tag36h11.h"

#include "MavlinkInterface.h"

#include "utils.h"

using namespace std;
namespace po = boost::program_options;
namespace ptime = boost::posix_time;
namespace pt = boost::property_tree;

map<int, vector<cv::Point3f> > calcObjectPointsMap(const Eigen::MatrixXi& tag_ids, double tag_size, double tag_space) {
	map<int, vector<cv::Point3f> > object_points_map;
    for (int i = 0; i < tag_ids.rows(); i++) {
    	for (int j = 0; j < tag_ids.cols(); j++) {
    		vector<cv::Point3f> object_points;
			object_points.push_back(cv::Point3f(float(j*tag_space)           , float(i*tag_space + tag_size), 0));
			object_points.push_back(cv::Point3f(float(j*tag_space + tag_size), float(i*tag_space + tag_size), 0));
			object_points.push_back(cv::Point3f(float(j*tag_space + tag_size), float(i*tag_space)           , 0));
    		object_points.push_back(cv::Point3f(float(j*tag_space)           , float(i*tag_space)           , 0));
			object_points_map[tag_ids(i,j)] = object_points;
		}
	}
    return object_points_map;
}

int addObjectAndImagePoints(const vector<AprilTags::TagDetection>& detections, const map<int, vector<cv::Point3f> >& object_points_map, vector<cv::Point3f>& object_points, vector<cv::Point2f>& image_points) {
	int num_points = 0;
    for (int i = 0; i < detections.size(); i++) {
    	if (object_points_map.find(detections[i].id) == object_points_map.end()) { // detected tag id is not in object_points_map
    		continue;
    	}
    	const vector<cv::Point3f>& detection_object_points = object_points_map.at(detections[i].id);
    	object_points.insert(object_points.end(), detection_object_points.begin(), detection_object_points.end());
		image_points.push_back(cv::Point2f(detections[i].p[0].first, detections[i].p[0].second));
		image_points.push_back(cv::Point2f(detections[i].p[1].first, detections[i].p[1].second));
		image_points.push_back(cv::Point2f(detections[i].p[2].first, detections[i].p[2].second));
		image_points.push_back(cv::Point2f(detections[i].p[3].first, detections[i].p[3].second));
		num_points += 4;
    }
    return num_points;
}

int main(int argc, char* argv[]) {
	string id;
	int width;
	int height;
	double fps;
	int exposure;
	int gain;
	int brightness;
	string output;
	double tag_size;
	double tag_space;
	po::options_description desc("Allowed options");
	desc.add_options()
				("help", "produce help message")
				("id", po::value<string>(&id), "device or camera id or camera serial numbers")
				("width,w", po::value<int>(&width)->default_value(1280))
				("height,h", po::value<int>(&height)->default_value(720))
				("fps", po::value<double>(&fps)->default_value(30.0))
				("exposure", po::value<int>(&exposure)->default_value(-1))
				("gain", po::value<int>(&gain)->default_value(-1))
				("brightness", po::value<int>(&brightness)->default_value(-1))
				("output,o", po::value<string>(&output), "base name for output files")
				("tag-size", po::value<double>(&tag_size)->default_value(25.4 * 13.0/16.0))
				("tag-space", po::value<double>(&tag_space)->default_value(25.4 * 1.0))
				;
	po::positional_options_description p;
	p.add("id", 1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return 1;
	}

	int device_id = getDeviceId(id);
	string camera_id = getCameraIdFromSerialNumber(getCameraSerialNumberFromDeviceId(device_id));

	setupCameraSettings(device_id, exposure, gain, brightness);
	cv::VideoCapture capture(device_id);
	if (!capture.isOpened()) {
		cout << "Cannot open video capture " << device_id << endl;
		return 1;
	}
	capture.set(CV_CAP_PROP_FRAME_WIDTH, width);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, height);
	capture.set(CV_CAP_PROP_FPS, fps);

    Eigen::MatrixXi tag_ids(8,10);
    for (int i = 0; i < tag_ids.rows(); i++) {
    	for (int j = 0; j < tag_ids.cols(); j++) {
    		tag_ids(i,j) = 14 + i*24 + j;
    	}
    }
	map<int, vector<cv::Point3f> > object_points_map = calcObjectPointsMap(tag_ids, tag_size, tag_space);
    vector<vector<cv::Point3f> > object_points;
    vector<vector<cv::Point2f> > image_points;

	cv::Mat image, image_gray;
	cv::namedWindow("image", cv::WINDOW_NORMAL);
	cv::resizeWindow("image", width, height);
	AprilTags::TagDetector tag_detector(AprilTags::tagCodes36h11);
	vector<AprilTags::TagDetection> detections;
	bool detect = false;
	bool calib_capture = false;
	char key = (char) 0;
    while (true) {
    	capture >> image;

		if (detect || calib_capture) {
			// detect April tags (requires a gray scale image)
			cv::cvtColor(image, image_gray, CV_BGR2GRAY);
			detections = tag_detector.extractTags(image_gray);

			if (calib_capture) {
				object_points.push_back(vector<cv::Point3f>());
				image_points.push_back(vector<cv::Point2f>());
				int num_points = addObjectAndImagePoints(detections, object_points_map, object_points.back(), image_points.back());
				cout << "added " << num_points << " points" << endl;
			}

			// show the current image including any detections
			for (auto &detection : detections) {
				detection.draw(image);
			}
		}
		key = (char) cv::waitKey(1);
    	cv::flip(image, image, 1);
		cv::imshow("image", image);
		calib_capture = key == 'c';
		if (key == 'd') {
			detect = !detect;
		}
		if (key == 'q' || key == 27) {
			break;
		}
    }
    cout << "calibrating" << endl;

    cv::Matx33d camera_matrix;
    cv::Vec<double, 5> dist_coeffs;
    vector<cv::Mat> rvecs;
    vector<cv::Mat> tvecs;
    cv::calibrateCamera(object_points, image_points, image.size(), camera_matrix, dist_coeffs, rvecs, tvecs);


    Eigen::Matrix3d camera_matrix_eigen;
	Eigen::Matrix<double, 5, 1> dist_coeffs_eigen;
	cv::cv2eigen(camera_matrix, camera_matrix_eigen);
	cv::cv2eigen(dist_coeffs, dist_coeffs_eigen);
    cout << "camera_matrix: " << endl << camera_matrix_eigen << endl;
    cout << "dist_coeffs: " << dist_coeffs_eigen.transpose() << endl;

    while (true) {
        capture >> image;

        cv::Mat rectified_image;
        cv::undistort(image, rectified_image, camera_matrix, dist_coeffs);

		key = (char) cv::waitKey(1);
    	cv::flip(rectified_image, rectified_image, 1);
		imshow("image", rectified_image);
		if (key == 'q' || key == 27) {
			break;
		}
	}

	// save intrinsics
	if (output.size()) {
		Eigen::IOFormat matrix_format(Eigen::StreamPrecision, Eigen::DontAlignCols, " ", " ");
		pt::ptree tree;
		pt::ptree tcamera;
		tcamera.put("serial_number", getCameraSerialNumberFromId(camera_id));
		tcamera.put("camera_matrix", camera_matrix_eigen.format(matrix_format));
		tcamera.put("dist_coeffs", dist_coeffs_eigen.format(matrix_format));
		tree.add_child("cameras." + camera_id, tcamera);
//		pt::write_json(cout, tree);
		pt::write_json(output + "intrinsic_calib.json", tree);
	}

	return 0;
}
