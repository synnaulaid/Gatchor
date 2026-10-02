#include "gatchor_transaction.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    gatchor::Ed25519PrivateSeed seed{};
    gatchor::Ed25519PrivateSeed wrong_seed{};
    for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<uint8_t>(i + 1);
        wrong_seed[i] = static_cast<uint8_t>(i + 2);
    }

    gatchor::Ed25519PublicKey recipient{};
    assert(gatchor::derive_public_key(seed, recipient));

    gatchor::SignedTransaction transaction;
    transaction.nonce = 7;
    transaction.amount = 123456;
    transaction.fee = 789;
    transaction.recipient = recipient;
    assert(gatchor::sign_transaction(transaction, seed));
    assert(gatchor::verify_transaction(transaction));

    const auto serialized = gatchor::serialize_transaction(transaction);
    assert(serialized.size() == gatchor::SIGNED_TRANSACTION_SIZE);
    assert(serialized[0] == gatchor::TRANSACTION_VERSION);
    assert((serialized[1] == 1 && serialized[2] == 0 &&
        serialized[3] == 0 && serialized[4] == 0));
    assert((serialized[5] == 7 && serialized[6] == 0 &&
        serialized[12] == 0));
    assert((serialized[13] == 0x40 && serialized[14] == 0xe2 &&
        serialized[15] == 0x01 && serialized[20] == 0));
    assert((serialized[21] == 0x15 && serialized[22] == 0x03 &&
        serialized[23] == 0 && serialized[28] == 0));
    gatchor::SignedTransaction decoded;
    assert(gatchor::deserialize_transaction(serialized, decoded));
    assert(decoded.version == transaction.version);
    assert(decoded.chain_id == transaction.chain_id);
    assert(decoded.nonce == transaction.nonce);
    assert(decoded.amount == transaction.amount);
    assert(decoded.fee == transaction.fee);
    assert(decoded.sender == transaction.sender);
    assert(decoded.recipient == transaction.recipient);
    assert(decoded.signature == transaction.signature);
    assert(gatchor::validate_serialized_transaction(serialized));

    auto changed_amount = decoded;
    ++changed_amount.amount;
    assert(!gatchor::verify_transaction(changed_amount));
    auto changed_fee = decoded;
    ++changed_fee.fee;
    assert(!gatchor::verify_transaction(changed_fee));

    auto wrong_chain = decoded;
    ++wrong_chain.chain_id;
    assert(!gatchor::verify_transaction(wrong_chain));
    assert(!gatchor::verify_transaction(decoded, gatchor::POW_CHAIN_ID + 1));

    auto zero_amount = decoded;
    zero_amount.amount = 0;
    assert(!gatchor::verify_transaction(zero_amount));

    auto wrong_sender = decoded;
    assert(gatchor::derive_public_key(wrong_seed, wrong_sender.sender));
    assert(!gatchor::verify_transaction(wrong_sender));

    assert(!gatchor::sign_transaction(
        transaction, std::span<const uint8_t>(seed.data(), seed.size() - 1)));
    assert(!gatchor::deserialize_transaction(
        std::span<const uint8_t>(serialized.data(), serialized.size() - 1), decoded));
    assert(!gatchor::validate_serialized_transaction(
        std::span<const uint8_t>(serialized.data(), serialized.size() - 1)));

    std::cout << "Canonical transaction encoding, Ed25519 signature, and tamper checks passed\n";
    return 0;
}
