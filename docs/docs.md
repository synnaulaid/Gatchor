#Gatchor256 Documentation

## Overview
**Gatchor256** is a hashing library that provides a simple and efficient interface for hashing data using the **Gatchor256 algorithm**.
This library is designed to be **fast, reliable, and suitable for a wide range of applications**, ranging from data integrity checks to cryptographic operations.

> ⚠️ **Note:** Gatchor256 is currently under active development. Contributions and feedback are highly appreciated!

---

## Features
- Fast and efficient hashing
- Simple and CPU-friendly
- Suitable for both small and large datasets
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

# Statistics
![Gatchor256 Benchmark](img/stats.png)

```
➜  build cmake ..
make -j$(nproc)
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/syn/Documents/Project/algo/gatchor/build
[ 16%] Built target gatchor
[ 33%] Built target main
[ 50%] Built target test_gatchor
[ 58%] Building CXX object CMakeFiles/test_security.dir/tests/test_security.cpp.o
[ 75%] Built target interactive
[ 83%] Building CXX object CMakeFiles/test_benchmark.dir/tests/test_benchmark.cpp.o
[ 91%] Linking CXX executable test_benchmark
[ 91%] Built target test_benchmark
[100%] Linking CXX executable test_security
[100%] Built target test_security
➜  build ./test_security
Avalanche test for input: 'hello world'
Bit flipped at byte 0 → 149/256 bits changed
Bit flipped at byte 1 → 192/256 bits changed
Bit flipped at byte 2 → 176/256 bits changed
Bit flipped at byte 3 → 168/256 bits changed
Bit flipped at byte 4 → 185/256 bits changed
Bit flipped at byte 5 → 177/256 bits changed
Bit flipped at byte 6 → 164/256 bits changed
Bit flipped at byte 7 → 167/256 bits changed
Bit flipped at byte 8 → 161/256 bits changed
Bit flipped at byte 9 → 182/256 bits changed
Bit flipped at byte 10 → 180/256 bits changed

Random collision test (5000 inputs)...
Collisions found: 0

Bit distribution test (1000 random inputs)...
Bit 0: 480/1000 (48%)
Bit 1: 430/1000 (43%)
Bit 2: 430/1000 (43%)
Bit 3: 137/1000 (13.7%)
Bit 4: 635/1000 (63.5%)
Bit 5: 1000/1000 (100%)
Bit 6: 365/1000 (36.5%)
Bit 7: 0/1000 (0%)
Bit 8: 517/1000 (51.7%)
Bit 9: 440/1000 (44%)
Bit 10: 434/1000 (43.4%)
Bit 11: 130/1000 (13%)
Bit 12: 608/1000 (60.8%)
Bit 13: 1000/1000 (100%)
Bit 14: 392/1000 (39.2%)
Bit 15: 0/1000 (0%)
Bit 16: 502/1000 (50.2%)
Bit 17: 473/1000 (47.3%)
Bit 18: 448/1000 (44.8%)
Bit 19: 129/1000 (12.9%)
Bit 20: 608/1000 (60.8%)
Bit 21: 1000/1000 (100%)
Bit 22: 392/1000 (39.2%)
Bit 23: 0/1000 (0%)
Bit 24: 510/1000 (51%)
Bit 25: 411/1000 (41.1%)
Bit 26: 431/1000 (43.1%)
Bit 27: 139/1000 (13.9%)
Bit 28: 632/1000 (63.2%)
Bit 29: 1000/1000 (100%)
Bit 30: 368/1000 (36.8%)
Bit 31: 0/1000 (0%)
Bit 32: 471/1000 (47.1%)
Bit 33: 423/1000 (42.3%)
Bit 34: 468/1000 (46.8%)
Bit 35: 127/1000 (12.7%)
Bit 36: 638/1000 (63.8%)
Bit 37: 1000/1000 (100%)
Bit 38: 362/1000 (36.2%)
Bit 39: 0/1000 (0%)
Bit 40: 498/1000 (49.8%)
Bit 41: 418/1000 (41.8%)
Bit 42: 418/1000 (41.8%)
Bit 43: 135/1000 (13.5%)
Bit 44: 648/1000 (64.8%)
Bit 45: 1000/1000 (100%)
Bit 46: 352/1000 (35.2%)
Bit 47: 0/1000 (0%)
Bit 48: 488/1000 (48.8%)
Bit 49: 414/1000 (41.4%)
Bit 50: 445/1000 (44.5%)
Bit 51: 122/1000 (12.2%)
Bit 52: 611/1000 (61.1%)
Bit 53: 1000/1000 (100%)
Bit 54: 389/1000 (38.9%)
Bit 55: 0/1000 (0%)
Bit 56: 485/1000 (48.5%)
Bit 57: 429/1000 (42.9%)
Bit 58: 446/1000 (44.6%)
Bit 59: 124/1000 (12.4%)
Bit 60: 617/1000 (61.7%)
Bit 61: 1000/1000 (100%)
Bit 62: 383/1000 (38.3%)
Bit 63: 0/1000 (0%)
Bit 64: 495/1000 (49.5%)
Bit 65: 454/1000 (45.4%)
Bit 66: 442/1000 (44.2%)
Bit 67: 121/1000 (12.1%)
Bit 68: 615/1000 (61.5%)
Bit 69: 1000/1000 (100%)
Bit 70: 385/1000 (38.5%)
Bit 71: 0/1000 (0%)
Bit 72: 498/1000 (49.8%)
Bit 73: 453/1000 (45.3%)
Bit 74: 419/1000 (41.9%)
Bit 75: 134/1000 (13.4%)
Bit 76: 630/1000 (63%)
Bit 77: 1000/1000 (100%)
Bit 78: 370/1000 (37%)
Bit 79: 0/1000 (0%)
Bit 80: 497/1000 (49.7%)
Bit 81: 421/1000 (42.1%)
Bit 82: 433/1000 (43.3%)
Bit 83: 131/1000 (13.1%)
Bit 84: 632/1000 (63.2%)
Bit 85: 1000/1000 (100%)
Bit 86: 368/1000 (36.8%)
Bit 87: 0/1000 (0%)
Bit 88: 512/1000 (51.2%)
Bit 89: 411/1000 (41.1%)
Bit 90: 432/1000 (43.2%)
Bit 91: 143/1000 (14.3%)
Bit 92: 635/1000 (63.5%)
Bit 93: 1000/1000 (100%)
Bit 94: 365/1000 (36.5%)
Bit 95: 0/1000 (0%)
Bit 96: 500/1000 (50%)
Bit 97: 446/1000 (44.6%)
Bit 98: 454/1000 (45.4%)
Bit 99: 117/1000 (11.7%)
Bit 100: 625/1000 (62.5%)
Bit 101: 1000/1000 (100%)
Bit 102: 375/1000 (37.5%)
Bit 103: 0/1000 (0%)
Bit 104: 528/1000 (52.8%)
Bit 105: 444/1000 (44.4%)
Bit 106: 421/1000 (42.1%)
Bit 107: 126/1000 (12.6%)
Bit 108: 610/1000 (61%)
Bit 109: 1000/1000 (100%)
Bit 110: 390/1000 (39%)
Bit 111: 0/1000 (0%)
Bit 112: 503/1000 (50.3%)
Bit 113: 430/1000 (43%)
Bit 114: 449/1000 (44.9%)
Bit 115: 123/1000 (12.3%)
Bit 116: 625/1000 (62.5%)
Bit 117: 1000/1000 (100%)
Bit 118: 375/1000 (37.5%)
Bit 119: 0/1000 (0%)
Bit 120: 492/1000 (49.2%)
Bit 121: 468/1000 (46.8%)
Bit 122: 454/1000 (45.4%)
Bit 123: 117/1000 (11.7%)
Bit 124: 626/1000 (62.6%)
Bit 125: 1000/1000 (100%)
Bit 126: 374/1000 (37.4%)
Bit 127: 0/1000 (0%)
Bit 128: 522/1000 (52.2%)
Bit 129: 445/1000 (44.5%)
Bit 130: 407/1000 (40.7%)
Bit 131: 123/1000 (12.3%)
Bit 132: 652/1000 (65.2%)
Bit 133: 1000/1000 (100%)
Bit 134: 348/1000 (34.8%)
Bit 135: 0/1000 (0%)
Bit 136: 504/1000 (50.4%)
Bit 137: 447/1000 (44.7%)
Bit 138: 444/1000 (44.4%)
Bit 139: 124/1000 (12.4%)
Bit 140: 603/1000 (60.3%)
Bit 141: 1000/1000 (100%)
Bit 142: 397/1000 (39.7%)
Bit 143: 0/1000 (0%)
Bit 144: 526/1000 (52.6%)
Bit 145: 427/1000 (42.7%)
Bit 146: 450/1000 (45%)
Bit 147: 128/1000 (12.8%)
Bit 148: 591/1000 (59.1%)
Bit 149: 1000/1000 (100%)
Bit 150: 409/1000 (40.9%)
Bit 151: 0/1000 (0%)
Bit 152: 485/1000 (48.5%)
Bit 153: 449/1000 (44.9%)
Bit 154: 457/1000 (45.7%)
Bit 155: 117/1000 (11.7%)
Bit 156: 617/1000 (61.7%)
Bit 157: 1000/1000 (100%)
Bit 158: 383/1000 (38.3%)
Bit 159: 0/1000 (0%)
Bit 160: 512/1000 (51.2%)
Bit 161: 438/1000 (43.8%)
Bit 162: 415/1000 (41.5%)
Bit 163: 125/1000 (12.5%)
Bit 164: 635/1000 (63.5%)
Bit 165: 1000/1000 (100%)
Bit 166: 365/1000 (36.5%)
Bit 167: 0/1000 (0%)
Bit 168: 492/1000 (49.2%)
Bit 169: 453/1000 (45.3%)
Bit 170: 454/1000 (45.4%)
Bit 171: 113/1000 (11.3%)
Bit 172: 626/1000 (62.6%)
Bit 173: 1000/1000 (100%)
Bit 174: 374/1000 (37.4%)
Bit 175: 0/1000 (0%)
Bit 176: 505/1000 (50.5%)
Bit 177: 423/1000 (42.3%)
Bit 178: 441/1000 (44.1%)
Bit 179: 127/1000 (12.7%)
Bit 180: 637/1000 (63.7%)
Bit 181: 1000/1000 (100%)
Bit 182: 363/1000 (36.3%)
Bit 183: 0/1000 (0%)
Bit 184: 497/1000 (49.7%)
Bit 185: 433/1000 (43.3%)
Bit 186: 430/1000 (43%)
Bit 187: 116/1000 (11.6%)
Bit 188: 635/1000 (63.5%)
Bit 189: 1000/1000 (100%)
Bit 190: 365/1000 (36.5%)
Bit 191: 0/1000 (0%)
Bit 192: 509/1000 (50.9%)
Bit 193: 419/1000 (41.9%)
Bit 194: 423/1000 (42.3%)
Bit 195: 141/1000 (14.1%)
Bit 196: 649/1000 (64.9%)
Bit 197: 1000/1000 (100%)
Bit 198: 351/1000 (35.1%)
Bit 199: 0/1000 (0%)
Bit 200: 500/1000 (50%)
Bit 201: 462/1000 (46.2%)
Bit 202: 426/1000 (42.6%)
Bit 203: 119/1000 (11.9%)
Bit 204: 625/1000 (62.5%)
Bit 205: 1000/1000 (100%)
Bit 206: 375/1000 (37.5%)
Bit 207: 0/1000 (0%)
Bit 208: 494/1000 (49.4%)
Bit 209: 428/1000 (42.8%)
Bit 210: 429/1000 (42.9%)
Bit 211: 138/1000 (13.8%)
Bit 212: 627/1000 (62.7%)
Bit 213: 1000/1000 (100%)
Bit 214: 373/1000 (37.3%)
Bit 215: 0/1000 (0%)
Bit 216: 507/1000 (50.7%)
Bit 217: 460/1000 (46%)
Bit 218: 419/1000 (41.9%)
Bit 219: 118/1000 (11.8%)
Bit 220: 630/1000 (63%)
Bit 221: 1000/1000 (100%)
Bit 222: 370/1000 (37%)
Bit 223: 0/1000 (0%)
Bit 224: 522/1000 (52.2%)
Bit 225: 440/1000 (44%)
Bit 226: 451/1000 (45.1%)
Bit 227: 112/1000 (11.2%)
Bit 228: 606/1000 (60.6%)
Bit 229: 1000/1000 (100%)
Bit 230: 394/1000 (39.4%)
Bit 231: 0/1000 (0%)
Bit 232: 518/1000 (51.8%)
Bit 233: 429/1000 (42.9%)
Bit 234: 401/1000 (40.1%)
Bit 235: 135/1000 (13.5%)
Bit 236: 643/1000 (64.3%)
Bit 237: 1000/1000 (100%)
Bit 238: 357/1000 (35.7%)
Bit 239: 0/1000 (0%)
Bit 240: 511/1000 (51.1%)
Bit 241: 411/1000 (41.1%)
Bit 242: 463/1000 (46.3%)
Bit 243: 125/1000 (12.5%)
Bit 244: 634/1000 (63.4%)
Bit 245: 1000/1000 (100%)
Bit 246: 366/1000 (36.6%)
Bit 247: 0/1000 (0%)
Bit 248: 492/1000 (49.2%)
Bit 249: 438/1000 (43.8%)
Bit 250: 438/1000 (43.8%)
Bit 251: 131/1000 (13.1%)
Bit 252: 591/1000 (59.1%)
Bit 253: 1000/1000 (100%)
Bit 254: 409/1000 (40.9%)
Bit 255: 0/1000 (0%)
➜  build ./test_benchmark
Input size: 16 bytes → hash: 4e8d54479382bce5... Elapsed: 0.042686 ms
Input size: 256 bytes → hash: 7ee2ea149c6d235d... Elapsed: 0.003663 ms
Input size: 1024 bytes → hash: 5e07550c853ddcee... Elapsed: 0.005065 ms
Input size: 1048576 bytes → hash: dff0d3b0abae4089... Elapsed: 5.59786 ms
```
