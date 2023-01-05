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
* MbedTLS >= v3.2.1

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

### Build Options

The library's build options can be used to tailor it to your needs. All of the
build options described below can be supplied via Cmake variables like this:

```sh
cmake -D<OPTION1_NAME>=<OPTION1_VALUE> -D<OPTION2_NAME>=<OPTION2_VALUE> ..
```

#### Change MbedTLS Include Path (MBEDTLS_INCLUDE_PATH)

```sh
cmake -DMBEDTLS_INCLUDE_PATH=/path/to/mbedtls/include ..
```

If your installation of MbedTLS is not located in the default location of your
system, you can manually supply the location of its header files to CMake. If
you want to supply a relative path instead, make sure the path is relative to
this repository's root directory, not the build directory.

#### Change Maximum Message Size (MAX_MESSAGE_SIZE)

```sh
cmake -DMAX_MESSAGE_SIZE=2048 ..
```

The library defines a maximum message size that is used to allocate certain data
structures, like incoming new messages for example. If your system has only a
small amount of RAM, it can be useful to reduce this size when building the library.

#### Change Maximum Reply Size (MAX_REPLY_SIZE)

```sh
cmake -DMAX_REPLY_SIZE=1240 ..
```

This changes the maximum size replies can have. It must be smaller than
MAX_MESSAGE_SIZE.

#### Change the state page size (BLOCK_SIZE)

```sh
cmake -DBLOCK_SIZE=4096 ..
```

The `State` and `TrivialState` classes manage the system state in data
structures that are block-based, meaning the system state is chunked into blocks
of `BLOCK_SIZE`. This number must be a multiple of two and should be smaller or
equal to your architecture's page size. Since blocks may be sent between
replicas during state transfer, `BLOCK_SIZE` must be smaller than `MAX_MESSAGE_SIZE`


#### Change the Maximum Number of Replicas (MAX_NUM_REPLICAS)

```sh
cmake -DMAX_NUM_REPLICAS=4 ..
```

Libbyzea by default supports up to 32 replicas (i.e. `f = 10`). Changing the
maximum number of replicas affects the size of certain data structures and
message types, since they have to store information for each possible replica.
Reducing this size can positively influence the library's memory footprint, at
the cost of less redundancy.

#### Change the Window Size (WINDOW_SIZE)

```sh
cmake -DWINDOW_SIZE=64 ..
```

The window describes the number of sequence numbers that are accepted by a
replica during a certain point in time. This means that many of the library's
data structures have to allocate memory for each slot in a window. Reducing this
size can drastically reduce the library's memory consumption. Its **minimum value**
64.

#### Change Checkpoint Interval (CHECKPOINT_INTERVAL)

```sh
cmake -DCHECKPOINT_INTERVAL=2 ..
```

The checkpoint interval defines how many checkpoints are created during a
certain _message window_. Its value is relative to the `WINDOW_SIZE`. A value of
2 means that during a window a checkpoint is made twice.

The sequence numbers in a window where checkpoints are made are exactly those
where `SEQUENCE_NUMBER mod (WINDOW_SIZE / CHECKPOINT_INTERVAL) == 0`.  For
example if `WINDOW_SIZE` is set to 64 and `CHECKPOINT_INTERVAL` is set to 2,
then a checkpoint is created every time the sequence number of the last request
executed modulo 32 is 0.

Note that a larger amount of checkpoints means that potentially more memory has
to be allocated to safe checkpoint state if there exist many checkpoints that
are not stable, yet.

#### Tuning the Log Allocator (LOG_CHUNK_SIZE, LOG_NUM_CHUNKS)

```sh
cmake -DLOG_CHUNK_SIZE=4096 -DLOG_NUM_CHUNKS=8 ..
```

The original PBFT library came with a special memory allocator used for
messages. This allocator allocates the memory it manages via `mmap(2)` in chunks
of size `LOG_CHUNK_SIZE * LOG_NUM_CHUNKS`. Furthermore, it requires the memory
returned by `mmap(2)` to be aligned to `LOG_CHUNK_SIZE`. This can be problematic
on architectures that do not have virtual memory (e.g. embedded platforms).
Therefore, it can be useful to reduce the chunk size and number of chunks
allocated by a single mmap.

#### Tuning the Partition Tree (PARTITION_TREE_LEVELS, PARTITION_TREE_CHILDREN)
```sh
camke -DPARTITION_TREE_LEVELS=4 -DPARTITIONS_TREE_CHILDREN=256 ..
```

The `State` class uses a multi-level partition tree structure in order to track
changes to the blocks of the application state. The maximum amount of
application memory depends on `BLOCK_SIZE`, the number of levels, and the number
of children per node in the partition tree. A larger tree also means more
memory overhead for allocating tree nodes.  The number of hierarchy levels in
the partition tree can be altered using `PARTITION_TREE_LEVELS`. Valid values
are `2`, `3` or `4`. In order to control the number of children per node in the
partition tree, you can use `PARTITION_TREE_CHILDREN`.

#### Enable Recovery Suppport (ENABLE_RECOVERY)

```sh
cmake -DENABLE_RECOVERY
```

Enables PBFT's recovery support, allowing a replica to persist its current
state to disk and load it on rebooting.
