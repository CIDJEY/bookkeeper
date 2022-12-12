
#pragma once

#include <asio.hpp>
#include <streambuf>

#include "network/channel.hpp"
#include "network/stream.hpp"

namespace client {

static uint32_t sSessionCounter = 0;

template <typename Stream>
struct Session {
	Session() = delete;
	Session(const Session& other) = delete;
	Session& operator=(const Session& other) = delete;

	Session(Session&& other)
		: mChannel{std::move(other.mChannel)}
		, mNum(other.mNum)
	{}

	Session& operator=(Session&& other) {
		std::swap(mChannel, other.mChannel);
		std::swap(mNum, other.mNum);
	}

	~Session();

	explicit Session(Stream&& stream)
		: mChannel{std::move(stream)}
		, mNum(++sSessionCounter)
	{}

	uint32_t num() const { return mNum; }
	asio::awaitable<void> run();

private:
	void close();
	uint32_t mNum;

	network::Channel<Stream> mChannel;
};

template <typename Stream>
Session<Stream>::~Session() {
	spdlog::debug("[Session] #{}: Destroying session", mNum);
	close();
}

template <typename Stream>
asio::awaitable<void> Session<Stream>::run() {
	std::string isSecure = Stream::isSecure ? "secured" : "open";

	spdlog::info("[Session] #{}: Started new {} session with {}", mNum, isSecure, mChannel.remoteEndpoint());
	spdlog::info("[Session] #{}: Type \":exit\" to exit", mNum);

	auto inputStream = asio::streambuf{1024};
	auto streamDescriptor = asio::posix::stream_descriptor{co_await asio::this_coro::executor, ::dup(STDIN_FILENO)};

	while(true) {
		auto bytes = co_await asio::async_read_until(streamDescriptor, inputStream, "\n", use_awaitable);

		std::istream is(&inputStream);
		std::string message;
		std::getline(is, message);

		if (message == ":exit") {
			break;
		}

		spdlog::info("[Session] #{}: Sending message \"{}\"", mNum, message);

		co_await mChannel.sendMessage(message);
		auto reply = co_await mChannel.getMessage();
		spdlog::info("[Session] #{}: got reply from the server: {}", mNum, reply);
	}

	co_await mChannel.shutdown();
}

template <typename Stream>
void Session<Stream>::close() {
	if (mChannel.isOpen()) {
		spdlog::debug("[Session] #{}: Closing channel", mNum);
		mChannel.close();
	}
}
}