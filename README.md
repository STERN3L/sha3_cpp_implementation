# SHA3-256 Implementation in C++

![C++11](https://img.shields.io/badge/C%2B%2B-11-00599C?logo=cplusplus&logoColor=white)
![Standard](https://img.shields.io/badge/standard-FIPS%20202-blue)
![Algorithm](https://img.shields.io/badge/algorithm-SHA3--256-purple)
![Platform](https://img.shields.io/badge/platform-OpenBSD%20%7C%20Linux-lightgrey)
![Compilers](https://img.shields.io/badge/compilers-clang%20%7C%20gcc-orange)
![Dependencies](https://img.shields.io/badge/dependencies-none-brightgreen)
![Last commit](https://img.shields.io/github/last-commit/STERN3L/sha3_cpp_implementation)
![Repo size](https://img.shields.io/github/repo-size/STERN3L/sha3_cpp_implementation)
![Top language](https://img.shields.io/github/languages/top/STERN3L/sha3_cpp_implementation)

A simple, dependency-free implementation of the **SHA3-256** hash function
([FIPS 202](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf)) in C++11.
It hashes files of any size in constant memory and gives the same results as
`openssl dgst -sha3-256`.

The code is written for readability rather than speed: each step of the Keccak
permutation is a separate function and the notation follows the Keccak specification.

---

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [Testing](#testing)
- [How It Works](#how-it-works)
- [Design Choices](#design-choices)
- [Performance](#performance)
- [References](#references)

---

## Features

- SHA3-256 (Keccak-f[1600], rate 1088 bits, capacity 512 bits)
- Hashes files of any size (tested with 50 MB) in **constant memory**
- No dynamic allocation: all buffers have a fixed size and live on the stack
- Reads files in binary mode, so any byte value (`\0`, `\n`, `\r`, `0xff`…) is handled correctly
- Endianness-independent: lanes are built byte by byte in little-endian order
- Standard C++11 only, no external library
- Portable POSIX `sh` test suite, compared against known test vectors and OpenSSL

---

## Project Structure

```
.
├── sha3.cpp      # Implementation: Keccak-f[1600], absorption, padding, file reading, main()
├── sha3.h        # Constants: rate, hash size, round constants (RC), rotation offsets (R)
├── Makefile      # Build, test and packaging targets
├── test.sh       # Test suite (POSIX sh)
└── README.md     # This file
```

---

## Requirements

- A C++11 compiler: `clang++` (default `c++` on OpenBSD) or `g++`
- `make` (BSD make or GNU make)
- Optional, for tests: `openssl` with SHA3-256 support (`openssl dgst -sha3-256`)

---

## Building

```sh
make
```

This compiles `sha3.cpp` with:

```sh
c++ -O2 -Wall -Wextra -std=c++11 -o sha3 sha3.cpp
```

To use a specific compiler:

```sh
make CXX=clang++
make CXX=g++
```

To remove the binary and temporary test files:

```sh
make clean
```

---

## Usage

```sh
./sha3 <file>
```

The program prints the hash as 64 lowercase hexadecimal characters.

**Example:**

```sh
$ printf 'abc' > abc.txt
$ ./sha3 abc.txt
3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532
```

**Exit codes:**

| Code | Meaning                                         |
| ---- | ----------------------------------------------- |
| `0`  | Success                                         |
| `1`  | Wrong number of arguments, or file not readable |

---

## Testing

```sh
make test
# or, to test any binary:
sh test.sh ./sha3
```

The test suite checks:

1. **Known test vectors**
   - empty message
   - `"abc"`
   - `"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"` (56 bytes)
   - 1,000,000 × `'a'`
2. **Comparison with OpenSSL** (skipped if OpenSSL does not support SHA3-256)
   - random files of 1, 134, 135, 136, 137, 271, 272, 273, 1000 and 100000 bytes,
     chosen around the 136-byte block boundary
   - a file containing all 256 byte values, to detect encoding problems
3. **Large file**: a random 50 MB file (execution time + comparison with OpenSSL)
4. **Error handling**: a missing file must return a non-zero exit code

Example output:

```
== 1. Vecteurs de test connus ==
OK    message vide
OK    "abc"
...
Resultat : 17 OK, 0 ECHEC
```

Because the script takes the binary as an argument, the same tests can be run
on builds made with different compilers or optimisation levels:

```sh
clang++ -O0 -std=c++11 -o sha3_O0 sha3.cpp && sh test.sh ./sha3_O0
clang++ -O2 -fsanitize=address,undefined -std=c++11 -o sha3_asan sha3.cpp && sh test.sh ./sha3_asan
```

---

## How It Works

SHA3 uses the **sponge construction**: a 1600-bit state is filled with the message
block by block, mixed by the Keccak-f permutation after each block, and the hash is
read from the state at the end.

```
file ──read──► bloc[136] ──absorber──► A[5][5] ──keccak_f (24 rounds)──► A[5][5]
                                          ▲                                 │
                                          └────────── next block ───────────┘

last block + padding ──► absorber ──► first 4 lanes of A ──► hash[32] ──► output
```

### Main functions (`sha3.cpp`)

| Function             | Role                                                                           |
| -------------------- | ------------------------------------------------------------------------------ |
| `rotl64`             | 64-bit left rotation (handles `n = 0` separately to avoid undefined behaviour) |
| `theta`              | XORs each lane with the parity of two neighbouring columns                     |
| `rho_pi`             | Rotates each lane (`rho`) and moves it to a new position (`pi`)                |
| `chi`                | Non-linear step: combines each lane with its two neighbours in the row         |
| `iota`               | XORs a round constant into `A[0][0]`                                           |
| `keccak_f`           | Applies the 4 steps above for 24 rounds                                        |
| `octets_vers_uint64` | Builds a 64-bit lane from 8 bytes in little-endian order                       |
| `absorber`           | XORs a 136-byte block into the first 17 lanes, then calls `keccak_f`           |
| `sha3_256_fichier`   | Reads the file, absorbs each block, applies padding, extracts the hash         |

### Data structures

| Array               | Size          | Purpose                                                |
| ------------------- | ------------- | ------------------------------------------------------ |
| `A[5][5]`           | 200 bytes     | The Keccak state (25 lanes of 64 bits)                 |
| `bloc[136]`         | 136 bytes     | The block currently read from the file                 |
| `C[5]`, `D[5]`      | 40 bytes each | Column parities used by `theta`                        |
| `B[5][5]`           | 200 bytes     | Temporary copy of the state in `rho_pi` and `chi`      |
| `hash[32]`          | 32 bytes      | The final 256-bit digest                               |
| `RC[24]`, `R[5][5]` | constant      | Round constants and rotation offsets from the standard |

---

## Design Choices

**Block-by-block reading.**
The file is read 136 bytes at a time (the SHA3-256 rate) with `ifstream::read`.
Each full block is absorbed immediately. As soon as a read returns fewer than
136 bytes (`gcount()`), the remaining `n` bytes (0 ≤ n < 136) become the last block.

**Padding.**
The last block is filled with zeros, then `0x06` is written at position `n` (SHA3
domain suffix `01` + first bit of `pad10*1`) and `0x80` is OR-ed into the last byte.
Since `n < 136`, there is always at least one free byte, so a single padding block
is always enough. When `n = 135`, both values fall on the same byte, which becomes
`0x86`; using `|=` handles this case without any special code.

**Memory.**
Only fixed-size arrays are used (state, current block, temporaries), all on the stack.
There is no `new`, `malloc` or `std::vector`, so memory usage does not depend on the
file size and no leak is possible.

**Initialisation.**
The state is zero-initialised with `= {}`, and the end of the last block is cleared
before padding, so no leftover data from the previous block is hashed. Results are
identical at `-O0`, `-O2` and `-O3`.

**Binary input and endianness.**
Files are opened with `std::ios::binary`. Lanes are built byte by byte with shifts
instead of casting the buffer to `uint64_t*` or using `memcpy`, so the result does
not depend on the machine's endianness or on memory alignment.

**Hash extraction.**
The 32-byte digest is smaller than the rate, so it is read directly from the first
four lanes `A[0][0]` to `A[3][0]`, without another permutation.

---

## Performance

Measured on Linux with a random 50 MB file:

| Build                                      | Tests                         | Time (50 MB) |
| ------------------------------------------ | ----------------------------- | ------------ |
| `clang++ -O0`                              | 17/17 pass                    | ~6 s         |
| `clang++ -O2` / `-O3`                      | 17/17 pass                    | ~1 s         |
| `g++ -O0`                                  | 17/17 pass                    | ~6 s         |
| `g++ -O2` / `-O3`                          | 17/17 pass                    | ~1 s         |
| `clang++ -O2 -fsanitize=address,undefined` | 17/17 pass, no error reported | —            |

Peak memory usage is about 3.8 MB (mostly the C++ runtime), and the program still
runs under `ulimit -v 10000`.

The target platform is OpenBSD (default compiler: clang). The code only uses
standard C++11 and the Makefile works with both BSD make and GNU make.

---

## References

- NIST, _FIPS 202 — SHA-3 Standard: Permutation-Based Hash and Extendable-Output Functions_, 2015
- G. Bertoni, J. Daemen, M. Peeters, G. Van Assche, _The Keccak reference_, version 3.0
- Keccak team website: <https://keccak.team>
