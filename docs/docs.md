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

The README's Gatchor256 v1 chart reports a sample run of the current
`test_security` and `test_benchmark` programs. It records the measured avalanche
average, bit-frequency range, random-collision smoke-test count, and 86-byte
header throughput. Results vary by input sample, CPU, and system load; they do
not establish cryptographic security or production mining performance.

## Experimental private-testnet scaffold

The repository also contains experimental Proof-of-Work headers, signed
transactions, a replayed account ledger, local chain snapshots, and a
127.0.0.1-only block propagation prototype. Protocol v3's transaction v2
encoding is fixed at 157 bytes and uses Ed25519 signatures. Genesis allocation
and account nonce state are deterministic; each transfer debits `amount + fee`,
credits the recipient with `amount`, and burns the fee. There is no block
reward or difficulty adjustment.

These components are useful only for local protocol experiments. The custom
Gatchor256 hash is used for header PoW and Merkle/genesis commitments despite
not being cryptographically reviewed; a valid Ed25519 transaction signature
does not make the chain consensus secure. Snapshots and networking are not
hardened for hostile environments. Do not deploy this scaffold with real
assets, expose its loopback prototype to an untrusted network, or treat passing
tests/statistical hash tests as a security audit.
