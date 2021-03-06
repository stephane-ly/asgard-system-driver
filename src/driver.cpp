//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "asgard/driver.hpp"

namespace {

// Configuration
std::vector<asgard::KeyValue> config;

// The driver connection
asgard::driver_connector driver;

// The remote IDs
int source_id = -1;
int sensor_id = -1;

void stop(){
    std::cout << "asgard:system: stop the driver" << std::endl;

    asgard::unregister_sensor(driver, source_id, sensor_id);
    asgard::unregister_source(driver, source_id);

    // Unlink the client socket
    unlink(asgard::get_string_value(config, "sys_client_socket_path").c_str());

    // Close the socket
    close(driver.socket_fd);
}

void terminate(int){
    stop();

    std::exit(0);
}

double read_system_temperature(){
    std::ifstream is(asgard::get_string_value(config, "sys_thermal").c_str());
    std::string line;
    std::getline(is, line);
    int value = std::atoi(line.c_str());
    return value / (double)1000;
}

} //End of anonymous namespace

int main(){
    // Load the configuration file
    asgard::load_config(config);
    
    // Open the connection
    if(!asgard::open_driver_connection(driver, asgard::get_string_value(config, "server_socket_addr").c_str(), asgard::get_int_value(config, "server_socket_port"))){
        return 1;
    }

    // Register signals for "proper" shutdown
    signal(SIGTERM, terminate);
    signal(SIGINT, terminate);

    // Register the source and sensors
    source_id = asgard::register_source(driver, "system");
    sensor_id = asgard::register_sensor(driver, source_id, "TEMPERATURE", "cpu");

    // Send data to the server continuously
    while(true){
        double value = read_system_temperature();

        asgard::send_data(driver, source_id, sensor_id, value);

        // Wait some time before messages
        usleep(asgard::get_int_value(config, "sys_delay_ms") * 1000);
    }

    stop();

    return 0;
}
