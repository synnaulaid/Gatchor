# Gatchor256 Documentation

## Scope and security

Gatchor256 is an experimental, non-cryptographic hash implementation intended
for learning, data-integrity experiments, and performance benchmarks.

The construction has not received independent cryptanalysis or a security
audit. Statistical tests such as avalanche, bit distribution, and random
collision checks do not establish collision, preimage, or second-preimage
resistance.

Do not use Gatchor256 for authentication, signatures, password hashing,
commitments, Merkle trees, address derivation, blockchain consensus, or
Proof-of-Work. Use a standardized, vetted primitive such as SHA-256, SHA-3,
BLAKE2, or BLAKE3 for security-sensitive applications.

## Features

- Fixed 256-bit output
- Span-based API for hashing without input-sized allocations
- Experimental benchmark and integrity-check use cases

## Installation

```sh
git clone https://github.com/synnaulaid/Gatchor.git
cd Gatchor
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The default build is portable and does not enable CPU-specific instructions.
For local benchmarking only, `-march=native` can be enabled explicitly:

```sh
cmake -S . -B build -DGATCHOR_NATIVE_OPTIMIZATIONS=ON
cmake --build build --parallel
```

Do not distribute binaries built with native optimizations to machines with
unknown CPU capabilities.

## API

Use `Gatchor256::hash_into` when the binary digest is needed repeatedly:

```cpp
gatchor::Gatchor256::Digest digest{};
gatchor::Gatchor256::hash_into(data, digest);
```

Use `Gatchor256::hash` when a lowercase hexadecimal string is required.
Neither API provides authenticity, keying, domain separation, or canonical
serialization for protocol objects.

## Benchmarking

The `test_benchmark` executable measures repeated hashing of an 80-byte buffer
with a changing nonce. This is a performance benchmark only and is not a
production mining or consensus implementation.
