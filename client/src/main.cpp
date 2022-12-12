
#include <iostream>
#include <sstream>
#include <array>

#include "asio.hpp"
#include "asio/ssl.hpp"

#include "spdlog/spdlog.h"

using asio::ip::tcp;
using asio::awaitable;
using asio::buffer;
using asio::co_spawn;
using asio::streambuf;
using asio::detached;
using asio::use_awaitable;

using asio::ssl::context;

std::string to_string(tcp::endpoint endpoint) {
	std::stringstream ss;
	ss << endpoint;

	return ss.str();
}

struct Session {
	Session() = delete;
	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;

	explicit Session(tcp::socket socket);

	~Session();

	awaitable<void> run();
	void close();

private:
	tcp::socket mSocket;
	uint32_t mNum;

	static uint32_t sNumCounter;
};

uint32_t Session::sNumCounter = 0;

Session::Session(tcp::socket socket)
	: mSocket(std::move(socket))
	, mNum(++sNumCounter)
{
}

awaitable<void> Session::run() {
	spdlog::info("[Session] #{}: started new session with {}", mNum, to_string(mSocket.remote_endpoint()));
	spdlog::info("[Session] #{}: type \":exit\" to exit the sesison", mNum);

	auto inputStream = streambuf{1024};
	auto streamDescriptor = asio::posix::stream_descriptor{co_await asio::this_coro::executor, ::dup(STDIN_FILENO)};

	std::array<uint8_t, 1024> buf;

	while(true) {
		auto bytes = co_await asio::async_read_until(streamDescriptor, inputStream, "\n", use_awaitable);

		std::istream is(&inputStream);
		std::string message;
		std::getline(is, message);

		spdlog::info("[Session] #{}: Sending message \"{}\"", mNum, message);

		co_await async_write(mSocket, buffer(message), use_awaitable);

		bytes = co_await mSocket.async_read_some(buffer(buf), use_awaitable);

		message = std::string{(const char*)buf.data(), bytes};
		spdlog::info("[Session] #{}: got reply from server: {}", mNum, message);
	}

}

Session::~Session() {
	spdlog::info("[Session] #{}: destroying connection", mNum);
	close();
}


void Session::close() {
	if (mSocket.is_open()) {
		spdlog::info("[Session] #{}: Closing socket", mNum);
	}
}


using ssl_socket = asio::ssl::stream<tcp::socket>;

struct SessionSecure {
	SessionSecure() = delete;
	SessionSecure(const SessionSecure&) = delete;
	SessionSecure& operator=(const SessionSecure&) = delete;

	explicit SessionSecure(ssl_socket socket);

	~SessionSecure();

	awaitable<void> completeConnect();
	awaitable<void> shutdown();
	awaitable<void> run();
	void close();

private:
	ssl_socket mSocket;
	uint32_t mNum;

	static uint32_t sNumCounter;
};

uint32_t SessionSecure::sNumCounter = 0;

SessionSecure::SessionSecure(ssl_socket socket)
	: mSocket(std::move(socket))
	, mNum(++sNumCounter)
{
}

awaitable<void> SessionSecure::completeConnect() {
	co_await mSocket.async_handshake(asio::ssl::stream_base::client, use_awaitable);
}

awaitable<void> SessionSecure::run() {
	spdlog::info("[SessionSecure] #{}: started new session with {}", mNum, to_string(mSocket.lowest_layer().remote_endpoint()));
	spdlog::info("[SessionSecure] #{}: type \":exit\" to exit the sesison", mNum);

	auto inputStream = streambuf{1024};
	auto streamDescriptor = asio::posix::stream_descriptor{co_await asio::this_coro::executor, ::dup(STDIN_FILENO)};

	std::array<uint8_t, 1024> buf;

	while(true) {
		auto bytes = co_await asio::async_read_until(streamDescriptor, inputStream, "\n", use_awaitable);

		std::istream is(&inputStream);
		std::string message;
		std::getline(is, message);

		spdlog::info("[Session] #{}: Sending message \"{}\"", mNum, message);

		co_await async_write(mSocket, buffer(message), use_awaitable);

		bytes = co_await mSocket.async_read_some(buffer(buf), use_awaitable);

		message = std::string{(const char*)buf.data(), bytes};
		spdlog::info("[Session] #{}: got reply from server: {}", mNum, message);
	}

}

SessionSecure::~SessionSecure() {
	spdlog::info("[SessionSecure] #{}: destroying connection", mNum);
	close();
}

awaitable<void> SessionSecure::shutdown() {
	spdlog::info("[SessionSecure] #{}: Shutting down connection", mNum);

	co_await mSocket.async_shutdown(use_awaitable);

}

void SessionSecure::close() {
	if (mSocket.lowest_layer().is_open()) {
		spdlog::info("[Session] #{}: Closing socket", mNum);
		mSocket.lowest_layer().close();
	}
}

struct Client {
	Client(asio::io_context& io): mIo(io) {}
	Client(const Client&) = delete;
	Client& operator=(const Client&) = delete;

	awaitable<void> runSession(tcp::endpoint endpoint);

private:
	asio::io_context& mIo;
};

awaitable<void> Client::runSession(tcp::endpoint endpoint) {
	try {
		spdlog::info("[Client] Starting new session with {}", to_string(endpoint));

		asio::ssl::context ctx(asio::ssl::context::sslv23);
		ctx.set_default_verify_paths();

		ctx.set_verify_mode(asio::ssl::verify_none);

		ssl_socket socket(mIo, ctx);
		//tcp::socket socket(mIo);
		co_await socket.lowest_layer().async_connect(endpoint, use_awaitable);

		spdlog::info("[Client] Connected!");

		auto session = SessionSecure(std::move(socket));

		co_await session.completeConnect();
		co_await session.run();
		co_await session.shutdown();


	} catch (const std::exception& error) {
		spdlog::info("[Client]: caught exception: {}", error.what());
	}


	co_return;
}


int main(int argc, char** argv) {
	try {
		spdlog::set_level(spdlog::level::debug);

		asio::io_context io;

		auto client = Client{io};

		auto endpoint = *tcp::resolver(io).resolve("0.0.0.0", "8443");
		co_spawn(io, client.runSession(endpoint), detached);

		io.run();

	} catch (const std::exception& error) {
		spdlog::error("{}", error.what());
	}
}