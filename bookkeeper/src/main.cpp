
#include <iostream>

int main(int argc, char** argv) {
	try {

		std::cout << "Hello world\n";

	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
	}

	return 0;
}