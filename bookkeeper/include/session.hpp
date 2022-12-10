#pragma once

#include "asio.hpp"
#include "spdlog/spdlog.h"

#include "network/stream.hpp"
#include "network/channel.hpp"

namespace bookkeeper {

static uint32_t sSessionCounter = 0;

template <typename Stream>
struct Session {
	Session() = delete;
	Session(const Session& other) = delete;
	Session& operator=(const Session& other) = delete;
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
	spdlog::info("[Session] #{}: Started new session with {}", mNum, mChannel.remoteEndpoint());

	while (true) {
		auto message = co_await mChannel.getMessage();

		spdlog::info("[Session] #{}: Message from {}: {}", mNum, mChannel.remoteEndpoint(), message);
		message += " yourself!";
		co_await mChannel.sendMessage(message);
	}

	co_await mChannel.shutdown();
	co_return;
}

template <typename Stream>
void Session<Stream>::close() {
	if (mChannel.isOpen()) {
		spdlog::debug("[Session] #{}: Closing channel", mNum);
		mChannel.close();
	}
}

}