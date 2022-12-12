
#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>

#include "session.hpp"

using asio::ip::tcp;

namespace client {

struct Client {
	Client() = delete;
	Client(const Client&) = delete;
	Client& operator=(const Client&) = delete;

	virtual ~Client() {}

	Client(asio::io_context& io)
		: mIo(io)
	{}

	asio::awaitable<void> runSession(tcp::endpoint endpoint, auto&& connect) {
		try {
			auto session = co_await connect(endpoint);

			spdlog::info("[Client] Connected!");

			co_await session.run();
		} catch (const std::exception& error) {
			spdlog::info("[Client]: caught exception: {}", error.what());
		}


		co_return;
	}

protected:
	asio::io_context& mIo;
};

struct TcpClient: Client {
	using Client::Client;

	asio::awaitable<void> runSession(tcp::endpoint endpoint) {
		co_await Client::runSession(endpoint, [this](tcp::endpoint endpoint) { return connect(endpoint); });
	}

	asio::awaitable<Session<network::TcpStream>> connect(tcp::endpoint endpoint) {
		tcp::socket socket(mIo);
		co_await socket.async_connect(endpoint, use_awaitable);
		co_return Session{network::TcpStream{std::move(socket)}};
	}
};

struct SslClient: Client {
	SslClient(asio::io_context& io)
		: Client(io)
		, mSslCtx{asio::ssl::context::sslv23}
	{
		mSslCtx.set_default_verify_paths();
		mSslCtx.set_verify_mode(asio::ssl::verify_none);
	}


	asio::awaitable<void> runSession(tcp::endpoint endpoint) {
		co_await Client::runSession(endpoint, [this](tcp::endpoint endpoint) { return connect(endpoint); });
	}

	asio::awaitable<Session<network::SslStream>> connect(tcp::endpoint endpoint) {
		auto socket = network::SslSocket{mIo, mSslCtx};
		co_await socket.lowest_layer().async_connect(endpoint, use_awaitable);
		co_await socket.async_handshake(asio::ssl::stream_base::client, use_awaitable);
		co_return Session{network::SslStream{std::move(socket)}};
	}

private:
	asio::ssl::context mSslCtx;
};

} //namespace client