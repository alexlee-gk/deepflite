#include "MavlinkInterface.h"

MavlinkInterface::MavlinkInterface(){
        _done = false;
        _debug = false;
}

MavlinkInterface::~MavlinkInterface(){}


void MavlinkInterface::start(string uart_name, int baudrate){

    cout << "Mavlink: Open serial port" << endl;

    // initialize and open the serial port
    _serial_port =  new Serial_Port(uart_name.c_str(), baudrate);
    _serial_port->start();


    if(_serial_port->status != 1){
        cerr << "Mavlink ERROR: Serial port not open" << endl;
        throw 1;
    }

    cout << "Mavlink: Start read thread" << endl;
    _read_thread = boost::thread(start_mavlink_read_thread, this);

    // wait until we receive the first message to get the system and component IDs
    cout << "Mavlink: Waiting for message... ";
    while(_msgs.empty()) {
        if (_done)
            return;
        usleep(500000);
    }

    _system_id = _msgs.front().sysid;
    _component_id = _msgs.front().compid;
    cout << "Found" << endl;



    // start write to file thread
    cout << "Mavlink: Start saving messages" << endl;
    _write_file_thread = boost::thread(start_mavlink_save_file_thread, this);


    cout << "Mavlink: Initialization successful" << endl;
}

// change the data stream request
void MavlinkInterface::request_data_stream(uint8_t stream_id, uint16_t rate, uint8_t start_stop)
{
    mavlink_request_data_stream_t req;
    req.req_message_rate = rate;
    req.target_system = _system_id;
    req.target_component = _component_id;
    req.req_stream_id = stream_id;
    req.start_stop = start_stop;

    mavlink_message_t message;
    mavlink_msg_request_data_stream_encode(_system_id, _component_id, &message, &req);

    int len = write_message(message);
}


void MavlinkInterface::read_message() {

    bool success = false;
    mavlink_message_t message;

    while(not success) {
        // read one char at a time, returns success when the complete message is read
        success = _serial_port->read_message(message);

/*
        // save the current position and attitude
        switch (message.msgid)
        {
            case MAVLINK_MSG_ID_LOCAL_POSITION_NED:
            {
                printf("MAVLINK_MSG_ID_LOCAL_POSITION_NED\n");
                mavlink_msg_local_position_ned_decode(&message, &(current_messages.local_position_ned));
                current_messages.time_stamps.local_position_ned = get_time_usec();
                this_timestamps.local_position_ned = current_messages.time_stamps.local_position_ned;
                break;
            }
            case MAVLINK_MSG_ID_ATTITUDE:
            {
                printf("MAVLINK_MSG_ID_ATTITUDE\n");
                mavlink_msg_attitude_decode(&message, &(current_messages.attitude));
                current_messages.time_stamps.attitude = get_time_usec();
                this_timestamps.attitude = current_messages.time_stamps.attitude;
                break;
            }
        }

*/
        // give time for other threads
        usleep(100);
    }


    // don't handle messages here. Just queue them.
    _msgs.push(message);
//    cout << "Received message id " << message.msgid << endl;
    if(_debug)
        printf("Received message id %i\n", message.msgid);

}



int MavlinkInterface::write_message(mavlink_message_t msg){
    int len = _serial_port->write_message(msg);

    return len;
}





void MavlinkInterface::stop(){
    cout << "Mavlink: Closing...";
    _done = true;

    _read_thread.join();
    _write_file_thread.join();


	_serial_port->stop();
	delete _serial_port;


    cout << " done" << endl;
}




void MavlinkInterface::read_messages_thread() {
	while(!_done) {
        read_message();
		usleep(1000);
	}
	return;
}


void MavlinkInterface::write_file_thread() {

    // current time string
    time_t now = time(0);
    struct tm tstruct;
    char datebuf[100];
    tstruct  = *localtime(&now);
    strftime(datebuf, sizeof(datebuf), "%Y-%m-%d-%H.%M.%S", &tstruct);


    char fileprefix[100];
    strcpy(fileprefix, "./data/");
    strcat(fileprefix, datebuf);

    char imufilename[100];
    char gpsfilename[100];
    char pressurefilename[100];

    strcat(strcpy(imufilename, fileprefix), "_IMU.txt");
    strcat(strcpy(gpsfilename, fileprefix), "_GPS.txt");
    strcat(strcpy(pressurefilename, fileprefix), "_pressure.txt");

    ofstream fp_imu, fp_gps, fp_pressure;
    fp_imu.open(imufilename);
    fp_gps.open(gpsfilename);
    fp_pressure.open(pressurefilename);


    // write the header
    fp_imu << "GCS_time timestamp xacc yacc zacc xgyro ygyro zgyro xmag ymag zmag" << endl;
    fp_gps << "GCS_time timestamp fix_type lat lon alt eph epv vel cog satellites_visible" << endl;
    fp_pressure << "GCS_time time_boot press_abs press_diff temperature" << endl;


    while (!_done)
    {
        while(!_msgs.empty())
        {

            switch(_msgs.front().msgid)
            {

                case MAVLINK_MSG_ID_RAW_IMU:
                    mavlink_raw_imu_t msg_imu;
                    mavlink_msg_raw_imu_decode(&_msgs.front(), &(msg_imu));
                    fp_imu << get_timestamp() << " " << msg_imu.time_usec << " "
                           << msg_imu.xacc  << " " << msg_imu.yacc  << " " << msg_imu.zacc  << " "
                           << msg_imu.xgyro << " " << msg_imu.ygyro << " " << msg_imu.zgyro << " "
                           << msg_imu.xmag  << " " << msg_imu.ymag  << " " << msg_imu.zmag  << endl;
                    break;
                case MAVLINK_MSG_ID_GPS_RAW_INT:
                    mavlink_gps_raw_int_t msg_gps;
                    mavlink_msg_gps_raw_int_decode(&_msgs.front(), &(msg_gps));
                    fp_gps << get_timestamp() << " " << msg_gps.time_usec << " " << (uint16_t)msg_gps.fix_type <<  " "
                           << msg_gps.lat << " " << msg_gps.lon << " " << msg_gps.alt << " "
                           << msg_gps.eph << " " << msg_gps.epv << " " << msg_gps.vel << " "
                           << msg_gps.cog << " " << (uint16_t)msg_gps.satellites_visible << endl;
                    break;
                case MAVLINK_MSG_ID_SCALED_PRESSURE:
                    mavlink_scaled_pressure_t msg_p;
                    mavlink_msg_scaled_pressure_decode(&_msgs.front(), &msg_p);
                    fp_pressure << get_timestamp() << " " << msg_p.time_boot_ms << " " << msg_p.press_abs << " "
                                << msg_p.press_diff << " " << msg_p.temperature << endl;
                    break;
            }
            _msgs.pop();


        }



        usleep(1000000); // write cycle 1Hz
    }

    fp_imu.close();
    fp_gps.close();
    fp_pressure.close();
}




void start_mavlink_read_thread(MavlinkInterface *mav){
    mav->read_messages_thread();
}

void start_mavlink_save_file_thread(MavlinkInterface *mav){
    mav->write_file_thread();
}
