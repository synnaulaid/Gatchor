#include "gatchor_chain.hpp"
#include "transaction_helpers.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

namespace {

gatchor::PowBlock make_block(
    uint64_t timestamp,
    uint16_t difficulty,
    const gatchor::Gatchor256::Digest& parent_hash,
    std::vector<uint8_t> transaction,
    uint64_t nonce = 0)
{
    gatchor::PowBlock block;
    block.header.version = gatchor::POW_PROTOCOL_VERSION;
    block.header.previous_hash = parent_hash;
    block.header.timestamp = timestamp;
    block.header.difficulty = difficulty;
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
    const auto path = std::filesystem::temp_directory_path() /
        ("gatchor-chain-test-" + std::to_string(unique) + ".dat");
    auto temporary_path = path;
    temporary_path += ".tmp";
    constexpr uint64_t now = 1700001000;

    const auto parameters = test_support::chain_parameters();
    gatchor::ChainStore store(path, parameters);
    assert(store.load(now) == gatchor::ChainStoreError::none);
    assert(store.empty());

    auto genesis = make_block(now - 100, 8, {}, {});
    genesis.header.merkle_root = gatchor::calculate_genesis_root(parameters);
    assert(gatchor::mine_header(genesis.header, 100000));
    assert(store.append(genesis, now) == gatchor::ChainStoreError::none);
    assert(store.size() == 1);
    const auto genesis_hash = gatchor::hash_header(genesis.header);

    const auto child = make_block(
        now - 99, 8, genesis_hash, {'c', 'h', 'i', 'l', 'd'});
    assert(store.append(child, now) == gatchor::ChainStoreError::none);
    assert(store.size() == 2);
    const auto child_hash = gatchor::hash_header(child.header);
    assert(store.tip_hash() == child_hash);
    const auto sender_account = parameters.genesis_allocations[0].account;
    assert(store.ledger().at(sender_account).balance == 999998);
    assert(store.ledger().at(sender_account).next_nonce == 1);

    gatchor::ChainStore restarted(path, parameters);
    assert(restarted.load(now) == gatchor::ChainStoreError::none);
    assert(restarted.size() == 2);
    assert(restarted.tip_hash() == child_hash);
    assert(restarted.ledger().at(sender_account).balance == 999998);
    assert(restarted.ledger().at(sender_account).next_nonce == 1);
    assert(restarted.blocks()[0].transactions == genesis.transactions);
    assert(restarted.blocks()[1].transactions == child.transactions);
    auto changed_parameters = parameters;
    ++changed_parameters.genesis_allocations[0].amount;
    gatchor::ChainStore wrong_configuration(path, changed_parameters);
    assert(wrong_configuration.load(now) == gatchor::ChainStoreError::corrupt_store);
    assert(wrong_configuration.empty());

    const auto fork = make_block(
        now - 98, 8, genesis_hash, {'f', 'o', 'r', 'k'});
    assert(restarted.append(fork, now) == gatchor::ChainStoreError::invalid_block);
    assert(restarted.size() == 2);
    assert(restarted.tip_hash() == child_hash);

    assert(restarted.consider_chain({genesis}, now) ==
        gatchor::ChainStoreError::chain_not_better);
    assert(restarted.size() == 2);

    const auto alternate_child = make_block(
        now - 99, 8, genesis_hash, {'a', 'l', 't'});
    const auto alternate_child_hash = gatchor::hash_header(alternate_child.header);
    const auto alternate_grandchild = make_block(
        now - 98, 8, alternate_child_hash, {'g', 'r', 'a', 'n', 'd'}, 1);
    const std::vector<gatchor::PowBlock> longer_fork = {
        genesis, alternate_child, alternate_grandchild
    };
    assert(restarted.consider_chain(longer_fork, now) ==
        gatchor::ChainStoreError::none);
    assert(restarted.size() == 3);
    assert(restarted.tip_hash() ==
        gatchor::hash_header(alternate_grandchild.header));
    assert(restarted.ledger().at(sender_account).balance == 999996);
    assert(restarted.ledger().at(sender_account).next_nonce == 2);

    const auto tied_child = make_block(
        now - 99, 8, genesis_hash, {'t', 'i', 'e'});
    const auto tied_child_hash = gatchor::hash_header(tied_child.header);
    const auto tied_grandchild = make_block(
        now - 98, 8, tied_child_hash, {'t', 'i', 'e', '2'}, 1);
    const std::vector<gatchor::PowBlock> tied_fork = {
        genesis, tied_child, tied_grandchild
    };
    const auto tied_tip_hash = gatchor::hash_header(tied_grandchild.header);
    const auto current_tip_hash = restarted.tip_hash();
    const auto tie_result = restarted.consider_chain(tied_fork, now);
    const auto selected_tip_hash =
        tied_tip_hash < current_tip_hash ? tied_tip_hash : current_tip_hash;
    if (tied_tip_hash < current_tip_hash) {
        assert(tie_result == gatchor::ChainStoreError::none);
        assert(restarted.tip_hash() == tied_tip_hash);
    } else {
        assert(tie_result == gatchor::ChainStoreError::chain_not_better);
        assert(restarted.tip_hash() == current_tip_hash);
    }

    gatchor::ChainStore reloaded_after_reorg(path, parameters);
    assert(reloaded_after_reorg.load(now) == gatchor::ChainStoreError::none);
    assert(reloaded_after_reorg.tip_hash() == selected_tip_hash);
    assert(reloaded_after_reorg.size() == 3);

    auto other_genesis = make_block(now - 101, 8, {}, {});
    other_genesis.header.merkle_root = gatchor::calculate_genesis_root(parameters);
    assert(gatchor::mine_header(other_genesis.header, 100000));
    assert(reloaded_after_reorg.consider_chain({other_genesis}, now) ==
        gatchor::ChainStoreError::invalid_block);

    const auto invalid_merkle = make_block(
        now - 97, 8, reloaded_after_reorg.tip_hash(), {'o', 'k'}, 2);
    auto tampered = invalid_merkle;
    tampered.transactions[0][0] ^= 1;
    assert(reloaded_after_reorg.append(tampered, now) ==
        gatchor::ChainStoreError::invalid_block);
    assert(reloaded_after_reorg.size() == 3);

    {
        std::ofstream corrupt(path, std::ios::binary | std::ios::app);
        assert(corrupt);
        corrupt.put('x');
        assert(corrupt);
    }
    gatchor::ChainStore corrupted(path, parameters);
    assert(corrupted.load(now) == gatchor::ChainStoreError::corrupt_store);
    assert(corrupted.empty());

    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    assert(!cleanup_error);
    std::filesystem::remove(temporary_path, cleanup_error);
    assert(!cleanup_error);

    std::cout << "Chain store append, reload, fork choice, and corruption checks passed\n";
    return 0;
}
