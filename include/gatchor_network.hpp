#pragma once

#include "gatchor_chain.hpp"

#include <cstdint>

namespace gatchor {

enum class NetworkError {
    none,
    socket_error,
    timeout,
    malformed_message,
    invalid_block,
    chain_error
};

class LoopbackBlockServer {
public:
    LoopbackBlockServer() = default;
    ~LoopbackBlockServer();

    LoopbackBlockServer(const LoopbackBlockServer&) = delete;
    LoopbackBlockServer& operator=(const LoopbackBlockServer&) = delete;

    NetworkError start(uint16_t port = 0) noexcept;
    NetworkError receive_and_append(ChainStore& chain, uint64_t current_time);

    uint16_t port() const noexcept;

private:
    int socket_fd_ = -1;
    uint16_t port_ = 0;
};

NetworkError send_block_loopback(uint16_t port, const PowBlock& block);

} // namespace gatchor
