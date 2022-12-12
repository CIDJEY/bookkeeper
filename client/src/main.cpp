
#include "spdlog/spdlog.h"

#include "client.hpp"

int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);

		asio::io_context io;

		auto client = client::SslClient{io};

		auto endpoint = *tcp::resolver(io).resolve("0.0.0.0", "8443");
		co_spawn(io, client.runSession(endpoint), asio::detached);

		io.run();

	} catch (const std::exception& error) {
		spdlog::error("{}", error.what());
	}
}