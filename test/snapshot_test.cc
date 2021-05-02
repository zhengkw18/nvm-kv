#include <assert.h>
#include <stdio.h>

#include <string>
#include <algorithm>

#include "include/engine.h"
#include "test_util.h"

using namespace polar_race;

#define KV_CNT 10000
#define TEST_CNT 10

char k[1024];
char v[9024];
std::string ks[KV_CNT];
std::string vs[TEST_CNT][KV_CNT];
Snapshot *ss[TEST_CNT];

int main()
{

    Engine *engine = NULL;
    printf_(
        "======================= snapshot test "
        "============================");
#ifdef MOCK_NVM
    std::string engine_path =
        std::string("/tmp/ramdisk/data/test-") + std::to_string(asm_rdtsc());
#else
    std::string engine_path = "/dev/dax0.0";
#endif
    RetCode ret = Engine::Open(engine_path, &engine);
    assert(ret == kSucc);
    printf("open engine_path: %s\n", engine_path.c_str());

    std::string value;

    for (int i = 0; i < KV_CNT; ++i)
    {
        gen_random(k, 6);
        ks[i] = std::string(k) + std::to_string(i);
    }
    for (int i = 0; i < TEST_CNT; ++i)
    {
        for (int j = 0; j < KV_CNT / TEST_CNT * i; ++j)
        {
            gen_random(v, 511);
            vs[i][j] = v;
        }
    }

    for (int i = 0; i < TEST_CNT; ++i)
    {
        for (int j = 0; j < KV_CNT / TEST_CNT * i; ++j)
        {
            ret = engine->Write(ks[j], vs[i][j]);
            assert(ret == kSucc);
        }
        ss[i] = engine->GetSnapshot();
    }

    for (int i = 0; i < TEST_CNT; ++i)
    {
        for (int j = 0; j < KV_CNT / TEST_CNT * i; ++j)
        {
            ret = engine->Read(ks[j], &value, ss[i]);
            assert(ret == kSucc);
            assert(value == vs[i][j]);
        }
        for (int j = KV_CNT / TEST_CNT * i; j < KV_CNT; ++j)
        {
            ret = engine->Read(ks[j], &value, ss[i]);
            assert(ret == kNotFound);
        }
    }

    printf_(
        "======================= snapshot test pass :) "
        "======================");

    return 0;
}