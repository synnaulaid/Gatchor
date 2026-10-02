#pragma once

#include "gatchor_pow.hpp"

#include <filesystem>
#include <vector>

namespace gatchor {

enum class ChainStoreError {
    none,
    io_error,
    corrupt_store,
    invalid_block,
    chain_not_better
};

class ChainStore {
public:
    explicit ChainStore(
        std::filesystem::path file_path,
        PowChainParameters parameters = {});

    ChainStoreError load(uint64_t current_time);
    ChainStoreError append(const PowBlock& block, uint64_t current_time);
    ChainStoreError consider_chain(
        const std::vector<PowBlock>& candidate,
        uint64_t current_time);

    size_t size() const noexcept;
    bool empty() const noexcept;
    const std::vector<PowBlock>& blocks() const noexcept;
    const LedgerState& ledger() const noexcept;
    Gatchor256::Digest tip_hash() const noexcept;
    const std::filesystem::path& file_path() const noexcept;

private:
    ChainStoreError persist(const std::vector<PowBlock>& blocks) const;

    std::filesystem::path file_path_;
    PowChainParameters parameters_;
    std::vector<PowBlock> blocks_;
    LedgerState ledger_;
};

} // namespace gatchor
