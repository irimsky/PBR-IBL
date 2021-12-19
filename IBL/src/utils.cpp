#include <fstream>
#include <sstream>
#include <memory>

#include "utils.hpp"

std::string File::readText(const std::string& filename)
{

	std::ifstream file{ filename };
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + filename);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::vector<char> File::readBinary(const std::string& filename)
{
	std::ifstream file{ filename, std::ios::binary | std::ios::ate };
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + filename);
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	file.read(buffer.data(), size);
	return buffer;
}