#include <iostream>
#include <vector>
#include <string>

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

#include "MavlinkInterface.h"

#include "utils.h"

using namespace std;
namespace po = boost::program_options;
namespace ptime = boost::posix_time;
namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

void captureAndWrite(bool& done, int i, cv::Mat& frame, cv::VideoCapture* capture, cv::VideoWriter* writter, vector<long>& timestamps) {
	int i_frame = 0;
	ptime::ptime last_t, t;
	last_t = ptime::microsec_clock::local_time();
	while (!done) {
		(*capture) >> frame;
        long timestamp = get_timestamp();

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
	vector<string> ids;
	int width;
	int height;
	double fps;
	int exposure;
	int gain;
	int brightness;
	string output;
	bool telemetry;
	po::options_description desc("Allowed options");
	desc.add_options()
				("help", "produce help message")
				("ids", po::value<vector<string> >(&ids), "device or camera ids or camera serial numbers")
				("width,w", po::value<int>(&width)->default_value(1280))
				("height,h", po::value<int>(&height)->default_value(720))
				("fps", po::value<double>(&fps)->default_value(30.0))
				("exposure", po::value<int>(&exposure)->default_value(-1))
				("gain", po::value<int>(&gain)->default_value(-1))
				("brightness", po::value<int>(&brightness)->default_value(-1))
				("output,o", po::value<string>(&output), "base name for output files")
				("telemetry,t", po::value<bool>(&telemetry)->default_value(true), "Use telemetry")
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

	const int n_cameras = ids.size();
	vector<int> device_ids;
	vector<string> camera_ids;
	for (auto id : ids) {
		int device_id = getDeviceId(id);
		device_ids.push_back(device_id);
		camera_ids.push_back(getCameraIdFromSerialNumber(getCameraSerialNumberFromDeviceId(device_id)));
	}

	// setup captures and writters
	vector<cv::VideoCapture> captures;
	vector<cv::VideoWriter> writters;
	vector<string> video_filenames;
	string file_prefix = output + to_iso_string(ptime::second_clock::local_time());
	for (int i = 0; i < n_cameras; i++) {
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
			string filename = file_prefix + "_video" + camera_ids[i] + ".avi";
			cv::VideoWriter writter(filename, CV_FOURCC('M', 'J', 'P', 'G'), fps, cv::Size(width, height), true);
			if (!writter.isOpened()) {
				cout << "Cannot open video writter" << endl;
				return 1;
			}
			writters.push_back(writter);

			video_filenames.push_back(fs::path(filename).filename().string());
		}
	}

	// start capture and write threads for each camera
	bool done = false;
	vector<cv::Mat> frames;
	vector<vector<long> > timestamps;

	// reserve space so that reference doesn't change
	frames.reserve(n_cameras);
	timestamps.reserve(n_cameras);
	boost::thread_group thread_group;
	for (int i = 0; i < n_cameras; i++) {
		frames.push_back(cv::Mat(height, width, CV_8UC3));
		timestamps.push_back(vector<long>());
		thread_group.add_thread(new boost::thread(captureAndWrite, boost::ref(done), i, boost::ref(frames[i]), &captures[i], (writters.size() > i) ? &writters[i] : NULL, boost::ref(timestamps[i])));
	}

    // start capturing telemetry data
    MavlinkInterface mav;
    if (telemetry) {
		int baudrate = 57600;
		mav.start("/dev/ttyUSB0", baudrate, file_prefix);
		mav.request_data_stream(MAV_DATA_STREAM_ALL, 0, 0);
		mav.request_data_stream(MAV_DATA_STREAM_RAW_SENSORS, 50, 1);
    }

	// main loop for visualization and interactive input
	cv::Mat image, image_gray;
	int grid_width = (n_cameras < 3) ? 1 : 2;
	int grid_height = (n_cameras < 2) ? 1 : 2;
	cv::Mat all_image(grid_height*height, grid_width*width, CV_8UC3);
	cv::namedWindow("image", cv::WINDOW_NORMAL);
	int max_width = width;
	int max_height = height;
	double scale = min(((double) max_width)/(width*grid_width), ((double) max_height)/(height*grid_height));
	cv::resizeWindow("image", scale*all_image.cols, scale*all_image.rows);
	AprilTags::TagDetector tag_detector(AprilTags::tagCodes36h11);
	vector<AprilTags::TagDetection> detections;
	vector<long> image_timestamps;
	bool detect = false;
	char key = (char) 0;
	while (true) {
		for (int i = 0; i < n_cameras; i++) {
			frames[i].copyTo(image);
			if (detect) {
				// detect April tags (requires a gray scale image)
				cv::cvtColor(image, image_gray, CV_BGR2GRAY);
				detections = tag_detector.extractTags(image_gray);

				// show the current image including any detections
				for (auto &detection : detections) {
					detection.draw(image);
				}
			}
			image.copyTo(all_image(cv::Rect((i%grid_width) * width, (i/grid_width) * height, width, height)));
		}
                key = (char) cv::waitKey(1);

		cv::imshow("image", all_image);
		if (key == 'c') {
            image_timestamps.push_back(get_timestamp());
		}
		if (key == 'd') {
			detect = !detect;
		}
		if (key == 'q' || key == 27) {
			done = true;
			break;
		}
	}

	// stop telemetry
	if (telemetry) {
		mav.stop();
	}

	thread_group.join_all();

	// save metadata
	if (output.size()) {
		pt::ptree tree;
		for (int i = 0; i < n_cameras; i++) {
			pt::ptree tcamera;
			tcamera.put("serial_number", getCameraSerialNumberFromId(camera_ids[i]));
			tcamera.put("video.filename", video_filenames[i]);
			pt::ptree ttimestamps;
			for (long &timestamp : timestamps[i]) {
				pt::ptree ttimestamp;
				ttimestamp.put("", timestamp);
				ttimestamps.push_back(make_pair("", ttimestamp));
			}
			tcamera.add_child("video.timestamps", ttimestamps);

			tree.add_child("cameras." + camera_ids[i], tcamera);
		}

		pt::write_json(file_prefix + "_info.json", tree);
	}

	return 0;
}
