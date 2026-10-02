#include "gatchor_network.hpp"
#include "transaction_helpers.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <utility>

namespace {

gatchor::PowBlock make_block(
    uint64_t timestamp,
    const gatchor::Gatchor256::Digest& parent_hash,
    std::vector<uint8_t> transaction,
    uint64_t nonce = 0)
{
    gatchor::PowBlock block;
    block.header.version = gatchor::POW_PROTOCOL_VERSION;
    block.header.previous_hash = parent_hash;
    block.header.timestamp = timestamp;
    block.header.difficulty = 8;
    if (!transaction.empty()) {
        block.transactions.push_back(
            test_support::make_signed_transaction(transaction, nonce));
    }
    block.header.merkle_root = gatchor::calculate_merkle_root(block.transactions);
    assert(gatchor::mine_header(block.header, 100000));
    return block;
}

} // namespace

int main()
{
    const auto unique = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path() /
        ("gatchor-loopback-" + std::to_string(unique));
    assert(std::filesystem::create_directory(directory));
    constexpr uint64_t now = 1700003000;

    const auto parameters = test_support::chain_parameters();
    gatchor::ChainStore sender(directory / "sender.dat", parameters);
    gatchor::ChainStore receiver(directory / "receiver.dat", parameters);
    assert(sender.load(now) == gatchor::ChainStoreError::none);
    assert(receiver.load(now) == gatchor::ChainStoreError::none);
    auto genesis = make_block(now - 100, {}, {});
    genesis.header.merkle_root = gatchor::calculate_genesis_root(parameters);
    assert(gatchor::mine_header(genesis.header, 100000));
    assert(sender.append(genesis, now) == gatchor::ChainStoreError::none);
    assert(receiver.append(genesis, now) == gatchor::ChainStoreError::none);

    gatchor::LoopbackBlockServer server;
    assert(server.start() == gatchor::NetworkError::none);
    assert(server.port() != 0);

    const auto block = make_block(now - 99, sender.tip_hash(), {'t', 'c', 'p'});
    assert(sender.append(block, now) == gatchor::ChainStoreError::none);

    gatchor::NetworkError receive_result = gatchor::NetworkError::socket_error;
    std::thread receiver_thread([&] {
        receive_result = server.receive_and_append(receiver, now);
    });
    const auto send_result =
        gatchor::send_block_loopback(server.port(), block);
    receiver_thread.join();
    assert(send_result == gatchor::NetworkError::none);
    assert(receive_result == gatchor::NetworkError::none);
    assert(receiver.size() == 2);
    assert(receiver.tip_hash() == sender.tip_hash());

    auto invalid_block = make_block(
        now - 98, receiver.tip_hash(), {'b', 'a', 'd'}, 1);
    invalid_block.transactions[0][0] ^= 1;
    receive_result = gatchor::NetworkError::socket_error;
    std::thread invalid_receiver_thread([&] {
        receive_result = server.receive_and_append(receiver, now);
    });
    const auto invalid_send_result =
        gatchor::send_block_loopback(server.port(), invalid_block);
    invalid_receiver_thread.join();
    assert(invalid_send_result == gatchor::NetworkError::invalid_block);
    assert(receive_result == gatchor::NetworkError::invalid_block);
    assert(receiver.size() == 2);

    gatchor::ChainStore restarted(directory / "receiver.dat", parameters);
    assert(restarted.load(now) == gatchor::ChainStoreError::none);
    assert(restarted.tip_hash() == sender.tip_hash());
    assert(restarted.blocks().back().transactions == block.transactions);

    std::error_code cleanup_error;
    std::filesystem::remove_all(directory, cleanup_error);
    assert(!cleanup_error);

    std::cout << "Loopback TCP block propagation and invalid block rejection passed\n";
    return 0;
}
