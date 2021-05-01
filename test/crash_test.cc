#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <string>

#include "include/engine.h"
#include "test_util.h"

using namespace polar_race;

#define KV_CNT 30000
#define KEY_SIZE 8
#define VALUE_SIZE 16

char k[1024];
char v[9024];

std::string ks[KV_CNT];
std::string vs[KV_CNT];

volatile bool need_kill = false;
void handler(int signal) {
    (void)signal;
    need_kill = true;
}

int main() {
    printf_("======================= crash test ============================");
#ifdef MOCK_NVM
    std::string engine_path =
        std::string("/tmp/ramdisk/data/test-") + std::to_string(asm_rdtsc());
#else
    std::string engine_path = "/dev/dax0.0";
#endif
    printf("open engine_path: %s\n", engine_path.c_str());

    for (int i = 0; i < KV_CNT; ++i) {
        gen_marked_random(k, std::to_string(i) + "-", KEY_SIZE);
        ks[i] = std::string(k);
        gen_random(v, VALUE_SIZE);
        vs[i] = v;
    }

    signal(SIGUSR1, handler);
    pid_t fpid = fork();
    if (fpid == 0) {  // child
        Engine *engine = NULL;
        RetCode ret = Engine::Open(engine_path, &engine);
        assert(ret == kSucc);
        for (int i = 0; i < KV_CNT; ++i) {
            RetCode ret = engine->Write(ks[i], vs[i]);
            assert(ret == kSucc);
            if (i == KV_CNT / 3) {
                kill(getppid(), SIGUSR1);
            }
        }
        delete engine;

    } else if (fpid > 0) {  // me

        while (!need_kill)
            ;

        int res = kill(fpid, 9);
        assert(res == 0);

        waitpid(fpid, &res, 0);
        assert(res > 0);

        // re-open and check
        Engine *engine = NULL;
        RetCode ret = Engine::Open(engine_path, &engine);
        assert(ret == kSucc);
        std::string value;

        int i = 0;
        for (; i <= KV_CNT / 3; ++i) {
            ret = engine->Read(ks[i], &value);
            assert(ret == kSucc);
            assert(value == vs[i]);
        }
        for (; i < KV_CNT; ++i) {
            ret = engine->Read(ks[i], &value);
            if (ret == kSucc) {
                assert(value == vs[i]);
            } else {
                assert(ret == kNotFound);
                break;
            }
        }
        for (; i < KV_CNT; ++i) {
            ret = engine->Read(ks[i], &value);
            assert(ret == kNotFound);
        }
        printf_(
            "======================= crash test pass :) "
            "======================");

    } else {
        assert(false);
    }

    return 0;
};
