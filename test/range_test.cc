#include <assert.h>
#include <stdio.h>

#include <string>
#include <algorithm>

#include "include/engine.h"
#include "test_util.h"

using namespace polar_race;

#define KV_CNT 10000
#define TEST_CNT 1000

char k[1024];
char v[9024];
std::string ks[KV_CNT];
std::string vs[KV_CNT];

struct MyVisitor : Visitor
{
    int cnt = 0;
    std::string *ks;
    int start;
    MyVisitor(std::string *ks, int start) : ks(ks), start(start) {}
    void Visit(const PolarString &key, const PolarString &) override
    {
        assert(key.compare(PolarString(ks[start + cnt])) == 0);
        ++cnt;
    }
    int get_cnt() { return cnt; }
};
int main()
{

    Engine *engine = NULL;
    printf_(
        "======================= range test "
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
        gen_random(v, 511);
        vs[i] = v;
    }

    std::sort(ks, ks + KV_CNT);

    for (int i = 0; i < KV_CNT; ++i)
    {
        ret = engine->Write(ks[i], vs[i]);
        assert(ret == kSucc);
    }

    for (int i = 0; i < TEST_CNT; ++i)
    {
        int lo = rand() % KV_CNT, hi = rand() % KV_CNT;
        if (lo > hi)
        {
            std::swap(lo, hi);
        }
        MyVisitor v(ks, lo);
        ret = engine->Range(ks[lo], ks[hi], v);
        assert(ret == kSucc);
        assert(v.get_cnt() == hi - lo);
    }

    for (int i = 0; i < TEST_CNT; ++i)
    {
        int hi = rand() % KV_CNT;
        MyVisitor v(ks, 0);
        ret = engine->Range("", ks[hi], v);
        assert(ret == kSucc);
        assert(v.get_cnt() == hi);
    }

    for (int i = 0; i < TEST_CNT; ++i)
    {
        int lo = rand() % KV_CNT;
        MyVisitor v(ks, lo);
        ret = engine->Range(ks[lo], "", v);
        assert(ret == kSucc);
        assert(v.get_cnt() == KV_CNT - lo);
    }

    MyVisitor v(ks, 0);
    ret = engine->Range("", "", v);
    assert(ret == kSucc);
    assert(v.get_cnt() == KV_CNT);

    printf_(
        "======================= range test pass :) "
        "======================");

    return 0;
}