#include <assert.h>
#include <stdio.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "include/engine.h"
#include "test_util.h"

using namespace polar_race;

#define KV_CNT 1000
#define TEST_CNT 100000
#define KEY_SIZE 16
#define VALUE_SIZE 16
#define THREAD_NUM 4
#define CONFLICT_KEY 50

char v[9024];
std::string key;
std::string vs[THREAD_NUM][KV_CNT];
Engine* engine = NULL;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class Tracer {
public:
    Tracer(const std::string& filename) {
        outfile.open(filename, std::ios_base::trunc);
    }
    void start_write(int tid, std::string* value) {
        std::lock_guard<std::mutex> lk(mu_);
        log("invoke", "write", value, tid);
    }
    void finish_write(int tid, std::string* value) {
        std::lock_guard<std::mutex> lk(mu_);
        log("ok", "write", value, tid);
    }
    void start_read(int tid) {
        std::lock_guard<std::mutex> lk(mu_);
        log("invoke", "read", nullptr, tid);
    }
    void finish_read(int tid, std::string* value) {
        std::lock_guard<std::mutex> lk(mu_);
        log("ok", "read", value, tid);
    }
    ~Tracer() { outfile << std::endl; }

private:
    void log(const std::string& type, const std::string f, std::string* value,
             int tid) {
        outfile << "{:type :" << type << ", :f :" << f << ", :value "
                << ((value == nullptr || value->empty()) ? "nil" : "V" + *value)
                << ", :process " << tid << "}\n";
    }
    std::mutex mu_;
    std::ofstream outfile;
};
std::string trace_file = "./trace.edn";
Tracer tracer(trace_file);

void test_thread_conflict(int id) {
    RetCode ret;
    std::string value;

    for (int i = 0; i < TEST_CNT; ++i) {
        if (rand_int(0, 100) <= 50) {
            tracer.start_read(id);
            ret = engine->Read(key, &value);
            tracer.finish_read(id, &value);
            assert(ret == kSucc);
        } else {
            tracer.start_write(id, &vs[id][i % KV_CNT]);
            ret = engine->Write(key, vs[id][i % KV_CNT]);
            tracer.finish_write(id, &vs[id][i % KV_CNT]);
            assert(ret == kSucc);
        }
    }
}

int main() {
    printf_(
        "======================= multi thread test "
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

    for (int t = 0; t < THREAD_NUM; ++t) {
        for (int i = 0; i < KV_CNT; ++i) {
            gen_random(v, VALUE_SIZE);
            vs[t][i] = v;
        }
    }

    std::thread ths[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i) {
        ths[i] = std::thread(test_thread_conflict, i);
    }
    for (int i = 0; i < THREAD_NUM; ++i) {
        ths[i].join();
    }

    delete engine;

    printf_("trace generated to " + trace_file);

    return 0;
}
