#include "utils.h"
#include <string>
#include <map>
#include <boost/filesystem.hpp>

using namespace std;
namespace fs = boost::filesystem;

int getDeviceId(string id) {
	int device_id;
	try {
		device_id = stoi(id);
	} catch (invalid_argument& e) {
		string camera_serial_number;
		try {
			camera_serial_number = getCameraSerialNumberFromId(id);
		} catch (invalid_argument& esn) {
			camera_serial_number = id;
		}
		device_id = getDeviceIdFromSerialNumber(camera_serial_number);
	}
	return device_id;
}

int getDeviceIdFromSerialNumber(string camera_serial_number) {
	fs::path full_path("/dev");
	fs::directory_iterator end_iter;
	int device_id = -1;
	for (fs::directory_iterator dir_itr(full_path); dir_itr != end_iter; dir_itr++) {
		string filename = dir_itr->path().filename().string();
		if (filename.find("video") != string::npos) {
			string cmd = "udevadm info -a -n /dev/" + filename + " | grep '{serial}' | head -n1";
			FILE* pipe = popen(cmd.c_str(), "r");
			char buffer[128];
			string cmd_res = "";
			while(!feof(pipe)) {
				if(fgets(buffer, 128, pipe) != NULL)
					cmd_res += buffer;
			}
			pclose(pipe);
			if (cmd_res.find(camera_serial_number) != string::npos) {
				device_id = stoi(filename.substr(5));
				break;
			}
		}
	}
	if (device_id == -1) {
		throw runtime_error("Camera with serial number " + camera_serial_number + " could not be found");
	}
	return device_id;
}

string getCameraSerialNumberFromDeviceId(int device_id) {
	string path_name = "/dev/video" + to_string(device_id);
	if (!boost::filesystem::exists(path_name)) {
		throw runtime_error("Camera with device id " + to_string(device_id) + " could not be found");
	}
	string cmd = "udevadm info -a -n " + path_name + " | grep '{serial}' | head -n1";
	FILE* pipe = popen(cmd.c_str(), "r");
	char buffer[128];
	string cmd_res = "";
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL)
			cmd_res += buffer;
	}
	pclose(pipe);
	int end_pos = cmd_res.rfind("\"");
	int start_pos = cmd_res.rfind("\"", end_pos-1) + 1;
	if (start_pos == 0 || end_pos == -1) {
		throw runtime_error("Camera with device id " + to_string(device_id) + " could not be found");
	}
	string camera_serial_number = cmd_res.substr(start_pos, end_pos - start_pos);
	return camera_serial_number;
}

string getCameraSerialNumberFromId(string camera_id) {
	string camera_serial_number;
	try {
		camera_serial_number = map<string, string> {{"A", "EE96593F"}, {"B", "E8FE493F"}}.at(camera_id);
	} catch (out_of_range& e) {
		throw invalid_argument("Invalid camera id: " + camera_id);
	}
	return camera_serial_number;
}

string getCameraIdFromSerialNumber(string camera_serial_number) {
	string camera_id;
	try {
		camera_id = map<string, string> {{"EE96593F", "A"}, {"E8FE493F", "B"}}.at(camera_serial_number);
	} catch (out_of_range& e) {
		throw invalid_argument("Invalid camera id: " + camera_serial_number);
	}
	return camera_id;
}
