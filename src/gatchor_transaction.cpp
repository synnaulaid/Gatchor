#include "gatchor_transaction.hpp"

#include <algorithm>
#include <array>
#include <memory>

#include <openssl/evp.h>

namespace gatchor {
namespace {

using PKeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using DigestContextPtr =
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

constexpr std::array<uint8_t, 13> SIGNING_DOMAIN = {
    'G', 'A', 'T', 'C', 'H', 'O', 'R', '-', 'T', 'X', '-', 'V', '2'
};
constexpr size_t UNSIGNED_TRANSACTION_SIZE = 93;

void write_little_endian(uint32_t value, uint8_t* output) noexcept
{
    for (size_t i = 0; i < sizeof(value); ++i) {
        output[i] = static_cast<uint8_t>(value >> (i * 8));
    }
}

void write_little_endian(uint64_t value, uint8_t* output) noexcept
{
    for (size_t i = 0; i < sizeof(value); ++i) {
        output[i] = static_cast<uint8_t>(value >> (i * 8));
    }
}

uint32_t read_u32(const uint8_t* input) noexcept
{
    return static_cast<uint32_t>(input[0]) |
        (static_cast<uint32_t>(input[1]) << 8) |
        (static_cast<uint32_t>(input[2]) << 16) |
        (static_cast<uint32_t>(input[3]) << 24);
}

uint64_t read_u64(const uint8_t* input) noexcept
{
    uint64_t value = 0;
    for (size_t i = 0; i < sizeof(value); ++i) {
        value |= static_cast<uint64_t>(input[i]) << (i * 8);
    }
    return value;
}

void make_signing_message(
    const SignedTransaction& transaction,
    std::array<uint8_t, SIGNING_DOMAIN.size() + UNSIGNED_TRANSACTION_SIZE>& message) noexcept
{
    message.fill(0);
    std::copy(SIGNING_DOMAIN.begin(), SIGNING_DOMAIN.end(), message.begin());

    const size_t offset = SIGNING_DOMAIN.size();
    message[offset] = transaction.version;
    write_little_endian(transaction.chain_id, message.data() + offset + 1);
    write_little_endian(transaction.nonce, message.data() + offset + 5);
    write_little_endian(transaction.amount, message.data() + offset + 13);
    write_little_endian(transaction.fee, message.data() + offset + 21);
    std::copy(transaction.sender.begin(), transaction.sender.end(),
        message.begin() + static_cast<std::ptrdiff_t>(offset + 29));
    std::copy(transaction.recipient.begin(), transaction.recipient.end(),
        message.begin() + static_cast<std::ptrdiff_t>(offset + 61));
}

} // namespace

bool derive_public_key(
    std::span<const uint8_t> private_seed,
    Ed25519PublicKey& public_key) noexcept
{
    if (private_seed.size() != ED25519_KEY_SIZE) {
        return false;
    }
    PKeyPtr key(EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, private_seed.data(), private_seed.size()),
        EVP_PKEY_free);
    if (!key) {
        return false;
    }
    size_t public_key_size = public_key.size();
    return EVP_PKEY_get_raw_public_key(
        key.get(), public_key.data(), &public_key_size) == 1 &&
        public_key_size == public_key.size();
}

bool sign_transaction(
    SignedTransaction& transaction,
    std::span<const uint8_t> private_seed,
    uint32_t expected_chain_id) noexcept
{
    if (transaction.version != TRANSACTION_VERSION ||
        transaction.chain_id != expected_chain_id ||
        transaction.amount == 0 ||
        private_seed.size() != ED25519_KEY_SIZE) {
        return false;
    }

    PKeyPtr key(EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, private_seed.data(), private_seed.size()),
        EVP_PKEY_free);
    if (!key) {
        return false;
    }
    Ed25519PublicKey public_key{};
    size_t public_key_size = public_key.size();
    if (EVP_PKEY_get_raw_public_key(
            key.get(), public_key.data(), &public_key_size) != 1 ||
        public_key_size != public_key.size()) {
        return false;
    }
    transaction.sender = public_key;

    std::array<uint8_t, SIGNING_DOMAIN.size() + UNSIGNED_TRANSACTION_SIZE> message{};
    make_signing_message(transaction, message);
    DigestContextPtr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!context ||
        EVP_DigestSignInit(context.get(), nullptr, nullptr, nullptr, key.get()) != 1) {
        return false;
    }
    size_t signature_size = transaction.signature.size();
    return EVP_DigestSign(
        context.get(),
        transaction.signature.data(),
        &signature_size,
        message.data(),
        message.size()) == 1 &&
        signature_size == transaction.signature.size();
}

