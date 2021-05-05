#include <thread>
#include <algorithm>

#include "bench_util.h"
#include "include/engine.h"
#include "zipf.h"

#define MAX_THREAD 64
#define OP_PER_THREAD 200000ull
#define KEY_SPACE (OP_PER_THREAD * 4)
#define KEY_SIZE 8
#define VALUE_SIZE 16
#define VALUE_BUFFER 5000

static_assert(VALUE_BUFFER > VALUE_SIZE);
// key size is not configurable.
static_assert(KEY_SIZE == 8);

using namespace polar_race;

int threadNR = 1;
Engine *engine = NULL;
std::string ks[KEY_SPACE / 100];

struct MyVisitor : Visitor
{
    MyVisitor() {}
    void Visit(const PolarString &key, const PolarString &) override {}
};
MyVisitor visitor;

void usage()
{
    fprintf(stderr,
            "Usage: ./bench_range thread_num[1-64]\n");
    exit(-1);
}

void parseArgs(int argc, char **argv)
{
    if (argc != 2)
    {
        usage();
    }
    threadNR = std::atoi(argv[1]);

    if (threadNR <= 0 || threadNR > MAX_THREAD)
        usage();

    fprintf(stdout, "thread_num: %d\n", threadNR);
}

void bench_thread(int id)
{
    unsigned int seed = asm_rdtsc() + id;
    for (int i = 0; i < OP_PER_THREAD; ++i)
    {
        int lo = rand_r(&seed) % (KEY_SPACE / 100), hi = rand_r(&seed) % (KEY_SPACE / 100);
        if (lo > hi)
        {
            std::swap(lo, hi);
        }
        engine->Range(ks[lo], ks[hi], visitor);
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
    for (int i = 0; i < KEY_SPACE; i += 100)
    {
        uint64_t key = i;
        ks[i / 100] = std::string((char *)&key, sizeof(uint64_t));
        PolarString k((char *)&key, sizeof(uint64_t));
        engine->Write(k, v);
    }
    delete engine;
    std::sort(ks, ks + KEY_SPACE / 100);
    timespec s, e;
    std::thread ths[MAX_THREAD];
    clock_gettime(CLOCK_REALTIME, &s);
    ret = Engine::Open(engine_path, &engine);
    assert(ret == kSucc);
    for (int i = 0; i < threadNR; ++i)
    {
        ths[i] = std::thread(bench_thread, i);
    }

    for (int i = 0; i < threadNR; ++i)
    {
        ths[i].join();
    }
    clock_gettime(CLOCK_REALTIME, &e);

    double us = (e.tv_sec - s.tv_sec) * 1000000 + (double)(e.tv_nsec - s.tv_nsec) / 1000;
    printf("%d thread, %llu operations per thread, time: %lfus\n", threadNR, OP_PER_THREAD, us);
    printf("throughput %lf operations/s\n", 1ull * (threadNR * OP_PER_THREAD) * 1000000 / us);

    delete engine;

    system((std::string("rm -rf ") + engine_path).c_str());

    return 0;
}
