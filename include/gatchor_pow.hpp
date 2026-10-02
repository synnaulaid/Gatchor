#pragma once

#include "gatchor.hpp"
#include "gatchor_transaction.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace gatchor {

inline constexpr uint32_t POW_PROTOCOL_VERSION = 3;

struct GenesisAllocation {
    Ed25519PublicKey account{};
    uint64_t amount = 0;
};

struct PowChainParameters {
    uint32_t chain_id = POW_CHAIN_ID;
    uint16_t initial_difficulty = 8;
    std::vector<GenesisAllocation> genesis_allocations;
};

struct LedgerAccount {
    uint64_t balance = 0;
    uint64_t next_nonce = 0;
};

using LedgerState = std::map<Ed25519PublicKey, LedgerAccount>;

struct PowHeader {
    uint32_t version = POW_PROTOCOL_VERSION;
    Gatchor256::Digest previous_hash{};
    Gatchor256::Digest merkle_root{};
    uint64_t timestamp = 0;
    uint16_t difficulty = 0;
    uint64_t nonce = 0;
};

struct PowBlock {
    PowHeader header;
    std::vector<std::vector<uint8_t>> transactions;
};

inline constexpr size_t POW_HEADER_SIZE = 86;
inline constexpr uint64_t POW_MAX_FUTURE_SECONDS = 7200;
using SerializedPowHeader = std::array<uint8_t, POW_HEADER_SIZE>;

enum class PowValidationError {
    none,
    unsupported_version,
    invalid_difficulty,
    invalid_genesis_parent,
    parent_hash_mismatch,
    timestamp_not_increasing,
    timestamp_too_far_in_future,
    invalid_proof_of_work,
    empty_block,
    merkle_root_mismatch,
    invalid_transaction_encoding,
    wrong_transaction_chain_id,
    invalid_transaction_fields,
    invalid_transaction_signature,
    invalid_genesis_allocations,
    invalid_transaction_nonce,
    insufficient_balance,
    balance_overflow
};

SerializedPowHeader serialize_header(const PowHeader& header) noexcept;
Gatchor256::Digest hash_header(const PowHeader& header) noexcept;
Gatchor256::Digest calculate_genesis_root(
    const PowChainParameters& parameters);
Gatchor256::Digest calculate_merkle_root(
    const std::vector<std::vector<uint8_t>>& transactions);
bool meets_target(const Gatchor256::Digest& digest, uint16_t leading_zero_bits) noexcept;
bool verify_header(
    const PowHeader& header,
    Gatchor256::Digest* digest_out = nullptr) noexcept;
PowValidationError validate_genesis_header(
    const PowHeader& header,
    uint64_t current_time,
    Gatchor256::Digest* digest_out = nullptr) noexcept;
PowValidationError validate_child_header(
    const PowHeader& header,
    const Gatchor256::Digest& parent_hash,
    uint64_t parent_timestamp,
    uint16_t parent_difficulty,
    uint64_t current_time,
    Gatchor256::Digest* digest_out = nullptr) noexcept;
PowValidationError validate_genesis_block(
    const PowBlock& block,
    uint64_t current_time,
    const PowChainParameters& parameters = {},
    Gatchor256::Digest* digest_out = nullptr,
    LedgerState* ledger_out = nullptr);
PowValidationError validate_child_block(
    const PowBlock& block,
    const Gatchor256::Digest& parent_hash,
    uint64_t parent_timestamp,
    uint16_t parent_difficulty,
    uint64_t current_time,
    const PowChainParameters& parameters,
    LedgerState& ledger,
    Gatchor256::Digest* digest_out = nullptr);
bool mine_header(
    PowHeader& header,
    uint64_t max_attempts,
    Gatchor256::Digest* digest_out = nullptr) noexcept;

} // namespace gatchor