bool verify_transaction(
    const SignedTransaction& transaction,
    uint32_t expected_chain_id) noexcept
{
    if (transaction.version != TRANSACTION_VERSION ||
        transaction.chain_id != expected_chain_id ||
        transaction.amount == 0) {
        return false;
    }

    PKeyPtr key(EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519,
        nullptr,
        transaction.sender.data(),
        transaction.sender.size()),
        EVP_PKEY_free);
    if (!key) {
        return false;
    }
    std::array<uint8_t, SIGNING_DOMAIN.size() + UNSIGNED_TRANSACTION_SIZE> message{};
    make_signing_message(transaction, message);
    DigestContextPtr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!context ||
        EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, key.get()) != 1) {
        return false;
    }
    return EVP_DigestVerify(
        context.get(),
        transaction.signature.data(),
        transaction.signature.size(),
        message.data(),
        message.size()) == 1;
}

SerializedTransaction serialize_transaction(
    const SignedTransaction& transaction) noexcept
{
    SerializedTransaction bytes{};
    size_t offset = 0;
    bytes[offset++] = transaction.version;
    write_little_endian(transaction.chain_id, bytes.data() + offset);
    offset += sizeof(transaction.chain_id);
    write_little_endian(transaction.nonce, bytes.data() + offset);
    offset += sizeof(transaction.nonce);
    write_little_endian(transaction.amount, bytes.data() + offset);
    offset += sizeof(transaction.amount);
    write_little_endian(transaction.fee, bytes.data() + offset);
    offset += sizeof(transaction.fee);
    std::copy(transaction.sender.begin(), transaction.sender.end(), bytes.begin() + offset);
    offset += transaction.sender.size();
    std::copy(transaction.recipient.begin(), transaction.recipient.end(),
        bytes.begin() + offset);
    offset += transaction.recipient.size();
    std::copy(transaction.signature.begin(), transaction.signature.end(),
        bytes.begin() + offset);
    return bytes;
}

bool deserialize_transaction(
    std::span<const uint8_t> bytes,
    SignedTransaction& transaction) noexcept
{
    if (bytes.size() != SIGNED_TRANSACTION_SIZE) {
        return false;
    }

    size_t offset = 0;
    transaction.version = bytes[offset++];
    transaction.chain_id = read_u32(bytes.data() + offset);
    offset += sizeof(transaction.chain_id);
    transaction.nonce = read_u64(bytes.data() + offset);
    offset += sizeof(transaction.nonce);
    transaction.amount = read_u64(bytes.data() + offset);
    offset += sizeof(transaction.amount);
    transaction.fee = read_u64(bytes.data() + offset);
    offset += sizeof(transaction.fee);
    std::copy_n(bytes.data() + offset, transaction.sender.size(),
        transaction.sender.begin());
    offset += transaction.sender.size();
    std::copy_n(bytes.data() + offset, transaction.recipient.size(),
        transaction.recipient.begin());
    offset += transaction.recipient.size();
    std::copy_n(bytes.data() + offset, transaction.signature.size(),
        transaction.signature.begin());
    return true;
}

bool validate_serialized_transaction(
    std::span<const uint8_t> bytes,
    uint32_t expected_chain_id) noexcept
{
    SignedTransaction transaction;
    return deserialize_transaction(bytes, transaction) &&
        verify_transaction(transaction, expected_chain_id);
}

} // namespace gatchor
