
#include "spdlog/spdlog.h"

#include "server.hpp"

using asio::ip::tcp;

int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);
		asio::io_context io;

		auto tcpServer = bookkeeper::TcpServer{io, *tcp::resolver(io).resolve("", "8080")};
		auto sslServer = bookkeeper::SslServer{io, *tcp::resolver(io).resolve("", "8443")};

		asio::co_spawn(io, tcpServer.start(), asio::detached);
		asio::co_spawn(io, sslServer.start(), asio::detached);

		io.run();
	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
	}

	return 0;
}