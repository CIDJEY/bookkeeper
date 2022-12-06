
#include <iostream>

#include "spdlog/spdlog.h"

int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);

		spdlog::info("Hello world! from the client!");
	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
	}

}