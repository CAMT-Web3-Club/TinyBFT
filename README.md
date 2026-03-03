# TinyBFT - A BFT Library for Tiny Embedded Devices

TinyBFT is a library for implementing byzantine fault-tolerant (BFT) services.
It is based on MIT's [Practical Byzantine Fault Tolerance
(PBFT)](https://pmg.csail.mit.edu/bft/) library (`libbyz`) and adds
optimizations that reduce the implemented protocol's memory footprint.  It is
mainly intended to be used on systems with heavily resource-constrained
devices.

In order to be compatible with modern compilers, a few technical changes were
made to the original PBFT code. While `libbyz` uses the no longer maintained
library `sfslite` for cryptographic operations, `libbyzea` uses
[MbedTLS](https://github.com/Mbed-TLS/mbedtls).  This also made it necessary to
replace the Rabin public key cryptosystem with RSA, since the former is not
supported by MbedTLS.

## Dependencies

The following dependencies need to be installed in order to build the library:

* CMake >= v3.18.4
* MbedTLS == v3.5.2

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

#### Enable TinyBFT

```sh
cmake -DTINY_BFT=1 ..
```

The default configuration builds the regular PBFT prototype without TinyBFT's
optimizations. This switch allows you to enable TinyBFT for the build.

#### Change Maximum Message Size (MAX_MESSAGE_SIZE)

```sh
cmake -DMAX_MESSAGE_SIZE=2048 ..
```

The library defines a maximum message size that is used to allocate certain
data structures, like incoming new messages for example. If your system has
only a small amount of RAM, it can be useful to reduce this size when building
the library.

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
structures that are block-based, meaning the system state is chunked into
blocks of `BLOCK_SIZE`. This number must be a multiple of two and should be
smaller or equal to your architecture's page size. Since blocks may be sent
between replicas during state transfer, `BLOCK_SIZE` must be smaller than
`MAX_MESSAGE_SIZE`


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
cmake -DWINDOW_SIZE=256 ..
```

The window describes the number of sequence numbers that are accepted by a
replica during a certain point in time. This means that many of the library's
data structures have to allocate memory for each slot in a window. Reducing
this size can drastically reduce the library's memory consumption.

#### Change Checkpoint Interval (CHECKPOINT_INTERVAL)

```sh
cmake -DCHECKPOINT_INTERVAL=128 ..
```

The checkpoint interval defines at which sequence numbers checkpoints are
created.  A checkpoint is created after a executing requests with a sequence
number divisible by the checkpoint interval, that is `<sequence_number> %
CHECKPOINT_INTERVAL == 0`.  If the checkpoint interval is set to 128 for
example, then a checkpoint is created every time the sequence number of the
last request executed modulo 128 is 0.

Note that a larger amount of checkpoints means that potentially more memory has
to be allocated to safe checkpoint state if there exist many checkpoints that
are not stable, yet.

#### Disable Multicast

```sh
cmake -DDISABLE_MULTICAST=1 ..
```

Normally, replicas use UDP multicast messages to communicate with each other.
In certain network setups (e.g. across local network boundaries), this can lead
to problems. Disabling multicasts uses point-to-point UDP packets instead.

#### Enable Performance Statistics (PRINT_PERF_STATISTICS)

```sh
cmake -DPRINT_PERF_STATISTICS=1 ..
```

Enables gathering of performance statistics, that is, cycles spent in various
parts of the protocol.

#### Enable Memory Demand Statistics (PRINT_MEM_STATISTICS)

```sh
camke -DPRINT_MEM_STATISTICS=1 ..
```

Enables the (dynamic) memory demand benchmark. This compiles a custom
`malloc(3)` implementation into the library which tracks all memory
allocations.  The memory demand during initialization is then printed per
component. The output printed is of the form:

```
sizeof(Replica) = 2704
sizeof(Node) = 1736
sizeof(Prepared_cert) = 120
sizeof(Log<Prepared_cert>) = 32
sizeof(Certificate<Commit>) = 80
sizeof(Log<Certificate<Commit>>) = 32
sizeof(Certificate<Checkpoint>) = 80
sizeof(Log<Certificate<Checkpoint>>) = 32
Byz_init_replica->Replica->Node->RsaPrivateKey::RsaPrivateKey->mbedtls_pk_parse_keyfile: 1496
Byz_init_replica->Replica->Node->RsaPrivateKey::RsaPrivateKey: 1496
Byz_init_replica->Replica->Node->Principal->umac_new: 1528
Byz_init_replica->Replica->Node->Principal: 1648
...
Byz_init_replica->Replica->State: 6347224
...
Byz_init_replica->Replica: 6574346
Byz_init_replica: 6574818
max_total = 6651390
```

The memory consumed by a component always also includes the memory allocated by
the components it calls. If `Replica` calls the `State` constructor during
initialization for example, the memory allocated by `State`'s constructor is also
added to `Replica`'s memory demand.

## Runtime Configuration

The library reads cluster information from a configuration file during
initialization. Since the configuration has no support for comments, this
section explains its file format. Below is an example configuration file:

```
generic
1
1800000
4
234.5.6.8 3669
esp0 192.168.178.47 3669 pub/esp0.pub
esp1 192.168.178.48 3669 pub/esp1.pub
esp2 192.168.178.49 3669 pub/esp2.pub
esp3 192.168.178.50 3669 pub/esp3.pub
181000
150
9999250000
```

The configuration file is line-based. The first line holds an arbitrary service
name (`generic` in the example). The next three lines define the tolerated
number of faulty replicas `f`, the authentication timeout in milliseconds when
using authenticators instead of signatures, and the total number of nodes, i.e.
replicas and clients. In the example above this would mean `f = 1`, a
authentication timeout of 30 minutes, and 4 nodes in total.

The next lines define the nodes in the system. Here, the first line `234.5.6.8`
always holds the multicast address used when enabling multicast. This is
followed by the actual nodes in the system. Here, each line holds information
for one specific node using the following format:

```
<hostname> <ip-address> <port> <path-to-rsa-public-key>
```

Finally, the last three lines define three different timeouts in milliseconds
relevant to replicas only:

- View-Change Timeout (`181000`)
- Status Message Timeout (`150`)
- Recovery Timeout (`9999250000`)
