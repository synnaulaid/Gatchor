# Gatchor256 Documentation

## Overview
**Gatchor256** is an experimental, non-cryptographic hashing library that provides
a simple and efficient interface for data-integrity experiments and benchmarks.

> ⚠️ **Security warning:** Gatchor256 has not received independent cryptanalysis
> or a security audit. Do not use it for authentication, signatures, password
> hashing, commitments, Merkle trees, address derivation, or blockchain
> consensus/Proof-of-Work. Use a standardized, vetted primitive such as
> SHA-256, SHA-3, BLAKE2, or BLAKE3 for security-sensitive applications.

---

## Features
- Fast and efficient hashing
- Simple and CPU-friendly
- Suitable for experiments and benchmarking only
- Open to community contributions

---

## Installation

```
# Clone the repository
git clone https://github.com/synnaulaid/Gatchor.git
cd Gatchor
# Build the library
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```
**Binary**
```
main - sample imput
interactive - interactive mode input
test_gatchor - tests for gatchor256
test_security - tests for security of gatchor256
test_benchmark - benchmark tests for gatchor256
```

## CPU mining integration

For a mining loop, keep the block header in a reusable byte buffer and update
only the nonce. Use `Gatchor256::hash_into(std::span<const uint8_t>, Digest&)`
to write the 32-byte digest directly. This avoids the input-sized padding
allocation and hexadecimal formatting on every attempt, keeping memory usage
constant and reducing per-hash overhead. The benchmark uses this exact
80-byte header/nonce path. The existing `hash(vector)` API remains available
for applications that need a hexadecimal string.

The default build is portable and does not enable CPU-specific instructions.
For local benchmarking only, `-march=native` can be enabled explicitly:

```
cmake -S . -B build -DGATCHOR_NATIVE_OPTIMIZATIONS=ON
cmake --build build -j$(nproc)
```

Do not use the native optimization option for binaries distributed to other
machines.

# Statistics
![Gatchor256 Benchmark](docs/img/stats.png)



Full Documentation can be found in [docs/docs.md](docs/docs.md).