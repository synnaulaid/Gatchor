#include "gatchor_pow.hpp"
#include "transaction_helpers.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

int main()
{
    gatchor::PowHeader header;
    header.version = 0x01020304;
    for (size_t i = 0; i < header.previous_hash.size(); ++i) {
        header.previous_hash[i] = static_cast<uint8_t>(i);
        header.merkle_root[i] = static_cast<uint8_t>(0x80 + i);
    }
    header.timestamp = 0x0102030405060708ULL;
    header.difficulty = 9;
    header.nonce = 0x1112131415161718ULL;

    const auto bytes = gatchor::serialize_header(header);
    const gatchor::SerializedPowHeader expected_bytes = {
        0x04, 0x03, 0x02, 0x01,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
        0x09, 0x00,
        0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11
    };
    assert(bytes == expected_bytes);

    gatchor::Gatchor256::Digest digest{};
    assert(gatchor::meets_target(digest, 256));
    assert(!gatchor::meets_target(digest, 257));
    digest[0] = 0x7f;
    assert(gatchor::meets_target(digest, 1));
    digest[0] = 0x80;
    assert(!gatchor::meets_target(digest, 1));

    header.difficulty = 10;
    header.nonce = 0;
    gatchor::Gatchor256::Digest mined_digest{};
    assert(gatchor::mine_header(header, 100000, &mined_digest));
    assert(gatchor::verify_header(header));
    gatchor::Gatchor256::Digest verified_digest{};
    assert(gatchor::verify_header(header, &verified_digest));
    assert(mined_digest == verified_digest);

    header.difficulty = 257;
    assert(!gatchor::verify_header(header));

    header.difficulty = 257;
    header.nonce = std::numeric_limits<uint64_t>::max();
    assert(!gatchor::mine_header(header, 2));
    assert(header.nonce == std::numeric_limits<uint64_t>::max());

    gatchor::PowHeader genesis;
    genesis.version = gatchor::POW_PROTOCOL_VERSION;
    genesis.timestamp = 1700000000;
    genesis.difficulty = 8;
    const auto parameters = test_support::chain_parameters();
    genesis.merkle_root = gatchor::calculate_genesis_root(parameters);
    gatchor::PowBlock genesis_block;
    genesis_block.header = genesis;
    assert(gatchor::mine_header(genesis_block.header, 100000));
    gatchor::Gatchor256::Digest genesis_hash{};
    gatchor::LedgerState genesis_ledger;
    assert(gatchor::validate_genesis_block(
        genesis_block, 1700000100, parameters, &genesis_hash, &genesis_ledger) ==
        gatchor::PowValidationError::none);
    assert(genesis_hash == gatchor::hash_header(genesis_block.header));
    auto mismatched_parameters = parameters;
    ++mismatched_parameters.genesis_allocations[0].amount;
    assert(gatchor::validate_genesis_block(
        genesis_block, 1700000100, mismatched_parameters) ==
        gatchor::PowValidationError::invalid_transaction_fields);
    auto duplicate_allocations = parameters;
    duplicate_allocations.genesis_allocations.push_back(
        duplicate_allocations.genesis_allocations.front());
    assert(gatchor::validate_genesis_block(
        genesis_block, 1700000100, duplicate_allocations) ==
        gatchor::PowValidationError::invalid_genesis_allocations);

    gatchor::PowBlock child_block;
    child_block.transactions.push_back(
        test_support::make_signed_transaction({'c', 'h', 'i', 'l', 'd'}, 0));
    child_block.header.version = gatchor::POW_PROTOCOL_VERSION;
    child_block.header.previous_hash = genesis_hash;
    child_block.header.timestamp = genesis_block.header.timestamp + 1;
    child_block.header.difficulty = genesis_block.header.difficulty;
    child_block.header.merkle_root = gatchor::calculate_merkle_root(
        child_block.transactions);
    assert(gatchor::mine_header(child_block.header, 100000));
    assert(gatchor::validate_child_header(
        child_block.header, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100) ==
        gatchor::PowValidationError::none);
    auto child_ledger = genesis_ledger;
    assert(gatchor::validate_child_block(
        child_block, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100, parameters, child_ledger) ==
        gatchor::PowValidationError::none);
    gatchor::SignedTransaction decoded_child_transaction;
    assert(gatchor::deserialize_transaction(
        child_block.transactions[0], decoded_child_transaction));
    const auto initial_sender = parameters.genesis_allocations[0].account;
    assert(child_ledger.at(initial_sender).balance == 999998);
    assert(child_ledger.at(initial_sender).next_nonce == 1);
    assert(child_ledger.at(decoded_child_transaction.recipient).balance == 1);

    const auto validate_invalid_child = [&](const gatchor::PowBlock& candidate) {
        auto ledger = genesis_ledger;
        return gatchor::validate_child_block(
            candidate, genesis_hash, genesis_block.header.timestamp,
            genesis_block.header.difficulty, 1700000100, parameters, ledger);
    };

    gatchor::PowBlock invalid_child = child_block;
    invalid_child.transactions[0] =
        test_support::make_signed_transaction({'n', 'o', 'n', 'c', 'e'}, 1);
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::invalid_transaction_nonce);
    auto unchanged_ledger = genesis_ledger;
    assert(gatchor::validate_child_block(
        invalid_child, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100, parameters,
        unchanged_ledger) ==
        gatchor::PowValidationError::invalid_transaction_nonce);
    assert(unchanged_ledger.at(initial_sender).balance == 1000000);
    assert(unchanged_ledger.at(initial_sender).next_nonce == 0);

    invalid_child = child_block;
    invalid_child.transactions[0] =
        test_support::make_signed_transaction(
            {'o', 'v', 'e', 'r', 's', 'p', 'e', 'n', 'd'}, 0, 1000001, 0);
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::insufficient_balance);

    invalid_child = child_block;
    invalid_child.transactions[0] = test_support::make_signed_transaction(
        {'f', 'e', 'e', '-', 'o', 'v', 'e', 'r', 'f', 'l', 'o', 'w'},
        0, UINT64_MAX, 1);
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::invalid_transaction_fields);

    auto overflow_parameters = parameters;
    gatchor::SignedTransaction overflow_transaction;
    assert(gatchor::deserialize_transaction(
        child_block.transactions[0], overflow_transaction));
    assert(overflow_transaction.recipient != initial_sender);
    overflow_parameters.genesis_allocations.push_back(
        {overflow_transaction.recipient, std::numeric_limits<uint64_t>::max()});
    std::sort(
        overflow_parameters.genesis_allocations.begin(),
        overflow_parameters.genesis_allocations.end(),
        [](const auto& left, const auto& right) {
            return left.account < right.account;
        });
    auto overflow_genesis = genesis_block;
    overflow_genesis.header.merkle_root =
        gatchor::calculate_genesis_root(overflow_parameters);
    assert(gatchor::mine_header(overflow_genesis.header, 100000));
    gatchor::LedgerState overflow_ledger;
    assert(gatchor::validate_genesis_block(
        overflow_genesis, 1700000100, overflow_parameters,
        nullptr, &overflow_ledger) == gatchor::PowValidationError::none);
    const auto overflow_genesis_hash =
        gatchor::hash_header(overflow_genesis.header);
    auto overflow_child = child_block;
    overflow_child.header.previous_hash = overflow_genesis_hash;
    overflow_child.header.timestamp = overflow_genesis.header.timestamp + 1;
    overflow_child.header.merkle_root =
        gatchor::calculate_merkle_root(overflow_child.transactions);
    assert(gatchor::mine_header(overflow_child.header, 100000));
    const auto overflow_error = gatchor::validate_child_block(
        overflow_child, overflow_genesis_hash,
        overflow_genesis.header.timestamp, overflow_genesis.header.difficulty,
        1700000100, overflow_parameters, overflow_ledger);
    assert(overflow_error == gatchor::PowValidationError::balance_overflow);
    assert(overflow_ledger.at(overflow_transaction.recipient).balance ==
        std::numeric_limits<uint64_t>::max());
    assert(overflow_ledger.at(initial_sender).balance == 1000000);

    invalid_child = child_block;
    invalid_child.header.previous_hash[0] ^= 1;
    assert(gatchor::validate_child_header(
        invalid_child.header, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100) ==
        gatchor::PowValidationError::parent_hash_mismatch);
    invalid_child.header = child_block.header;
    invalid_child.header.timestamp = genesis_block.header.timestamp;
    assert(gatchor::validate_child_header(
        invalid_child.header, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100) ==
        gatchor::PowValidationError::timestamp_not_increasing);
    invalid_child.header = child_block.header;
    invalid_child.header.difficulty++;
    assert(gatchor::validate_child_header(
        invalid_child.header, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100) ==
        gatchor::PowValidationError::invalid_difficulty);
    invalid_child.header = child_block.header;
    invalid_child.header.timestamp =
        1700000100 + gatchor::POW_MAX_FUTURE_SECONDS + 1;
    assert(gatchor::validate_child_header(
        invalid_child.header, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100) ==
        gatchor::PowValidationError::timestamp_too_far_in_future);
    invalid_child.header = child_block.header;
    invalid_child.header.version++;
    assert(gatchor::validate_child_header(
        invalid_child.header, genesis_hash, genesis_block.header.timestamp,
        genesis_block.header.difficulty, 1700000100) ==
        gatchor::PowValidationError::unsupported_version);
    invalid_child = child_block;
    invalid_child.transactions.push_back(
        test_support::make_signed_transaction({'r', 'o', 'o', 't'}));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::merkle_root_mismatch);
    invalid_child = child_block;
    invalid_child.transactions[0][0] ^= 1;
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::invalid_transaction_encoding);
    invalid_child = child_block;
    gatchor::SignedTransaction wrong_chain_transaction;
    assert(gatchor::deserialize_transaction(
        invalid_child.transactions[0], wrong_chain_transaction));
    ++wrong_chain_transaction.chain_id;
    const auto wrong_chain_bytes =
        gatchor::serialize_transaction(wrong_chain_transaction);
    invalid_child.transactions[0].assign(
        wrong_chain_bytes.begin(), wrong_chain_bytes.end());
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::wrong_transaction_chain_id);
    invalid_child = child_block;
    invalid_child.transactions[0].back() ^= 1;
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::invalid_transaction_signature);
    invalid_child = child_block;
    gatchor::SignedTransaction zero_amount_transaction;
    assert(gatchor::deserialize_transaction(
        invalid_child.transactions[0], zero_amount_transaction));
    zero_amount_transaction.amount = 0;
    const auto zero_amount_bytes =
        gatchor::serialize_transaction(zero_amount_transaction);
    invalid_child.transactions[0].assign(
        zero_amount_bytes.begin(), zero_amount_bytes.end());
    invalid_child.header.merkle_root =
        gatchor::calculate_merkle_root(invalid_child.transactions);
    assert(gatchor::mine_header(invalid_child.header, 100000));
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::invalid_transaction_fields);
    invalid_child = child_block;
    invalid_child.transactions.clear();
    assert(validate_invalid_child(invalid_child) ==
        gatchor::PowValidationError::empty_block);

    std::cout << "Proof-of-work serialization, target, mining, and verification passed\n";
    return 0;
}
