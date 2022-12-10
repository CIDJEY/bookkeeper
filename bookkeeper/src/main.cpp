
#include <iostream>
#include <sstream>
#include <array>

#include "spdlog/spdlog.h"

#include "session.hpp"

using asio::io_context;
using asio::buffer;
using asio::ip::tcp;
using asio::co_spawn;
using asio::awaitable;
using asio::use_awaitable;
using asio::detached;
using asio::ssl::context;

using ssl_socket = asio::ssl::stream<tcp::socket>;

struct SslSession {
	SslSession() = delete;
	SslSession(const SslSession&) = delete;
	SslSession& operator=(const SslSession&) = delete;

	SslSession(tcp::socket socket, asio::ssl::context& conext);

	~SslSession();

	awaitable<void> completeAccept();
	awaitable<std::string> getMessage();
	awaitable<void> sendMessage(const std::string& message);
	awaitable<void> shutdown();
	awaitable<void> run();
	void close();

	uint32_t num() const { return mNum; }

private:
	ssl_socket mSocket;
	uint32_t mNum;

	std::array<uint8_t, 1024> mBuffer;

	static uint32_t sNumCounter;
};

uint32_t SslSession::sNumCounter = 0;

SslSession::SslSession(tcp::socket socket, asio::ssl::context& context)
	: mSocket(std::move(socket), context)
	, mNum(++sNumCounter)
{
}

SslSession::~SslSession() {
	spdlog::debug("[Session] #{}: Destroying session", mNum);
	close();
}

void SslSession::close() {
	if (mSocket.lowest_layer().is_open()) {
		spdlog::debug("[Session] #{}: Closing socket", mNum);
		mSocket.lowest_layer().close();
	}
}

awaitable<void> SslSession::shutdown() {
	if (mSocket.lowest_layer().is_open()) {
		spdlog::debug("[Session] #{}: Shutting down the connection", mNum);
		co_await mSocket.async_shutdown(use_awaitable);
	}
}

awaitable<void> SslSession::completeAccept() {
	co_await mSocket.async_handshake(asio::ssl::stream_base::server, use_awaitable);
}

awaitable<std::string> SslSession::getMessage() {

	auto bytes = co_await mSocket.async_read_some(buffer(mBuffer), use_awaitable);
	co_return std::string{(const char*)mBuffer.data(), bytes};
}

awaitable<void> SslSession::sendMessage(const std::string& message) {
	co_await async_write(mSocket, buffer(message), use_awaitable);
}

awaitable<void> SslSession::run() {
	//spdlog::info("[Session] #{}: Started new connection with {}", mNum, to_string(mSocket.lowest_layer().remote_endpoint()));
	while (true) {
		auto message = co_await getMessage();
	//	spdlog::info("[Session] #{}: Message from {}: {}", mNum, to_string(mSocket.lowest_layer().remote_endpoint()), message);

		message += " yourself!";

		co_await sendMessage(message);
	}
}

struct Server {
	Server() = delete;
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	explicit Server(io_context& ctx, tcp::endpoint listenEndpoint);
	~Server();

	awaitable<void> start();
	awaitable<void> handleAccept(tcp::socket socket);

private:
	tcp::acceptor mAcceptor;
};

Server::Server(io_context& ctx, tcp::endpoint listenEndpoint)
	: mAcceptor{ctx, listenEndpoint}
{
}

Server::~Server() {
	spdlog::debug("[Server]: Destroying server");
}

awaitable<void> Server::start() {
	spdlog::info("[Server]: Starting server on {}", network::to_string(mAcceptor.local_endpoint()));

	while (true) {
		auto socket = co_await mAcceptor.async_accept(use_awaitable);
		co_spawn(mAcceptor.get_executor(), handleAccept(std::move(socket)), detached);
	}
}

awaitable<void> Server::handleAccept(tcp::socket socket) {
	auto session = bookkeeper::Session{network::TcpStream{std::move(socket)}};

	try {
		co_await session.run();
	} catch (const std::exception& error) {
		spdlog::error("#{}: {}", session.num(), error.what());
	}
}

/*awaitable<void> Server::handleAcceptSecure(tcp::socket socket) {
	auto sslCtx = asio::ssl::context{context::sslv23};

	try {
		sslCtx.set_options(
	        context::default_workarounds
	        | context::no_sslv2);

    	sslCtx.use_certificate_file("/home/user/work/bookkeeper/bookkeeper/etc/cert/server.cert", context::pem);
    	sslCtx.use_private_key_file("/home/user/work/bookkeeper/bookkeeper/etc/cert/server.key", context::pem);
	} catch (const std::exception& error) {
		spdlog::error("{}", error.what());
	}

	auto session = SslSession{std::move(socket), sslCtx};

	try {

		co_await session.completeAccept();
		co_await session.run();
		co_await session.shutdown();
	} catch (const std::exception& error) {
		spdlog::error("#{}: {}", session.num(), error.what());
	}

}*/

int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);
		io_context io;

		auto serverTcp = Server{io, *tcp::resolver(io).resolve("", "8080")};
		//auto serverSsl = Server{io, *tcp::resolver(io).resolve("", "8443"), true};

		co_spawn(io, serverTcp.start(), detached);
		//co_spawn(io, serverSsl.start(), detached);

		io.run();
	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
	}

	return 0;
}