# Non-volatile Memory Based Key Value Store

## All you have to do

Complete engine_race/engine_race.[h,cc], and execute

```
make
```
to build your own engine

It is equivalent to `make MOCK_NVM=1` and will use mock NVM device by default.

## Example for you

For quick start, we have already implemented a simple
example engine in engine_example, you can view it and execute

```
make TARGET_ENGINE=engine_example
```
to build this example engine

## Correctness Test

After building the engine (`make` for your implementation, or `make TARGET_ENGINE=engine_example` for the example)

```
cd test
./build.sh
./run_test.sh
```

### (Optional) Test for linearizability

Build the test, and run

```
cd test
./multi_thread_linearizability
```

Then, a trace file called `trace.edn` will be generated under the current directory.

Then, you can use tool like [knossos](https://github.com/jepsen-io/knossos) to test whether your implementation violate linearizability.

## Performance Test

After building the engine

```
cd bench
./build.sh

# type ./bench to see the usage
./bench 1 100 0
```

## Run with Real NVM
```
make MOCK_NVM=0

cd bench
./build-real-nvm.sh

cd test
./build-real-nvm.sh
```

## NOTICE

### 1. About running performance test & correctness test

Running those tests *require* `/tmp/ramdisk/data` exists.

``` bash
# to create the required directory
mkdir -p /tmp/ramdisk/data
```

Otherwise, the test will fail.

### 2. About available space

The tests will fail if space is not enough.

You can ...

- use `df -h` to watch the available space.

- use `rm -rf /tmp/ramdisk/data/*` to clean up space between each test.

- allocate more space for the ramdisk.

### 3. About out of memory

If your host has less than 4GB memory, allocating a 4GB ramdisk may fail, or use up all your available memory. If your memory is mostly used up, the performance/correctness tests may get killed by your OS because of OOM(out of memory). Even worse, your computer may stop responding (i.e. get stuck).

You can ...

- Allocate less memory (i.e. 2GB) for your ramdisk. In this case, you may not be able to run `engine_example`.

### 4. About key/value size

Your implementation *should* support variable-length keys and values *within* 512 bytes (512 included), and *optimized* for small KV pairs (i.e. 8 byte keys and 16 bytes value).

## UPDATE LOG

1. Change `KEY_SIZE` to 8 and `VALUE_SIZE` to 16 in all tests.