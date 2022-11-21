

#include <iostream>
#include <asio.hpp>

int main(int argc, char** argv) {

	asio::io_context io;

	asio::steady_timer timer(io, asio::chrono::seconds(3));

	timer.wait();
	std::cout << "Hello world\n";
	return 0;
}