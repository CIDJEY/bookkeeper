
#pragma once

#include "asio.hpp"

#include <array>

namespace network {

template <typename Stream>
struct Channel {
	Channel() = delete;
	Channel(const Channel&) = delete;
	Channel& operator=(const Channel&) = delete;

	Channel(Channel<Stream>&& other)
		: mStream{std::move(other.mStream)}
	{}

	Channel& operator=(Channel<Stream>&& other) {
		std::swap(mStream, other.mStream);
	}

	Channel(Stream&& stream)
		: mStream{std::move(stream)}
	{}

	~Channel() {}

	bool isOpen() const { return mStream.isOpen(); }
	std::string remoteEndpoint() const { return mStream.remoteEndpoint(); }

	asio::awaitable<std::string> getMessage() {
		auto bytes = co_await mStream.asyncRead(asio::buffer(mBuffer));
		co_return std::string{(const char*)mBuffer.data(), bytes};
	}

	asio::awaitable<void> sendMessage(const std::string& message) {
		co_await mStream.asyncWrite(asio::buffer(message));
	}

	asio::awaitable<void> shutdown() {
		co_await mStream.asyncShutdown();
	}

	void close() {
		if (mStream.isOpen()) {
			mStream.close();
		}
	}


private:
	Stream mStream;
	std::array<uint8_t, 1024> mBuffer;
};

} //namespace network
