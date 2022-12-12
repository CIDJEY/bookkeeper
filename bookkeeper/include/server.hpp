
#pragma once

#include "asio.hpp"
#include "asio/ssl.hpp"

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#include <iostream>

#include "session.hpp"

using asio::ip::tcp;

namespace bookkeeper {

struct Server {
	Server() = delete;
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	explicit Server(asio::io_context& ctx)
		: mAcceptor(ctx)
	{
	}

	~Server() {}

	awaitable<void> start(auto&& handleAccept) {
		auto endpoint = tcp::endpoint(tcp::v4(), mPort);
		mAcceptor.open(endpoint.protocol());
		mAcceptor.set_option(tcp::acceptor::reuse_address(true));
		mAcceptor.bind(endpoint);

		mAcceptor.listen();

		spdlog::info("[Server]: Starting server on {}", network::to_string(mAcceptor.local_endpoint()));

		try {
			while (true) {
				auto socket = co_await mAcceptor.async_accept(asio::use_awaitable);
				std::cout << "Got something!\n";
				asio::co_spawn(mAcceptor.get_executor(), handleAccept(std::move(socket)), asio::detached);
			}
		} catch (const std::exception& error) {
			std::cout << "Wtf " << error.what() << std::endl;
		}

	}

protected:
	tcp::acceptor mAcceptor;
	uint16_t mPort;
};

struct TcpServer: Server {
	using Server::Server;

	explicit TcpServer(asio::io_context& ctx, const nlohmann::json& config)
	: Server(ctx)
	{
		try {
			mPort = config.value("open_port", 0);

			if (!mPort) {
				throw std::runtime_error("[Server]: No open_port specified in config");
			}

		} catch (const std::exception& error) {
			throw std::runtime_error(std::string("[TcpServer Construction]: ") + error.what());
		}

	}

	awaitable<void> start() {
		co_await Server::start([this](tcp::socket socket) { return handleAccept(std::move(socket)); });
	}

	awaitable<void> handleAccept(tcp::socket socket) {
		auto session = Session{network::TcpStream{std::move(socket)}};

		try {
			co_await session.run();
		} catch (const std::exception& error) {
			spdlog::error("#{}: {}", session.num(), error.what());
		}
	}
};

/* SSL-related */

struct SslServer: Server {

	using SslContext = asio::ssl::context;

	explicit SslServer(asio::io_context& ctx, const nlohmann::json& config)
	: Server(ctx)
	, mSslCtx{SslContext::sslv23}
	{
		try {
			mPort = config.value("ssl_port", 0);

			if (!mPort) {
				throw std::runtime_error("[Server]: No open_port specified in config");
			}

			auto certFile = config.value("cert_file", "");

			if(!certFile.length()) {
				throw std::runtime_error("[SslContextBuilder]: there is no \"config_file\" field in the config");
			}

			auto keyFile = config.value("key_file", "");

			if(!keyFile.length()) {
				throw std::runtime_error("[SslContextBuilder]: there is no \"key_file\" field in the config");
			}

			mSslCtx.set_options(SslContext::default_workarounds | SslContext::no_sslv2);

			mSslCtx.use_certificate_file(certFile, SslContext::pem);
			mSslCtx.use_private_key_file(keyFile, SslContext::pem);
		} catch (const std::exception& error) {
			throw std::runtime_error(std::string("[SslServer Construction]: ") + error.what());
		}

	}

	awaitable<void> handleAccept(tcp::socket socket) {
		auto sslSocket = network::SslSocket{std::move(socket), mSslCtx};
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

	awaitable<void> start() {
		co_await Server::start([this](tcp::socket socket) { return handleAccept(std::move(socket)); });
	}

private:
	SslContext mSslCtx;
};

} //namespace bookkeeper
