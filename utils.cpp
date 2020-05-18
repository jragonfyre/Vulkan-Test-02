#include "utils.h"

#include <iostream>
#include <fstream>


std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary); // ate - start at end, binary - binary file

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0); // back to beginning
	file.read(buffer.data(), fileSize); // read entire file in
	file.close(); // close file

	return buffer;
}