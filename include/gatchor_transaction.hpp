#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace gatchor {

inline constexpr uint8_t TRANSACTION_VERSION = 2;
inline constexpr uint32_t POW_CHAIN_ID = 1;
inline constexpr size_t ED25519_KEY_SIZE = 32;
inline constexpr size_t ED25519_SIGNATURE_SIZE = 64;
inline constexpr size_t SIGNED_TRANSACTION_SIZE = 157;
static_assert(SIGNED_TRANSACTION_SIZE ==
    1 + sizeof(uint32_t) + 3 * sizeof(uint64_t) +
    2 * ED25519_KEY_SIZE + ED25519_SIGNATURE_SIZE);

using Ed25519PublicKey = std::array<uint8_t, ED25519_KEY_SIZE>;
using Ed25519PrivateSeed = std::array<uint8_t, ED25519_KEY_SIZE>;
using Ed25519Signature = std::array<uint8_t, ED25519_SIGNATURE_SIZE>;
using SerializedTransaction = std::array<uint8_t, SIGNED_TRANSACTION_SIZE>;

struct SignedTransaction {
    uint8_t version = TRANSACTION_VERSION;
    uint32_t chain_id = POW_CHAIN_ID;
    uint64_t nonce = 0;
    uint64_t amount = 0;
    uint64_t fee = 0;
    Ed25519PublicKey sender{};
    Ed25519PublicKey recipient{};
    Ed25519Signature signature{};
};

bool derive_public_key(
    std::span<const uint8_t> private_seed,
    Ed25519PublicKey& public_key) noexcept;
bool sign_transaction(
    SignedTransaction& transaction,
    std::span<const uint8_t> private_seed,
    uint32_t expected_chain_id = POW_CHAIN_ID) noexcept;
bool verify_transaction(
    const SignedTransaction& transaction,
    uint32_t expected_chain_id = POW_CHAIN_ID) noexcept;
SerializedTransaction serialize_transaction(
    const SignedTransaction& transaction) noexcept;
bool deserialize_transaction(
    std::span<const uint8_t> bytes,
    SignedTransaction& transaction) noexcept;
bool validate_serialized_transaction(
    std::span<const uint8_t> bytes,
    uint32_t expected_chain_id = POW_CHAIN_ID) noexcept;

} // namespace gatchor
