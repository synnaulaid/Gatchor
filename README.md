# Gatchor
Cryptographic Hash (Experimental)

gatchor hash is not published yet, but here are some preliminary results from our internal testing:

# Training Data & Tests 
```
➜  Gatchor git:(master) ✗ python3 test/train.py
=== Gatchor-256 Full Test Suite ===

Bit distribution test:
Total bits: 2560000, Ones: 1280800, Zeros: 1279200
Percentage of ones: 50.03% (target ~50%)

Collision test:
Samples: 50000, Collisions found: 0

Avalanche test:
Original input: hello
Average differing bits after 1-bit flip: 128.38 / 256 (~50% target)

Benchmark speed (5000 samples, length 32 chars):
Gatchor-256 time: 0.858 s
SHA-256 time:    0.044 s
Relative speed: Gatchor/ SHA-256 = 19.38

Throughput benchmark (bytes/sec)
 size 1024 bytes: gatchor 911108.89 B/s, sha    125583839.06 B/s
 size 1048576 bytes: gatchor 974512.77 B/s, sha    344706909.04 B/s
 size 10485760 bytes: gatchor 929401.58 B/s, sha    285862160.46 B/s
```

Targeting 256-bit output, Gatchor-256 shows promising randomness and collision resistance in our tests. However, it is significantly slower than SHA-256, which is expected given its experimental nature. We will continue to optimize the algorithm and conduct more extensive testing before any public release.

and complexity and security depend on the design of the gatchor algorithm so it needs to be improved again and again, and we will keep testing it with more data and more tests, and we will keep improving it until we are satisfied with the results. 