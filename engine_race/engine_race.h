// Copyright [2018] Alibaba Cloud All rights reserved
#ifndef ENGINE_RACE_ENGINE_RACE_H_
#define ENGINE_RACE_ENGINE_RACE_H_
#include <string>
#include <pthread.h>
#include "include/engine.h"
#include "utils.h"
#include "avl.h"
#include "bplus.h"

namespace polar_race
{

    class Snapshot
    {
    private:
        u32 seq;
        friend class EngineRace;
    };

    class EngineRace : public Engine
    {
    public:
        static RetCode Open(const std::string &name, Engine **eptr);

        explicit EngineRace(const std::string &name);

        ~EngineRace();

        RetCode Write(const PolarString &key, const PolarString &value) override;

        RetCode Read(const PolarString &key, std::string *value, Snapshot *snapshot) override;

        /*
     * NOTICE: Implement 'Range' in quarter-final,
     *         you can skip it in preliminary.
     */
        RetCode Range(const PolarString &lower, const PolarString &upper, Visitor &visitor, Snapshot *snapshot) override;
        Snapshot *GetSnapshot() override;

    private:
        u32 current_seq, current_size, current_offset;
        i8 *file_map;
        i32 file_fd;
        pthread_rwlock_t rwlock;
        pthread_rwlock_t treelock[HASH_SIZE];
        Tree *tree[HASH_SIZE];
        inline void _expand();
    };

} // namespace polar_race

#endif // ENGINE_RACE_ENGINE_RACE_H_
