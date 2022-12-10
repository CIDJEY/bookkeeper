
#pragma once

#include "asio.hpp"
#include "asio/ssl.hpp"

using asio::awaitable;
using asio::use_awaitable;
using asio::ip::tcp;

namespace network {

std::string to_string(const tcp::endpoint& endpoint) {
	std::stringstream ss;
	ss << endpoint;
	return ss.str();
}

template <typename Handler>
struct Stream {
	Stream() = delete;
	Stream(const Stream&) = delete;
	Stream& operator=(const Stream&) = delete;

	virtual ~Stream() {}

	Stream(Stream&& other)
		: mHandler{std::move(other.mHandler)}
	{}

	Stream& operator=(Stream&& other) {
		std::swap(mHandler, other.mHandler);
	}


	Stream(Handler handler)
		: mHandler{std::move(handler)}
	{}

	awaitable<size_t> asyncWrite(auto&& buffer) {
		auto bytes = co_await asio::async_write(mHandler, buffer, use_awaitable);
		co_return bytes;

	}

	awaitable<size_t> asyncRead(auto&& buffer) {
		auto bytes = co_await mHandler.async_read_some(buffer, use_awaitable);
		co_return bytes;
	}


protected:
	Handler mHandler;
};


using TcpSocket = tcp::socket;

struct TcpStream: Stream<TcpSocket> {
	using Stream<TcpSocket>::Stream;

	TcpStream(TcpStream&& other): Stream<TcpSocket>(std::move(other)) {}
	~TcpStream() { close(); }

	bool isOpen() const { return mHandler.is_open(); }
	std::string remoteEndpoint() const { return to_string(mHandler.remote_endpoint()); }

	awaitable<void> asyncShutdown() {
		mHandler.shutdown(TcpSocket::shutdown_both);
		co_return;
	}

	void close() {
		if (isOpen()) {
			mHandler.close();
		}
	}
};

using SslSocket = asio::ssl::stream<TcpSocket>;

struct SslStream: Stream<SslSocket>
{
	using Stream<SslSocket>::Stream;
	SslStream(SslStream&& other): Stream<SslSocket>(std::move(other)) {}
	~SslStream() { close(); }

	bool isOpen() const { return mHandler.lowest_layer().is_open(); }
	std::string remoteEndpoint() const { return to_string(mHandler.lowest_layer().remote_endpoint()); }

	awaitable<void> asyncShutdown() {
		co_await mHandler.async_shutdown(use_awaitable);
	}

	void close() {
		if (isOpen()) {
			mHandler.lowest_layer().close();
		}
	}
};

} //namespace network