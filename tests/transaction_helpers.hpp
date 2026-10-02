#pragma once

#include "gatchor_pow.hpp"

#include <vector>

namespace test_support {

inline gatchor::Ed25519PrivateSeed sender_seed()
{
    gatchor::Ed25519PrivateSeed seed{};
    for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<uint8_t>(0x31 + i);
    }
    return seed;
}

inline gatchor::PowChainParameters chain_parameters()
{
    gatchor::PowChainParameters parameters;
    gatchor::Ed25519PublicKey sender{};
    const auto seed = sender_seed();
    if (!gatchor::derive_public_key(seed, sender)) {
        return parameters;
    }
    parameters.genesis_allocations.push_back({sender, 1000000});
    return parameters;
}

inline std::vector<uint8_t> make_signed_transaction(
    const std::vector<uint8_t>& marker,
    uint64_t nonce = 0,
    uint64_t amount = 1,
    uint64_t fee = 1)
{
    gatchor::Ed25519PrivateSeed recipient_seed{};
    for (size_t i = 0; i < recipient_seed.size(); ++i) {
        const uint8_t marker_byte = marker.empty()
            ? 0
            : marker[i % marker.size()];
        recipient_seed[i] = static_cast<uint8_t>(0x91 + i) ^ marker_byte;
    }

    gatchor::SignedTransaction transaction;
    transaction.nonce = nonce;
    transaction.amount = amount;
    transaction.fee = fee;
    if (!gatchor::derive_public_key(recipient_seed, transaction.recipient)) {
        return {};
    }
    const auto seed = sender_seed();
    if (!gatchor::sign_transaction(transaction, seed)) {
        return {};
    }

    const auto bytes = gatchor::serialize_transaction(transaction);
    return {bytes.begin(), bytes.end()};
}

} // namespace test_support
