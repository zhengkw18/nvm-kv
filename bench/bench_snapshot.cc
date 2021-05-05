#include <thread>
#include <algorithm>

#include "bench_util.h"
#include "include/engine.h"
#include "zipf.h"

#define OP_PER_THREAD 100000ull
#define KEY_SIZE 8
#define VALUE_SIZE 16
#define VALUE_BUFFER 5000

static_assert(VALUE_BUFFER > VALUE_SIZE);
// key size is not configurable.
static_assert(KEY_SIZE == 8);

using namespace polar_race;

Engine *engine = NULL;
std::string vs[OP_PER_THREAD];
Snapshot *ss[OP_PER_THREAD];

void usage()
{
    fprintf(stderr,
            "Usage: ./bench_snapshot\n");
    exit(-1);
}

void parseArgs(int argc, char **argv)
{
    if (argc != 1)
    {
        usage();
    }
}

int main(int argc, char **argv)
{
    parseArgs(argc, argv);
#ifdef MOCK_NVM
    system("mkdir -p /tmp/ramdisk/data");
    std::string engine_path =
        std::string("/tmp/ramdisk/data/test-") + std::to_string(asm_rdtsc());
#else
    std::string engine_path = "/dev/dax0.0";
#endif
    printf("open engine_path: %s\n", engine_path.c_str());

    RetCode ret = Engine::Open(engine_path, &engine);
    assert(ret == kSucc);

    char v[5000];
    gen_random(v, 4096);
    for (int i = 0; i < OP_PER_THREAD; ++i)
    {
        uint64_t key = i;
        PolarString k((char *)&key, sizeof(uint64_t));
        engine->Write(k, v);
    }
    delete engine;
    timespec s, e;
    clock_gettime(CLOCK_REALTIME, &s);
    ret = Engine::Open(engine_path, &engine);
    assert(ret == kSucc);
    uint64_t key = OP_PER_THREAD;
    PolarString k((char *)&key, sizeof(uint64_t));
    for (int i = 0; i < OP_PER_THREAD; ++i)
    {
        gen_random(v, VALUE_SIZE);
        vs[i] = v;
        engine->Write(k, v);
        ss[i] = engine->GetSnapshot();
    }
    std::string val;
    for (int i = 0; i < OP_PER_THREAD; ++i)
    {
        engine->Read(k, &val, ss[i]);
    }
    clock_gettime(CLOCK_REALTIME, &e);

    double us = (e.tv_sec - s.tv_sec) * 1000000 + (double)(e.tv_nsec - s.tv_nsec) / 1000;
    printf("%llu operations per thread, time: %lfus\n", OP_PER_THREAD, us);
    printf("throughput %lf operations/s\n", 1ull * OP_PER_THREAD * 1000000 / us);

    delete engine;

    system((std::string("rm -rf ") + engine_path).c_str());

    return 0;
}
