
#pragma once

#include "asio.hpp"
#include "asio/ssl.hpp"

#include "spdlog/spdlog.h"

#include <iostream>

#include "session.hpp"

using asio::ip::tcp;

namespace bookkeeper {

struct Server {
	Server() = delete;
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	explicit Server(asio::io_context& ctx, tcp::endpoint listenEndpoint)
		: mAcceptor(ctx, listenEndpoint)
	{}

	~Server() {}


	awaitable<void> start() {
		spdlog::info("[Server]: Starting server on {}", network::to_string(mAcceptor.local_endpoint()));

		while (true) {
			auto socket = co_await mAcceptor.async_accept(asio::use_awaitable);
			asio::co_spawn(mAcceptor.get_executor(), handleAccept(std::move(socket)), asio::detached);
		}
	}

private:
	virtual awaitable<void> handleAccept(tcp::socket socket) = 0;
	tcp::acceptor mAcceptor;
};

struct TcpServer: Server {
	using Server::Server;

	awaitable<void> handleAccept(tcp::socket socket) override {
		auto session = Session{network::TcpStream{std::move(socket)}};

		try {
			co_await session.run();
		} catch (const std::exception& error) {
			spdlog::error("#{}: {}", session.num(), error.what());
		}
	}
};

/* SSL-related */

using asio::ssl::context;

struct SslServer: Server {
	using Server::Server;

	awaitable<void> handleAccept(tcp::socket socket) override {
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

		auto sslSocket = network::SslSocket{std::move(socket), sslCtx};
		try {
	    	co_await sslSocket.async_handshake(asio::ssl::stream_base::server, asio::use_awaitable);
		} catch (const std::exception& error) {
			spdlog::error("{}", error.what());
		}

		auto session = Session{network::SslStream{std::move(sslSocket)}};

		try {
			co_await session.run();
		} catch (const std::exception& error) {
			spdlog::error("#{}: {}", session.num(), error.what());
		}
	}
};

} //namespace bookkeeper
