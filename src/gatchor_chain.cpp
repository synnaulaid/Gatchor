#include "gatchor_chain.hpp"

#include <array>
#include <fstream>
#include <system_error>
#include <type_traits>
#include <utility>

namespace gatchor {
namespace {

constexpr std::array<uint8_t, 8> STORE_MAGIC = {
    'G', 'A', 'T', 'C', 'H', 'A', 'I', '3'
};
constexpr uint64_t MAX_STORE_SIZE = 64 * 1024 * 1024;
constexpr uint64_t MAX_BLOCK_COUNT = 100000;
constexpr uint32_t MAX_TRANSACTIONS_PER_BLOCK = 100000;
constexpr uint32_t MAX_TRANSACTION_SIZE = SIGNED_TRANSACTION_SIZE;
using ChainWork = std::array<uint64_t, 5>;

template <typename Integer>
bool write_little_endian(std::ostream& output, Integer value)
{
    for (size_t i = 0; i < sizeof(Integer); ++i) {
        output.put(static_cast<char>(value >> (i * 8)));
    }
    return static_cast<bool>(output);
}

template <typename Integer>
bool read_little_endian(std::istream& input, Integer& value)
{
    value = 0;
    for (size_t i = 0; i < sizeof(Integer); ++i) {
        const int byte = input.get();
        if (byte == std::char_traits<char>::eof()) {
            return false;
        }
        value |= static_cast<Integer>(static_cast<uint8_t>(byte)) << (i * 8);
    }
    return true;
}

bool write_bytes(std::ostream& output, const uint8_t* bytes, size_t size)
{
    if (size == 0) {
        return true;
    }
    output.write(reinterpret_cast<const char*>(bytes),
        static_cast<std::streamsize>(size));
    return static_cast<bool>(output);
}

bool read_bytes(std::istream& input, uint8_t* bytes, size_t size)
{
    if (size == 0) {
        return true;
    }
    input.read(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(size));
    return input.gcount() == static_cast<std::streamsize>(size);
}

bool write_block(std::ostream& output, const PowBlock& block)
{
    if (block.transactions.size() > MAX_TRANSACTIONS_PER_BLOCK) {
        return false;
    }

    const auto header = serialize_header(block.header);
    if (!write_bytes(output, header.data(), header.size()) ||
        !write_little_endian(output, static_cast<uint32_t>(block.transactions.size()))) {
        return false;
    }

    for (const auto& transaction : block.transactions) {
        if (transaction.size() > MAX_TRANSACTION_SIZE ||
            !write_little_endian(output, static_cast<uint32_t>(transaction.size())) ||
            !write_bytes(output, transaction.data(), transaction.size())) {
            return false;
        }
    }
    return true;
}

bool read_block(std::istream& input, PowBlock& block)
{
    SerializedPowHeader header_bytes{};
    if (!read_bytes(input, header_bytes.data(), header_bytes.size())) {
        return false;
    }

    size_t offset = 0;
    auto read_header_integer = [&header_bytes, &offset](auto& value) {
        using Integer = std::decay_t<decltype(value)>;
        value = 0;
        for (size_t i = 0; i < sizeof(Integer); ++i) {
            value |= static_cast<Integer>(header_bytes[offset++]) << (i * 8);
        }
    };
    read_header_integer(block.header.version);
    for (auto& byte : block.header.previous_hash) {
        byte = header_bytes[offset++];
    }
    for (auto& byte : block.header.merkle_root) {
        byte = header_bytes[offset++];
    }
    read_header_integer(block.header.timestamp);
    read_header_integer(block.header.difficulty);
    read_header_integer(block.header.nonce);

    uint32_t transaction_count = 0;
    if (!read_little_endian(input, transaction_count) ||
        transaction_count > MAX_TRANSACTIONS_PER_BLOCK) {
        return false;
    }
    block.transactions.clear();
    block.transactions.reserve(transaction_count);
    for (uint32_t i = 0; i < transaction_count; ++i) {
        uint32_t transaction_size = 0;
        if (!read_little_endian(input, transaction_size) ||
            transaction_size > MAX_TRANSACTION_SIZE) {
            return false;
        }
        std::vector<uint8_t> transaction(transaction_size);
        if (!read_bytes(input, transaction.data(), transaction.size())) {
            return false;
        }
        block.transactions.push_back(std::move(transaction));
    }
    return true;
}

bool validate_loaded_chain(
    const std::vector<PowBlock>& blocks,
    uint64_t current_time,
    const PowChainParameters& parameters,
    LedgerState* ledger_out = nullptr)
{
    if (blocks.empty()) {
        return false;
    }

    LedgerState ledger;
    if (validate_genesis_block(
            blocks.front(), current_time, parameters, nullptr, &ledger) !=
        PowValidationError::none) {
        return false;
    }
    for (size_t i = 1; i < blocks.size(); ++i) {
        const auto parent_hash = hash_header(blocks[i - 1].header);
        if (validate_child_block(
                blocks[i],
                parent_hash,
                blocks[i - 1].header.timestamp,
                blocks[i - 1].header.difficulty,
                current_time,
                parameters,
                ledger) != PowValidationError::none) {
            return false;
        }
    }
    if (ledger_out != nullptr) {
        *ledger_out = std::move(ledger);
    }
    return true;
}

ChainWork calculate_chain_work(const std::vector<PowBlock>& blocks) noexcept
{
    ChainWork work{};
    for (const auto& block : blocks) {
        const size_t bit = block.header.difficulty;
        size_t limb = bit / 64;
        uint64_t addend = uint64_t{1} << (bit % 64);
        while (addend != 0 && limb < work.size()) {
            const uint64_t previous = work[limb];
            work[limb] += addend;
            addend = work[limb] < previous ? 1 : 0;
            ++limb;
        }
    }
    return work;
}

int compare_work(const ChainWork& left, const ChainWork& right) noexcept
{
    for (size_t i = left.size(); i > 0; --i) {
        if (left[i - 1] < right[i - 1]) {
            return -1;
        }
        if (left[i - 1] > right[i - 1]) {
            return 1;
        }
    }
    return 0;
}

} // namespace

ChainStore::ChainStore(std::filesystem::path file_path, PowChainParameters parameters)
    : file_path_(std::move(file_path)), parameters_(std::move(parameters))
{
}

ChainStoreError ChainStore::load(uint64_t current_time)
{
    std::error_code filesystem_error;
    const bool exists = std::filesystem::exists(file_path_, filesystem_error);
    if (filesystem_error) {
        return ChainStoreError::io_error;
    }
    if (!exists) {
        blocks_.clear();
        ledger_.clear();
        return ChainStoreError::none;
    }

    const uintmax_t file_size = std::filesystem::file_size(file_path_, filesystem_error);
    if (filesystem_error) {
        return ChainStoreError::io_error;
    }
    if (file_size > MAX_STORE_SIZE) {
        return ChainStoreError::corrupt_store;
    }

    std::ifstream input(file_path_, std::ios::binary);
    if (!input) {
        return ChainStoreError::io_error;
    }

    std::array<uint8_t, STORE_MAGIC.size()> magic{};
    uint64_t block_count = 0;
    if (!read_bytes(input, magic.data(), magic.size()) ||
        magic != STORE_MAGIC ||
        !read_little_endian(input, block_count) ||
        block_count == 0 ||
        block_count > MAX_BLOCK_COUNT) {
        return ChainStoreError::corrupt_store;
    }

    std::vector<PowBlock> loaded;
    loaded.reserve(static_cast<size_t>(block_count));
    for (uint64_t i = 0; i < block_count; ++i) {
        PowBlock block;
        if (!read_block(input, block)) {
            return ChainStoreError::corrupt_store;
        }
        loaded.push_back(std::move(block));
    }
    LedgerState loaded_ledger;
    if (input.peek() != std::char_traits<char>::eof() ||
        !validate_loaded_chain(loaded, current_time, parameters_, &loaded_ledger)) {
        return ChainStoreError::corrupt_store;
    }

    blocks_ = std::move(loaded);
    ledger_ = std::move(loaded_ledger);
    return ChainStoreError::none;
}

ChainStoreError ChainStore::append(const PowBlock& block, uint64_t current_time)
{
    if (blocks_.size() >= MAX_BLOCK_COUNT) {
        return ChainStoreError::invalid_block;
    }

    PowValidationError validation = PowValidationError::none;
    LedgerState updated_ledger = ledger_;
    if (blocks_.empty()) {
        validation = validate_genesis_block(
            block, current_time, parameters_, nullptr, &updated_ledger);
    } else {
        const auto& parent = blocks_.back();
        validation = validate_child_block(
            block,
            hash_header(parent.header),
            parent.header.timestamp,
            parent.header.difficulty,
            current_time,
            parameters_,
            updated_ledger);
    }
    if (validation != PowValidationError::none) {
        return ChainStoreError::invalid_block;
    }

    auto updated = blocks_;
    updated.push_back(block);
    const auto persist_error = persist(updated);
    if (persist_error != ChainStoreError::none) {
        return persist_error;
    }
    blocks_ = std::move(updated);
    ledger_ = std::move(updated_ledger);
    return ChainStoreError::none;
}

ChainStoreError ChainStore::consider_chain(
    const std::vector<PowBlock>& candidate,
    uint64_t current_time)
{
    if (candidate.empty() || candidate.size() > MAX_BLOCK_COUNT) {
        return ChainStoreError::invalid_block;
    }

    LedgerState updated_ledger;
    if (!validate_loaded_chain(
            candidate, current_time, parameters_, &updated_ledger)) {
        return ChainStoreError::invalid_block;
    }
    if (!blocks_.empty() &&
        hash_header(candidate.front().header) != hash_header(blocks_.front().header)) {
        return ChainStoreError::invalid_block;
    }

    if (!blocks_.empty()) {
        const int work_order =
            compare_work(calculate_chain_work(candidate), calculate_chain_work(blocks_));
        if (work_order < 0) {
            return ChainStoreError::chain_not_better;
        }
        if (work_order == 0 &&
            !(hash_header(candidate.back().header) < hash_header(blocks_.back().header))) {
            return ChainStoreError::chain_not_better;
        }
    }

    const auto persist_error = persist(candidate);
    if (persist_error != ChainStoreError::none) {
        return persist_error;
    }
    blocks_ = candidate;
    ledger_ = std::move(updated_ledger);
    return ChainStoreError::none;
}

size_t ChainStore::size() const noexcept
{
    return blocks_.size();
}

bool ChainStore::empty() const noexcept
{
    return blocks_.empty();
}

const std::vector<PowBlock>& ChainStore::blocks() const noexcept
{
    return blocks_;
}

const LedgerState& ChainStore::ledger() const noexcept
{
    return ledger_;
}

Gatchor256::Digest ChainStore::tip_hash() const noexcept
{
    return blocks_.empty() ? Gatchor256::Digest{} : hash_header(blocks_.back().header);
}

const std::filesystem::path& ChainStore::file_path() const noexcept
{
    return file_path_;
}

ChainStoreError ChainStore::persist(const std::vector<PowBlock>& blocks) const
{
    if (blocks.empty() || blocks.size() > MAX_BLOCK_COUNT) {
        return ChainStoreError::invalid_block;
    }

    uint64_t serialized_size = STORE_MAGIC.size() + sizeof(uint64_t);
    const auto add_size = [&serialized_size](uint64_t size) {
        if (size > MAX_STORE_SIZE - serialized_size) {
            return false;
        }
        serialized_size += size;
        return true;
    };
    for (const auto& block : blocks) {
        if (block.transactions.size() > MAX_TRANSACTIONS_PER_BLOCK ||
            !add_size(POW_HEADER_SIZE + sizeof(uint32_t))) {
            return ChainStoreError::invalid_block;
        }
        for (const auto& transaction : block.transactions) {
            if (transaction.size() > MAX_TRANSACTION_SIZE ||
                !add_size(sizeof(uint32_t) + transaction.size())) {
                return ChainStoreError::invalid_block;
            }
        }
    }

    std::error_code filesystem_error;
    const auto parent = file_path_.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, filesystem_error)) {
        return ChainStoreError::io_error;
    }
    if (filesystem_error) {
        return ChainStoreError::io_error;
    }

    auto temporary_path = file_path_;
    temporary_path += ".tmp";
    {
        std::ofstream output(
            temporary_path,
            std::ios::binary | std::ios::trunc);
        if (!output) {
            return ChainStoreError::io_error;
        }
        if (!write_bytes(output, STORE_MAGIC.data(), STORE_MAGIC.size()) ||
            !write_little_endian(output, static_cast<uint64_t>(blocks.size()))) {
            output.close();
            std::filesystem::remove(temporary_path, filesystem_error);
            return ChainStoreError::io_error;
        }
        for (const auto& block : blocks) {
            if (!write_block(output, block)) {
                output.close();
                std::filesystem::remove(temporary_path, filesystem_error);
                return ChainStoreError::invalid_block;
            }
        }
        output.flush();
        if (!output) {
            output.close();
            std::filesystem::remove(temporary_path, filesystem_error);
            return ChainStoreError::io_error;
        }
    }

    const uintmax_t temporary_size =
        std::filesystem::file_size(temporary_path, filesystem_error);
    if (filesystem_error || temporary_size > MAX_STORE_SIZE) {
        std::error_code cleanup_error;
        std::filesystem::remove(temporary_path, cleanup_error);
        return filesystem_error
            ? ChainStoreError::io_error
            : ChainStoreError::invalid_block;
    }

    std::filesystem::rename(temporary_path, file_path_, filesystem_error);
    if (filesystem_error) {
        std::error_code cleanup_error;
        std::filesystem::remove(temporary_path, cleanup_error);
        return ChainStoreError::io_error;
    }
    return ChainStoreError::none;
}

} // namespace gatchor
