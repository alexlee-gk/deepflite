#include <string>

void setupCameraSettings(int device_id, int exposure, int gain, int brightness);
int getDeviceId(std::string id);
int getDeviceIdFromSerialNumber(std::string camera_serial_number);
std::string getCameraSerialNumberFromDeviceId(int device_id);
std::string getCameraSerialNumberFromId(std::string camera_id);
std::string getCameraIdFromSerialNumber(std::string camera_serial_number);
