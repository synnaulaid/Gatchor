#include "gatchor_chain.hpp"
#include "transaction_helpers.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <utility>

namespace {

gatchor::PowBlock mine_block(
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
        ("gatchor-two-node-" + std::to_string(unique));
    assert(std::filesystem::create_directory(directory));
    const auto node_a_path = directory / "node-a.dat";
    const auto node_b_path = directory / "node-b.dat";
    constexpr uint64_t now = 1700002000;

    const auto parameters = test_support::chain_parameters();
    gatchor::ChainStore node_a(node_a_path, parameters);
    gatchor::ChainStore node_b(node_b_path, parameters);
    assert(node_a.load(now) == gatchor::ChainStoreError::none);
    assert(node_b.load(now) == gatchor::ChainStoreError::none);

    auto genesis = mine_block(now - 100, {}, {});
    genesis.header.merkle_root = gatchor::calculate_genesis_root(parameters);
    assert(gatchor::mine_header(genesis.header, 100000));
    assert(node_a.append(genesis, now) == gatchor::ChainStoreError::none);
    assert(node_b.append(genesis, now) == gatchor::ChainStoreError::none);
    assert(node_a.tip_hash() == node_b.tip_hash());

    const auto genesis_hash = node_a.tip_hash();
    const auto block_a = mine_block(now - 99, genesis_hash, {'n', 'o', 'd', 'e', 'a'});
    const auto block_b = mine_block(now - 99, genesis_hash, {'n', 'o', 'd', 'e', 'b'});
    assert(node_a.append(block_a, now) == gatchor::ChainStoreError::none);
    assert(node_b.append(block_b, now) == gatchor::ChainStoreError::none);
    assert(node_a.tip_hash() != node_b.tip_hash());

    const auto branch_a = node_a.blocks();
    const auto branch_b = node_b.blocks();
    const auto result_a = node_a.consider_chain(branch_b, now);
    const auto result_b = node_b.consider_chain(branch_a, now);
    assert(result_a == gatchor::ChainStoreError::none ||
        result_a == gatchor::ChainStoreError::chain_not_better);
    assert(result_b == gatchor::ChainStoreError::none ||
        result_b == gatchor::ChainStoreError::chain_not_better);
    assert(node_a.tip_hash() == node_b.tip_hash());

    auto& winner = node_a.tip_hash() < node_b.tip_hash() ? node_a : node_b;
    auto& lagging = node_a.tip_hash() < node_b.tip_hash() ? node_b : node_a;
    const auto winning_parent = winner.tip_hash();
    const auto next = mine_block(now - 98, winning_parent, {'m', 'o', 'r', 'e'}, 1);
    assert(winner.append(next, now) == gatchor::ChainStoreError::none);

    const auto stale_chain = lagging.blocks();
    assert(lagging.consider_chain(winner.blocks(), now) ==
        gatchor::ChainStoreError::none);
    assert(lagging.tip_hash() == winner.tip_hash());
    assert(lagging.consider_chain(stale_chain, now) ==
        gatchor::ChainStoreError::chain_not_better);

    auto invalid = mine_block(now - 97, winner.tip_hash(), {'v', 'a', 'l', 'i', 'd'}, 2);
    invalid.transactions[0][0] ^= 1;
    assert(lagging.append(invalid, now) == gatchor::ChainStoreError::invalid_block);
    assert(lagging.tip_hash() == winner.tip_hash());

    gatchor::ChainStore restarted_a(node_a_path, parameters);
    gatchor::ChainStore restarted_b(node_b_path, parameters);
    assert(restarted_a.load(now) == gatchor::ChainStoreError::none);
    assert(restarted_b.load(now) == gatchor::ChainStoreError::none);
    assert(restarted_a.tip_hash() == restarted_b.tip_hash());
    assert(restarted_a.size() == 3);
    assert(restarted_b.size() == 3);

    std::error_code cleanup_error;
    std::filesystem::remove_all(directory, cleanup_error);
    assert(!cleanup_error);

    std::cout << "Two-node propagation, fork convergence, invalid block rejection, and restart passed\n";
    return 0;
}
