#include "gatchor_network.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <type_traits>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace gatchor {
namespace {

constexpr std::array<uint8_t, 4> MESSAGE_MAGIC = {'G', 'N', 'P', '3'};
constexpr uint8_t BLOCK_MESSAGE = 1;
constexpr size_t FRAME_HEADER_SIZE = 9;
constexpr uint32_t MAX_FRAME_SIZE = 8 * 1024 * 1024;
constexpr uint32_t MAX_TRANSACTIONS = 100000;
constexpr uint32_t MAX_TRANSACTION_SIZE = SIGNED_TRANSACTION_SIZE;
constexpr int SOCKET_TIMEOUT_SECONDS = 3;

class ScopedSocket {
public:
    explicit ScopedSocket(int socket_fd) noexcept : socket_fd_(socket_fd) {}
    ~ScopedSocket()
    {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
        }
    }

    ScopedSocket(const ScopedSocket&) = delete;
    ScopedSocket& operator=(const ScopedSocket&) = delete;

    int get() const noexcept { return socket_fd_; }

private:
    int socket_fd_;
};

void append_u32(std::vector<uint8_t>& output, uint32_t value)
{
    for (unsigned int i = 0; i < 4; ++i) {
        output.push_back(static_cast<uint8_t>(value >> (i * 8)));
    }
}

uint32_t read_u32(const uint8_t* input) noexcept
{
    return static_cast<uint32_t>(input[0]) |
        (static_cast<uint32_t>(input[1]) << 8) |
        (static_cast<uint32_t>(input[2]) << 16) |
        (static_cast<uint32_t>(input[3]) << 24);
}

bool set_socket_timeout(int socket_fd) noexcept
{
    const timeval timeout{SOCKET_TIMEOUT_SECONDS, 0};
    if (setsockopt(
        socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0 &&
        setsockopt(
            socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0) {
        return true;
    }
    return false;
}

bool send_all(int socket_fd, const uint8_t* bytes, size_t size) noexcept
{
    size_t sent = 0;
    while (sent < size) {
        const ssize_t result = send(
            socket_fd, bytes + sent, size - sent, MSG_NOSIGNAL);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return false;
        }
        sent += static_cast<size_t>(result);
    }
    return true;
}

bool receive_all(int socket_fd, uint8_t* bytes, size_t size) noexcept
{
    size_t received = 0;
    while (received < size) {
        const ssize_t result = recv(socket_fd, bytes + received, size - received, 0);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return false;
        }
        received += static_cast<size_t>(result);
    }
    return true;
}

bool encode_block(const PowBlock& block, std::vector<uint8_t>& payload)
{
    if (block.transactions.empty() ||
        block.transactions.size() > MAX_TRANSACTIONS) {
        return false;
    }

    const auto header = serialize_header(block.header);
    payload.assign(header.begin(), header.end());
    append_u32(payload, static_cast<uint32_t>(block.transactions.size()));
    for (const auto& transaction : block.transactions) {
        if (transaction.size() != MAX_TRANSACTION_SIZE ||
            payload.size() > MAX_FRAME_SIZE - sizeof(uint32_t) - transaction.size()) {
            return false;
        }
        append_u32(payload, static_cast<uint32_t>(transaction.size()));
        payload.insert(payload.end(), transaction.begin(), transaction.end());
    }
    return payload.size() <= MAX_FRAME_SIZE;
}

bool decode_block(const std::vector<uint8_t>& payload, PowBlock& block)
{
    if (payload.size() < POW_HEADER_SIZE + sizeof(uint32_t)) {
        return false;
    }

    size_t offset = 0;
    auto read_header_integer = [&payload, &offset](auto& value) {
        using Integer = std::decay_t<decltype(value)>;
        if (payload.size() - offset < sizeof(Integer)) {
            return false;
        }
        value = 0;
        for (size_t i = 0; i < sizeof(Integer); ++i) {
            value |= static_cast<Integer>(payload[offset++]) << (i * 8);
        }
        return true;
    };
    auto read_header_bytes = [&payload, &offset](uint8_t* output, size_t size) {
        if (payload.size() - offset < size) {
            return false;
        }
        std::copy_n(payload.data() + offset, size, output);
        offset += size;
        return true;
    };

    if (!read_header_integer(block.header.version) ||
        !read_header_bytes(block.header.previous_hash.data(),
            block.header.previous_hash.size()) ||
        !read_header_bytes(block.header.merkle_root.data(),
            block.header.merkle_root.size()) ||
        !read_header_integer(block.header.timestamp) ||
        !read_header_integer(block.header.difficulty) ||
        !read_header_integer(block.header.nonce)) {
        return false;
    }

    if (payload.size() - offset < sizeof(uint32_t)) {
        return false;
    }
    const uint32_t transaction_count = read_u32(payload.data() + offset);
    offset += sizeof(uint32_t);
    if (transaction_count == 0 || transaction_count > MAX_TRANSACTIONS) {
        return false;
    }

    block.transactions.clear();
    block.transactions.reserve(transaction_count);
    for (uint32_t i = 0; i < transaction_count; ++i) {
        if (payload.size() - offset < sizeof(uint32_t)) {
            return false;
        }
        const uint32_t transaction_size = read_u32(payload.data() + offset);
        offset += sizeof(uint32_t);
        if (transaction_size != MAX_TRANSACTION_SIZE ||
            payload.size() - offset < transaction_size) {
            return false;
        }
        block.transactions.emplace_back(
            payload.begin() + static_cast<std::ptrdiff_t>(offset),
            payload.begin() + static_cast<std::ptrdiff_t>(offset + transaction_size));
        offset += transaction_size;
    }
    return offset == payload.size();
}

