
#include <iostream>
#include <sstream>
#include <array>

#include "spdlog/spdlog.h"

#include "asio.hpp"

using asio::io_context;
using asio::buffer;
using asio::ip::tcp;
using asio::co_spawn;
using asio::awaitable;
using asio::use_awaitable;
using asio::detached;

std::string to_string(const tcp::endpoint& endpoint) {
	std::stringstream ss;
	ss << endpoint;
	return ss.str();
}

struct Session {
	Session() = delete;
	Session(const Session& other) = delete;
	Session& operator=(const Session& other) = delete;

	explicit Session(tcp::socket socket);

	uint32_t num() const { return mNum; }
	awaitable<void> run();
	void close();

	~Session();

private:
	awaitable<std::string> getMessage();
	awaitable<void> sendMessage(const std::string& message);

	tcp::socket mSocket;
	uint32_t mNum;
	std::array<uint8_t, 1024> mBuffer;

	static uint32_t sNumCounter;
};

uint32_t Session::sNumCounter = 0;

Session::Session(tcp::socket socket)
	: mSocket(std::move(socket))
	, mNum(++sNumCounter)
{}

Session::~Session() {
	spdlog::debug("[Session] #{}: Destroying session", mNum);
	close();
}

awaitable<void> Session::run() {
	spdlog::info("[Session] #{}: Started new connection with {}", mNum, to_string(mSocket.remote_endpoint()));

	while (true) {
		auto message = co_await getMessage();

		spdlog::info("[Session] #{}: Message from {}: {}", mNum, to_string(mSocket.remote_endpoint()), message);
		message += " yourself!";
		co_await sendMessage(message);
	}

	close();
	co_return;
}

void Session::close() {
	if (mSocket.is_open()) {
		spdlog::debug("[Session] #{}: Closing socket", mNum);
		mSocket.close();
	}
}

awaitable<std::string> Session::getMessage() {
	auto bytes = co_await mSocket.async_read_some(buffer(mBuffer), use_awaitable);

	co_return std::string{(const char*)mBuffer.data(), bytes};
}

awaitable<void> Session::sendMessage(const std::string& message) {
	co_await async_write(mSocket, buffer(message), use_awaitable);
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
	spdlog::info("[Server]: Starting server on {}", to_string(mAcceptor.local_endpoint()));

	while (true) {
		auto socket = co_await mAcceptor.async_accept(use_awaitable);
		co_spawn(mAcceptor.get_executor(), handleAccept(std::move(socket)), detached);
	}
}

awaitable<void> Server::handleAccept(tcp::socket socket) {
	auto session = Session{std::move(socket)};

	try {
		co_await session.run();
	} catch (const std::exception& error) {
		spdlog::error("#{}: {}", session.num(), error.what());
	}

}

int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);

		io_context io;

		auto server = Server{io, *tcp::resolver(io).resolve("", "50000")};

		co_spawn(io, server.start(), detached);

		io.run();
	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
	}

	return 0;
}