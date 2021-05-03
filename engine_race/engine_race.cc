// Copyright [2018] Alibaba Cloud All rights reserved
#include "engine_race.h"

#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <tuple>

namespace polar_race
{

    RetCode Engine::Open(const std::string &name, Engine **eptr)
    {
        return EngineRace::Open(name, eptr);
    }

    Engine::~Engine() {}

    /*
 * Complete the functions below to implement you own engine
 */

    // 1. Open engine
    RetCode EngineRace::Open(const std::string &name, Engine **eptr)
    {
        *eptr = new EngineRace(name);
        return kSucc;
    }
    EngineRace::EngineRace(const std::string &name)
    {
        pthread_rwlock_init(&rwlock, nullptr);
        for (u32 i = 0; i < HASH_SIZE; i++)
        {
            pthread_rwlock_init(&treelock[i], nullptr);
        }
        file_fd = open(name.c_str(), O_RDWR | O_CREAT, 0666);
        struct stat st;
        fstat(file_fd, &st);
        if (st.st_size == 0)
        {
            ftruncate(file_fd, INITIAL_FILE_SIZE);
            file_map = (i8 *)mmap(nullptr, INITIAL_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
            current_size = INITIAL_FILE_SIZE;
            current_seq = 0;
            current_offset = 2 * sizeof(u32);
            u32 *header = (u32 *)file_map;
            header[0] = current_size;
            header[1] = current_seq;
            for (u32 i = 0; i < HASH_SIZE; i++)
            {
                switch (TREE_TYPE)
                {
                case AVL_:
                    tree[i] = new AVL();
                    break;
                case BPULS_:
                    tree[i] = new BPlus();
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            file_map = (i8 *)mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
            u32 *header = (u32 *)file_map;
            current_size = header[0];
            current_seq = header[1];
            current_offset = 2 * sizeof(u32);
            if (current_size > st.st_size)
                current_size = st.st_size;
            bool insert = st.st_size == current_size;
            if (!insert)
            {
                i8 *pos = file_map + current_size;
                u32 sz;
                for (u32 i = 0; i < HASH_SIZE; i++)
                {
                    switch (TREE_TYPE)
                    {
                    case AVL_:
                        tree[i] = new AVL(pos, sz);
                        break;
                    case BPULS_:
                        tree[i] = new BPlus(pos, sz);
                        break;
                    default:
                        break;
                    }
                    pos += sz;
                }
            }
            else
            {
                for (u32 i = 0; i < HASH_SIZE; i++)
                {
                    switch (TREE_TYPE)
                    {
                    case AVL_:
                        tree[i] = new AVL();
                        break;
                    case BPULS_:
                        tree[i] = new BPlus();
                        break;
                    default:
                        break;
                    }
                }
            }
            header += 2;
            i8 *log_map = (i8 *)header;

            for (u32 i = 0; i < current_seq; i++)
            {
                header = (u32 *)log_map;
                u32 key_len = header[1], val_len = header[2], valid = header[0];
                header += 3;
                log_map = (i8 *)header;
                PolarString key = PolarString(log_map, key_len), val = PolarString(log_map + key_len, val_len);
                log_map += key_len + val_len;
                current_offset += 3 * sizeof(u32) + key_len + val_len;
                if (insert && valid)
                    tree[hash(key.data())]->insert(key, i, current_offset - val_len, val_len);
            }
        }
    }
    // 2. Close engine
    EngineRace::~EngineRace()
    {
        u32 lens[HASH_SIZE];
        u32 tot_len = 0;
        for (u32 i = 0; i < HASH_SIZE; i++)
        {
            lens[i] = tree[i]->get_size();
            tot_len += lens[i];
        }
        ftruncate(file_fd, current_size + tot_len);
        file_map = (i8 *)mremap(file_map, current_size, current_size + tot_len, MREMAP_MAYMOVE);
        i8 *pos = file_map + current_size;
        for (u32 i = 0; i < HASH_SIZE; i++)
        {
            tree[i]->dump(pos);
            pos += lens[i];
        }
        munmap(file_map, current_size + tot_len);
        close(file_fd);
        for (u32 i = 0; i < HASH_SIZE; i++)
        {
            delete tree[i];
        }
    }

    // 3. Write a key-value pair into engine
    RetCode EngineRace::Write(const PolarString &key, const PolarString &value)
    {
        pthread_rwlock_wrlock(&rwlock);
        u32 offset = current_offset;
        current_offset += 3 * sizeof(u32) + key.size() + value.size();
        u32 _current_offset = current_offset;
        if (current_offset > current_size)
            _expand();
        u32 *header = (u32 *)(file_map + offset);
        header[1] = key.size();
        header[2] = value.size();
        header[0] = 0;
        u32 seq = current_seq;
        ((u32 *)file_map)[1] = ++current_seq;
        pthread_rwlock_unlock(&rwlock);
        i8 *log_map = (i8 *)(header + 3);
        memcpy(log_map, key.data(), key.size());
        memcpy(log_map + key.size(), value.data(), value.size());
        header[0] = 1;
        i32 _hash = hash(key.data());
        pthread_rwlock_wrlock(&treelock[_hash]);
        tree[_hash]->insert(key, seq, _current_offset - value.size(), value.size());
        pthread_rwlock_unlock(&treelock[_hash]);
        return kSucc;
    }

    // 4. Read value of a key
    RetCode EngineRace::Read(const PolarString &key, std::string *value, Snapshot *snapshot = nullptr)
    {
        u32 len, pos;
        i32 _hash = hash(key.data());
        bool res;
        pthread_rwlock_rdlock(&treelock[_hash]);
        if (snapshot)
        {
            res = tree[hash(key.data())]->read(key, pos, len, snapshot->seq);
        }
        else
        {
            res = tree[hash(key.data())]->read(key, pos, len);
        }
        pthread_rwlock_unlock(&treelock[_hash]);

        if (res)
        {
            value->assign(file_map + pos, len);
            return kSucc;
        }
        return kNotFound;
    }

    /*
 * NOTICE: Implement 'Range' in quarter-final,
 *         you can skip it in preliminary.
 */
    // 5. Applies the given Vistor::Visit function to the result
    // of every key-value pair in the key range [first, last),
    // in order
    // lower=="" is treated as a key before all keys in the database.
    // upper=="" is treated as a key after all keys in the database.
    // Therefore the following call will traverse the entire database:
    //   Range("", "", visitor)
    RetCode EngineRace::Range(const PolarString &lower, const PolarString &upper, Visitor &visitor, Snapshot *snapshot = nullptr)
    {
        std::vector<std::tuple<std::string, u32, u32>> arr;
        for (u32 i = 0; i < HASH_SIZE; i++)
        {
            pthread_rwlock_rdlock(&treelock[i]);
            if (snapshot)
            {
                tree[i]->range(lower, upper, arr, snapshot->seq);
            }
            else
            {
                tree[i]->range(lower, upper, arr);
            }
            pthread_rwlock_unlock(&treelock[i]);
        }

        for (auto t : arr)
        {
            std::string key;
            u32 pos, len;
            std::tie(key, pos, len) = t;
            visitor.Visit(PolarString(key), PolarString(file_map + pos, len));
        }
        return kSucc;
    }
    void EngineRace::_expand()
    {
        ((u32 *)file_map)[0] = current_size * 2;
        ftruncate(file_fd, current_size * 2);
        file_map = (i8 *)mremap(file_map, current_size, current_size * 2, MREMAP_MAYMOVE);
        current_size *= 2;
    }
    Snapshot *EngineRace::GetSnapshot()
    {
        Snapshot *snapshot = new Snapshot;
        snapshot->seq = current_seq;
        return snapshot;
    }

} // namespace polar_race
