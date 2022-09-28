# Libbyzea - An energy-aware BFT library

Libbyzea is a library for implementing byzantine fault-tolerant (BFT) services.
It is based on MIT's [Practical Byzantine Fault Tolerance
(PBFT)](https://pmg.csail.mit.edu/bft/) library (`libbyz`) and adds
optimizations that reduce the implemented protocol's energy footprint.  It is
mainly intended to be used on systems with energy-constraints, like embedded
systems with batteries as their main power source, for example.

In order to be compatible with modern compilers, a few technical changes were
made to the original code. While `libbyz` uses the no longer maintained library
`sfslite` for cryptographic operations, `libbyzea` uses
[MbedTLS](https://github.com/Mbed-TLS/mbedtls).  This also made it necessary to
replace the Rabin public key cryptosystem with RSA, since the former is not
supported by MbedTLS. It is planned to replace RSA with elliptic-curve
algorithms in the future to reduce keysizes and to improve performance.

## Dependencies

The following dependencies need to be installed in order to build the library:

* CMake >= v3.18.4
* MbedTLS >= v2.18.1 and < v3.0.0

## Building

The library can be built using CMake by executing the following commands in the
repository's root directory:

```sh
mkdir build
cd build
cmake ..
make
```

This outputs the static library `libbyzea.a` inside the `build` directory.

If your installation of MbedTLS is not located in the default build path, you
can manually supply the location of its header files to CMake. Just replace the
`cmake` command from above with the following:

```sh
cmake -DMBEDTLS_INCLUDE_PATH=/path/to/mbedtls/include ..
```

If you want to supply a relative path instead, make sure the path is relative to
this repository's root directory.
