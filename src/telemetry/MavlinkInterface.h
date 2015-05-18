#ifndef MAVLINK_INTERFACE_H_
#define MAVLINK_INTERFACE_H_

#include "mavlink/serial_port.h"
#include "mavlink/common/mavlink.h"
#include "common/utils.h"

#include <iostream>
#include <fstream>
#include <boost/thread.hpp>
#include <queue>

using namespace std;




class MavlinkInterface{
public:
    MavlinkInterface();
    ~MavlinkInterface();

    void start(string uart_name, int baudrate);
    void stop();
    void request_data_stream(uint8_t stream_id, uint16_t rate, uint8_t start_stop);
    int write_message(mavlink_message_t msg);

    void read_messages_thread();
    void write_file_thread();

private:
    void read_message();


    Serial_Port *_serial_port;
    boost::thread _write_file_thread;
    boost::thread _read_thread;


    queue<mavlink_message_t> _msgs;
    bool _done;
    bool _debug;

    int _system_id;
	int _component_id;

};




// for boost:thread
void start_mavlink_read_thread(MavlinkInterface *mav);
void start_mavlink_save_file_thread(MavlinkInterface *mav);



#endif // AUTOPILOT_INTERFACE_H_
