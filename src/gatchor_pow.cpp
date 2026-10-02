#include "gatchor_pow.hpp"

#include <utility>

namespace gatchor {
namespace {

template <typename Integer>
void write_little_endian(Integer value, uint8_t* output) noexcept
{
    for (size_t i = 0; i < sizeof(Integer); ++i) {
        output[i] = static_cast<uint8_t>(value >> (i * 8));
    }
}

Gatchor256::Digest hash_merkle_pair(
    const Gatchor256::Digest& left,
    const Gatchor256::Digest& right) noexcept
{
    std::array<uint8_t, 1 + 2 * Gatchor256::Digest{}.size()> input{};
    input[0] = 1;
    for (size_t i = 0; i < left.size(); ++i) {
        input[1 + i] = left[i];
        input[1 + left.size() + i] = right[i];
    }

    Gatchor256::Digest digest{};
    Gatchor256::hash_into(input, digest);
    return digest;
}

} // namespace

SerializedPowHeader serialize_header(const PowHeader& header) noexcept
{
    SerializedPowHeader bytes{};
    size_t offset = 0;

    write_little_endian(header.version, bytes.data() + offset);
    offset += sizeof(header.version);

    for (uint8_t byte : header.previous_hash) {
        bytes[offset++] = byte;
    }
    for (uint8_t byte : header.merkle_root) {
        bytes[offset++] = byte;
    }

    write_little_endian(header.timestamp, bytes.data() + offset);
    offset += sizeof(header.timestamp);
    write_little_endian(header.difficulty, bytes.data() + offset);
    offset += sizeof(header.difficulty);
    write_little_endian(header.nonce, bytes.data() + offset);

    return bytes;
}

Gatchor256::Digest hash_header(const PowHeader& header) noexcept
{
    const auto bytes = serialize_header(header);
    Gatchor256::Digest digest{};
    Gatchor256::hash_into(bytes, digest);
    return digest;
}

Gatchor256::Digest calculate_genesis_root(const PowChainParameters& parameters)
{
    std::vector<uint8_t> commitment;
    commitment.reserve(1 + sizeof(parameters.chain_id) +
        sizeof(parameters.initial_difficulty) + sizeof(uint64_t) +
        parameters.genesis_allocations.size() *
            (ED25519_KEY_SIZE + sizeof(uint64_t)));
    commitment.push_back(2);
    for (size_t i = 0; i < sizeof(parameters.chain_id); ++i) {
        commitment.push_back(
            static_cast<uint8_t>(parameters.chain_id >> (i * 8)));
    }
    commitment.push_back(
        static_cast<uint8_t>(parameters.initial_difficulty));
    commitment.push_back(
        static_cast<uint8_t>(parameters.initial_difficulty >> 8));
    const uint64_t allocation_count =
        static_cast<uint64_t>(parameters.genesis_allocations.size());
    for (size_t i = 0; i < sizeof(allocation_count); ++i) {
        commitment.push_back(static_cast<uint8_t>(allocation_count >> (i * 8)));
    }
    for (const auto& allocation : parameters.genesis_allocations) {
        commitment.insert(
            commitment.end(), allocation.account.begin(), allocation.account.end());
        for (size_t i = 0; i < sizeof(allocation.amount); ++i) {
            commitment.push_back(
                static_cast<uint8_t>(allocation.amount >> (i * 8)));
        }
    }

    Gatchor256::Digest root{};
    Gatchor256::hash_into(commitment, root);
    return root;
}

Gatchor256::Digest calculate_merkle_root(
    const std::vector<std::vector<uint8_t>>& transactions)
{
    if (transactions.empty()) {
        return {};
    }

    std::vector<Gatchor256::Digest> level;
    level.reserve(transactions.size());
    for (const auto& transaction : transactions) {
        std::vector<uint8_t> leaf_input;
        leaf_input.reserve(transaction.size() + 1);
        leaf_input.push_back(0);
        leaf_input.insert(leaf_input.end(), transaction.begin(), transaction.end());

        Gatchor256::Digest leaf{};
        Gatchor256::hash_into(leaf_input, leaf);
        level.push_back(leaf);
    }

    while (level.size() > 1) {
        std::vector<Gatchor256::Digest> next;
        next.reserve((level.size() + 1) / 2);
        for (size_t i = 0; i < level.size(); i += 2) {
            const auto& right = (i + 1 < level.size()) ? level[i + 1] : level[i];
            next.push_back(hash_merkle_pair(level[i], right));
        }
        level = std::move(next);
    }
    return level.front();
}

bool meets_target(const Gatchor256::Digest& digest, uint16_t leading_zero_bits) noexcept
{
    if (leading_zero_bits > 256) {
        return false;
    }

    const size_t whole_zero_bytes = leading_zero_bits / 8;
    for (size_t i = 0; i < whole_zero_bytes; ++i) {
        if (digest[i] != 0) {
            return false;
        }
    }

    const unsigned int remaining_zero_bits = leading_zero_bits % 8;
    if (remaining_zero_bits == 0) {
        return true;
    }

    const uint8_t mask = static_cast<uint8_t>(0xffU << (8 - remaining_zero_bits));
    return (digest[whole_zero_bytes] & mask) == 0;
}

bool verify_header(const PowHeader& header, Gatchor256::Digest* digest_out) noexcept
{
    if (header.difficulty > 256) {
        return false;
    }

    const Gatchor256::Digest digest = hash_header(header);
    if (digest_out != nullptr) {
        *digest_out = digest;
    }
    return meets_target(digest, header.difficulty);
}

namespace {

PowValidationError validate_common_fields(
    const PowHeader& header,
    uint64_t current_time) noexcept
{
    if (header.version != POW_PROTOCOL_VERSION) {
        return PowValidationError::unsupported_version;
    }
    if (header.difficulty == 0 || header.difficulty > 256) {
        return PowValidationError::invalid_difficulty;
    }
    if (header.timestamp > current_time &&
        header.timestamp - current_time > POW_MAX_FUTURE_SECONDS) {
        return PowValidationError::timestamp_too_far_in_future;
    }
    return PowValidationError::none;
}

} // namespace

PowValidationError validate_genesis_header(
    const PowHeader& header,
    uint64_t current_time,
    Gatchor256::Digest* digest_out) noexcept
{
    const auto common_error = validate_common_fields(header, current_time);
    if (common_error != PowValidationError::none) {
        return common_error;
    }
    if (header.previous_hash != Gatchor256::Digest{}) {
        return PowValidationError::invalid_genesis_parent;
    }
    if (!verify_header(header, digest_out)) {
        return PowValidationError::invalid_proof_of_work;
    }
    return PowValidationError::none;
}

PowValidationError validate_child_header(
    const PowHeader& header,
    const Gatchor256::Digest& parent_hash,
    uint64_t parent_timestamp,
    uint16_t parent_difficulty,
    uint64_t current_time,
    Gatchor256::Digest* digest_out) noexcept
{
    const auto common_error = validate_common_fields(header, current_time);
    if (common_error != PowValidationError::none) {
        return common_error;
    }
    if (header.previous_hash != parent_hash) {
        return PowValidationError::parent_hash_mismatch;
    }
    if (header.timestamp <= parent_timestamp) {
        return PowValidationError::timestamp_not_increasing;
    }
    if (header.difficulty != parent_difficulty) {
        return PowValidationError::invalid_difficulty;
    }
    if (!verify_header(header, digest_out)) {
        return PowValidationError::invalid_proof_of_work;
    }
    return PowValidationError::none;
}

namespace {

PowValidationError validate_block_transactions(
    const PowBlock& block,
    const PowChainParameters& parameters,
    LedgerState& ledger)
{
    if (block.transactions.empty()) {
        return PowValidationError::empty_block;
    }
    if (calculate_merkle_root(block.transactions) != block.header.merkle_root) {
        return PowValidationError::merkle_root_mismatch;
    }
    for (const auto& bytes : block.transactions) {
        SignedTransaction transaction;
        if (!deserialize_transaction(bytes, transaction) ||
            transaction.version != TRANSACTION_VERSION) {
            return PowValidationError::invalid_transaction_encoding;
        }
        if (transaction.chain_id != parameters.chain_id) {
            return PowValidationError::wrong_transaction_chain_id;
        }
        if (transaction.amount == 0 ||
            transaction.sender == Ed25519PublicKey{} ||
            transaction.recipient == Ed25519PublicKey{}) {
            return PowValidationError::invalid_transaction_fields;
        }
        if (!verify_transaction(transaction, parameters.chain_id)) {
            return PowValidationError::invalid_transaction_signature;
        }

        auto sender = ledger.find(transaction.sender);
        if (sender == ledger.end() ||
            transaction.nonce != sender->second.next_nonce) {
            return PowValidationError::invalid_transaction_nonce;
        }
        if (transaction.amount > UINT64_MAX - transaction.fee) {
            return PowValidationError::invalid_transaction_fields;
        }
        const uint64_t debit = transaction.amount + transaction.fee;
        if (sender->second.balance < debit) {
            return PowValidationError::insufficient_balance;
        }
        if (sender->second.next_nonce == UINT64_MAX) {
            return PowValidationError::invalid_transaction_nonce;
        }

        if (transaction.sender == transaction.recipient) {
            sender->second.balance -= debit;
            sender->second.balance += transaction.amount;
        } else {
            auto& recipient = ledger[transaction.recipient];
            if (recipient.balance > UINT64_MAX - transaction.amount) {
                return PowValidationError::balance_overflow;
            }
            sender->second.balance -= debit;
            recipient.balance += transaction.amount;
        }
        ++sender->second.next_nonce;
    }
    return PowValidationError::none;
}

} // namespace

PowValidationError validate_genesis_block(
    const PowBlock& block,
    uint64_t current_time,
    const PowChainParameters& parameters,
    Gatchor256::Digest* digest_out,
    LedgerState* ledger_out)
{
    if (parameters.chain_id != POW_CHAIN_ID ||
        parameters.initial_difficulty == 0 ||
        parameters.initial_difficulty > 256) {
        return PowValidationError::invalid_difficulty;
    }
    if (!block.transactions.empty()) {
        return PowValidationError::invalid_transaction_fields;
    }
    if (block.header.difficulty != parameters.initial_difficulty) {
        return PowValidationError::invalid_difficulty;
    }

    LedgerState ledger;
    Ed25519PublicKey previous_account{};
    bool has_previous_account = false;
    for (const auto& allocation : parameters.genesis_allocations) {
        if (allocation.account == Ed25519PublicKey{} ||
            allocation.amount == 0 ||
            (has_previous_account && !(previous_account < allocation.account))) {
            return PowValidationError::invalid_genesis_allocations;
        }
        ledger.emplace(allocation.account, LedgerAccount{allocation.amount, 0});
        previous_account = allocation.account;
        has_previous_account = true;
    }
    if (block.header.merkle_root != calculate_genesis_root(parameters)) {
        return PowValidationError::invalid_transaction_fields;
    }
    const auto error = validate_genesis_header(block.header, current_time, digest_out);
    if (error != PowValidationError::none) {
        return error;
    }
    if (ledger_out != nullptr) {
        *ledger_out = std::move(ledger);
    }
    return PowValidationError::none;
}

PowValidationError validate_child_block(
    const PowBlock& block,
    const Gatchor256::Digest& parent_hash,
    uint64_t parent_timestamp,
    uint16_t parent_difficulty,
    uint64_t current_time,
    const PowChainParameters& parameters,
    LedgerState& ledger,
    Gatchor256::Digest* digest_out)
{
    if (block.header.difficulty != parent_difficulty) {
        return PowValidationError::invalid_difficulty;
    }
    const auto header_error = validate_child_header(
        block.header,
        parent_hash,
        parent_timestamp,
        parent_difficulty,
        current_time,
        digest_out);
    if (header_error != PowValidationError::none) {
        return header_error;
    }

    LedgerState updated_ledger = ledger;
    const auto transaction_error =
        validate_block_transactions(block, parameters, updated_ledger);
    if (transaction_error != PowValidationError::none) {
        return transaction_error;
    }
    ledger = std::move(updated_ledger);
    return PowValidationError::none;
}

bool mine_header(
    PowHeader& header,
    uint64_t max_attempts,
    Gatchor256::Digest* digest_out) noexcept
{
    for (uint64_t attempt = 0; attempt < max_attempts; ++attempt) {
        if (verify_header(header, digest_out)) {
            return true;
        }

        if (attempt + 1 < max_attempts) {
            if (header.nonce == UINT64_MAX) {
                return false;
            }
            ++header.nonce;
        }
    }
    return false;
}

} // namespace gatchor
