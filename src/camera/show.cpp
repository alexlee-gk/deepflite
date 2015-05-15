#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <sys/time.h>

#include "boost/program_options.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/thread.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <libv4l2.h>
#include <linux/videodev2.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "AprilTags/TagDetector.h"
#include "AprilTags/Tag36h11.h"

using namespace std;
namespace po = boost::program_options;
namespace ptime = boost::posix_time;
namespace pt = boost::property_tree;

void setupCameraSettings(int device_id, int exposure, int gain, int brightness) {
	// manually setting camera exposure settings; OpenCV/v4l1 doesn't
	// support exposure control; so here we manually use v4l2 before
	// opening the device via OpenCV; confirmed to work with Logitech
	// C270; try exposure=20, gain=100, brightness=150

	string video_str = "/dev/video" + boost::lexical_cast<string>(device_id);

	int device = v4l2_open(video_str.c_str(), O_RDWR | O_NONBLOCK);

	if (exposure >= 0) {
		// not sure why, but v4l2_set_control() does not work for
		// V4L2_CID_EXPOSURE_AUTO...
		struct v4l2_control c;
		c.id = V4L2_CID_EXPOSURE_AUTO;
		c.value = 1; // 1=manual, 3=auto; V4L2_EXPOSURE_AUTO fails...
		if (v4l2_ioctl(device, VIDIOC_S_CTRL, &c) != 0) {
			cout << "Failed to set... " << strerror(errno) << endl;
		}
		cout << "exposure: " << exposure << endl;
		v4l2_set_control(device, V4L2_CID_EXPOSURE_ABSOLUTE, exposure*6);
	}
	if (gain >= 0) {
		cout << "gain: " << gain << endl;
		v4l2_set_control(device, V4L2_CID_GAIN, gain*256);
	}
	if (brightness >= 0) {
		cout << "brightness: " << brightness << endl;
		v4l2_set_control(device, V4L2_CID_BRIGHTNESS, brightness*256);
	}
	v4l2_close(device);
}

void captureAndWrite(bool& done, int i, cv::Mat& frame, cv::VideoCapture* capture, cv::VideoWriter* writter, vector<string>& timestamps) {
	int i_frame = 0;
	ptime::ptime last_t, t;
	last_t = ptime::microsec_clock::local_time();
	while (!done) {
		(*capture) >> frame;
		string timestamp = to_iso_extended_string(ptime::microsec_clock::local_time());
		if (writter) {
			(*writter) << frame;
		}
		timestamps.push_back(timestamp);
		// print out the frame rate at which image frames are being processed
		i_frame++;
		if (i_frame % 10 == 0) {
			t = ptime::microsec_clock::local_time();
			if (i == 0)
				cout << "  " << 1000. * 10./(t-last_t).total_milliseconds() << " fps" << endl;
			last_t = t;
		}
	}
}

