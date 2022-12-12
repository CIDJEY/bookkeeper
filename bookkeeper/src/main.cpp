
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "server.hpp"

#include <filesystem>

using asio::ip::tcp;

using namespace bookkeeper;

nlohmann::json& defaultConfig() {
	auto execDir = std::filesystem::canonical("/proc/self/exe").parent_path();

	execDir += "/../../etc/cert";

	auto certFile = execDir;
	auto keyFile = execDir;
	certFile += "/server.cert";
	keyFile += "/server.key";

	static nlohmann::json config = nlohmann::json::parse(fmt::format(R"(
			{{
				"open_port": 8080,
				"ssl_port": 8443,
				"cert_file": "{}",
				"key_file": "{}"
			}}
		)", std::string(certFile), std::string(keyFile)));

	return config;
}

int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);
		asio::io_context io;

		auto tcpServer = TcpServer{io, defaultConfig()};
		auto sslServer = SslServer{io, defaultConfig()};

		asio::co_spawn(io, tcpServer.start(), asio::detached);
		asio::co_spawn(io, sslServer.start(), asio::detached);

		io.run();
	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
	}

	return 0;
}