NetworkError send_response(int socket_fd, NetworkError error) noexcept
{
    const uint8_t response = static_cast<uint8_t>(error);
    return send_all(socket_fd, &response, sizeof(response))
        ? NetworkError::none
        : NetworkError::socket_error;
}

NetworkError error_for_chain(ChainStoreError error) noexcept
{
    if (error == ChainStoreError::none) {
        return NetworkError::none;
    }
    if (error == ChainStoreError::invalid_block ||
        error == ChainStoreError::chain_not_better) {
        return NetworkError::invalid_block;
    }
    return NetworkError::chain_error;
}

} // namespace

LoopbackBlockServer::~LoopbackBlockServer()
{
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

NetworkError LoopbackBlockServer::start(uint16_t port) noexcept
{
    if (socket_fd_ >= 0) {
        return NetworkError::socket_error;
    }

    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return NetworkError::socket_error;
    }
    int reuse_address = 1;
    if (setsockopt(
            socket_fd_, SOL_SOCKET, SO_REUSEADDR,
            &reuse_address, sizeof(reuse_address)) != 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return NetworkError::socket_error;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(socket_fd_, 4) != 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return NetworkError::socket_error;
    }

    socklen_t address_size = sizeof(address);
    if (getsockname(
            socket_fd_, reinterpret_cast<sockaddr*>(&address), &address_size) != 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return NetworkError::socket_error;
    }
    port_ = ntohs(address.sin_port);
    return NetworkError::none;
}

NetworkError LoopbackBlockServer::receive_and_append(
    ChainStore& chain,
    uint64_t current_time)
{
    if (socket_fd_ < 0) {
        return NetworkError::socket_error;
    }
    ScopedSocket client(accept(socket_fd_, nullptr, nullptr));
    if (client.get() < 0) {
        return errno == EAGAIN || errno == EWOULDBLOCK
            ? NetworkError::timeout
            : NetworkError::socket_error;
    }
    if (!set_socket_timeout(client.get())) {
        return NetworkError::socket_error;
    }

    std::array<uint8_t, FRAME_HEADER_SIZE> frame_header{};
    if (!receive_all(client.get(), frame_header.data(), frame_header.size())) {
        return NetworkError::malformed_message;
    }
    if (!std::equal(MESSAGE_MAGIC.begin(), MESSAGE_MAGIC.end(), frame_header.begin()) ||
        frame_header[4] != BLOCK_MESSAGE) {
        send_response(client.get(), NetworkError::malformed_message);
        return NetworkError::malformed_message;
    }

    const uint32_t payload_size = read_u32(frame_header.data() + 5);
    if (payload_size < POW_HEADER_SIZE + sizeof(uint32_t) ||
        payload_size > MAX_FRAME_SIZE) {
        send_response(client.get(), NetworkError::malformed_message);
        return NetworkError::malformed_message;
    }

    std::vector<uint8_t> payload(payload_size);
    if (!receive_all(client.get(), payload.data(), payload.size())) {
        send_response(client.get(), NetworkError::malformed_message);
        return NetworkError::malformed_message;
    }

    PowBlock block;
    if (!decode_block(payload, block)) {
        send_response(client.get(), NetworkError::malformed_message);
        return NetworkError::malformed_message;
    }

    const NetworkError result = error_for_chain(chain.append(block, current_time));
    const NetworkError response_result = send_response(client.get(), result);
    return response_result == NetworkError::none ? result : response_result;
}

uint16_t LoopbackBlockServer::port() const noexcept
{
    return port_;
}

NetworkError send_block_loopback(uint16_t port, const PowBlock& block)
{
    std::vector<uint8_t> payload;
    if (!encode_block(block, payload)) {
        return NetworkError::malformed_message;
    }

    ScopedSocket socket_handle(socket(AF_INET, SOCK_STREAM, 0));
    if (socket_handle.get() < 0) {
        return NetworkError::socket_error;
    }
    if (!set_socket_timeout(socket_handle.get())) {
        return NetworkError::socket_error;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    if (connect(socket_handle.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        return errno == ETIMEDOUT
            ? NetworkError::timeout
            : NetworkError::socket_error;
    }

    std::vector<uint8_t> frame;
    frame.reserve(FRAME_HEADER_SIZE + payload.size());
    frame.insert(frame.end(), MESSAGE_MAGIC.begin(), MESSAGE_MAGIC.end());
    frame.push_back(BLOCK_MESSAGE);
    append_u32(frame, static_cast<uint32_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    if (!send_all(socket_handle.get(), frame.data(), frame.size())) {
        return NetworkError::socket_error;
    }

    uint8_t response = 0;
    if (!receive_all(socket_handle.get(), &response, sizeof(response))) {
        return NetworkError::socket_error;
    }
    if (response > static_cast<uint8_t>(NetworkError::chain_error)) {
        return NetworkError::malformed_message;
    }
    return static_cast<NetworkError>(response);
}

} // namespace gatchor