int main(int argc, char* argv[]) {
	vector<int> device_ids;
	int width;
	int height;
	double fps;
	int exposure;
	int gain;
	int brightness;
	string output;
	po::options_description desc("Allowed options");
	desc.add_options()
				("help", "produce help message")
				("ids", po::value< vector<int> >(&device_ids))
				("width,w", po::value<int>(&width)->default_value(1280))
				("height,h", po::value<int>(&height)->default_value(720))
				("fps", po::value<double>(&fps)->default_value(30.0))
				("exposure", po::value<int>(&exposure)->default_value(-1))
				("gain", po::value<int>(&gain)->default_value(-1))
				("brightness", po::value<int>(&brightness)->default_value(-1))
				("output,o", po::value<string>(&output), "base name for output files")
				;
	po::positional_options_description p;
	p.add("ids", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return 1;
	}

	// setup captures and writters
	vector<cv::VideoCapture> captures;
	vector<cv::VideoWriter> writters;
	vector<string> video_filenames;
	for (int i = 0; i < device_ids.size(); i++) {
		setupCameraSettings(device_ids[i], exposure, gain, brightness);
		cv::VideoCapture capture(device_ids[i]);
		if (!capture.isOpened()) {
			cout << "Cannot open video capture " << device_ids[i] << endl;
			return 1;
		}
		capture.set(CV_CAP_PROP_FRAME_WIDTH, width);
		capture.set(CV_CAP_PROP_FRAME_HEIGHT, height);
		capture.set(CV_CAP_PROP_FPS, fps);
		captures.push_back(capture);

		if (output.size()) {
			string filename = output + "video" + boost::lexical_cast<string>(device_ids[i]) + ".avi";
			cv::VideoWriter writter(filename, CV_FOURCC('M', 'J', 'P', 'G'), fps, cv::Size(width, height), true);
			if (!writter.isOpened()) {
				cout << "Cannot open video writter" << endl;
				return 1;
			}
			writters.push_back(writter);

			video_filenames.push_back(filename);
		}
	}

	// start capture and write threads for each camera
	bool done = false;
	vector<cv::Mat> frames;
	vector<vector<string> > timestamps;
	// reserve space so that reference doesn't change
	frames.reserve(captures.size());
	timestamps.reserve(captures.size());
	boost::thread_group thread_group;
	for (int i = 0; i < captures.size(); i++) {
		frames.push_back(cv::Mat(height, width, CV_8UC3));
		timestamps.push_back(vector<string>());
		thread_group.add_thread(new boost::thread(captureAndWrite, boost::ref(done), i, boost::ref(frames[i]), &captures[i], (writters.size() > i) ? &writters[i] : NULL, boost::ref(timestamps[i])));
	}

	// main loop for visualization and interactive input
	AprilTags::TagDetector tag_detector(AprilTags::tagCodes36h11);
	cv::Mat image, image_gray;
	int grid_width = (frames.size() < 3) ? 1 : 2;
	int grid_height = (frames.size() < 2) ? 1 : 2;
	cv::Mat all_image(grid_height*height, grid_width*width, CV_8UC3);
	cv::namedWindow("image", cv::WINDOW_NORMAL);
	int max_width = width;
	int max_height = height;
	double scale = min(((double) max_width)/(width*grid_width), ((double) max_height)/(height*grid_height));
	cv::resizeWindow("image", scale*all_image.cols, scale*all_image.rows);
	vector<AprilTags::TagDetection> detections;
	vector<string> image_timestamps;
	bool detect = false;
	char key = (char) 0;
	while (true) {
		for (int i = 0; i < frames.size(); i++) {
			frames[i].copyTo(image);
			if (detect) {
				// detect April tags (requires a gray scale image)
				cv::cvtColor(image, image_gray, CV_BGR2GRAY);
				detections = tag_detector.extractTags(image_gray);

				// show the current image including any detections
				BOOST_FOREACH(const AprilTags::TagDetection &detection, detections) {
					detection.draw(image);
				}
			}
			image.copyTo(all_image(cv::Rect((i%grid_width) * width, (i/grid_width) * height, width, height)));
		}
		key = (char) cv::waitKey(1);
		cv::imshow("image", all_image);
		if (key == 'c') {
			image_timestamps.push_back(to_iso_extended_string(ptime::microsec_clock::local_time()));
		}
		if (key == 'd') {
			detect = !detect;
		}
		if (key == 'q') {
			done = true;
			break;
		}
	}
	thread_group.join_all();

	// save metadata
	if (output.size()) {
		pt::ptree tree;
		pt::ptree tvideos;
		for (int i = 0; i < device_ids.size(); i++) {
			pt::ptree tvideo;
			tvideo.put("filename", video_filenames[i]);
			pt::ptree ttimestamps;
			BOOST_FOREACH(string &timestamp, timestamps[i]) {
				pt::ptree ttimestamp;
				ttimestamp.put("", timestamp);
				ttimestamps.push_back(make_pair("", ttimestamp));
			}
			tvideo.add_child("timestamps", ttimestamps);
			tvideos.push_back(make_pair("", tvideo));
		}
		tree.add_child("cameras.videos", tvideos);

		pt::ptree ttimestamps;
		BOOST_FOREACH(string &timestamp, image_timestamps) {
			pt::ptree ttimestamp;
			ttimestamp.put("", timestamp);
			ttimestamps.push_back(make_pair("", ttimestamp));
		}
		tree.add_child("cameras.image_timestamps", ttimestamps);

		pt::write_json(output + "info.json", tree);
	}

	return 0;
}
