#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>

int main ()
{

std::cout << " Linux C++ Telemetry Engine " << std::endl;
std::ifstream serial_port("/dev/ttyACM0", std::ios::in | std::ios::binary);

if(!serial_port.is_open())
{
	std::cerr << " Critical Error: Could not open /dev/ttyACM0" << std::endl;
	std::cerr << " Verify if uspipd is attached and stty is configured" << std::endl;
	return 1;
}

std::string line;

while (std::getline(serial_port, line)) {
	
	if (line.empty()) continue;

	std::cout << "Raw Frame: " << line << std::endl;

	if (line.find("TEMP:") != std::string::npos) {
		
		try{
			size_t temp_pos = line.find("TEMP:") + 5;
			size_t comma1   = line.find(",", temp_pos);
			size_t hum_pos  = line.find("HUM:") + 4;
			size_t comma2   = line.find(",", hum_pos);
			size_t ts_pos   = line.find("TS:") + 3;
			
			std::string temp_val = line.substr(temp_pos, comma1 - temp_pos);
			std::string hum_val  = line.substr(hum_pos, comma2 - hum_pos);
			std::string ts_val   = line.substr(ts_pos);


			std::cout << "   ├── Parsed Temp : " << temp_val << " °C" << std::endl;
			std::cout << "   ├── Parsed Hum  : " << hum_val  << " %" << std::endl;
			std::cout << "   └── Timestamp   : " << ts_val   << " ms\n" << std::endl;
		}
		catch (...) {
                std::cout << "Warning: Packet framing error or partial line drop." << std::endl;
            }
        }
    }

    return 0;
}
