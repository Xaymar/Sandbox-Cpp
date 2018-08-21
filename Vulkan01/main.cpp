#include <inttypes.h>
#include <vector>
#include <iostream>
#include <ostream>
#include <iomanip>

int main(int argc, char* argv[]) {
	std::vector<char> buf(255);

	for (size_t idx = 0; idx < buf.size(); idx++) {
		std::cout << std::setw(sizeof(uint8_t) * 2) << std::setfill('0') << std::hex << int(0xFF & buf[idx]) << " ";
	}
	std::cout << std::endl;

	uint8_t* data = reinterpret_cast<uint8_t*>(buf.data());
	*data = 11; data++;
	*reinterpret_cast<uint32_t*>(data) = 0xADADADul; data += 4;
	*data = 11; data++;
	*reinterpret_cast<uint32_t*>(data) = 0xADADADul; data += 4;
	*data = 11; data++;
	*reinterpret_cast<uint32_t*>(data) = 0xADADADul; data += 4;
	*data = 11; data++;
	*reinterpret_cast<uint32_t*>(data) = 0xADADADul; data += 4;

	for (size_t idx = 0; idx < buf.size(); idx++) {
		std::cout << std::setw(sizeof(uint8_t) * 2) << std::setfill('0') << std::hex << int(0xFF & buf[idx]) << " ";
	}
	std::cout << std::endl;

	std::cin.get();
	return 0;
}