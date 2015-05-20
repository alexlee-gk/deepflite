#include "utils.h"
#include <string>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
namespace fs = boost::filesystem;

int getDeviceId(string camera_serial_number) {
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
				device_id = boost::lexical_cast<int>(filename.substr(5));
				break;
			}
		}
	}
	return device_id;
}
