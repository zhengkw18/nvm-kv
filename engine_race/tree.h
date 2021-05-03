#ifndef ENGINE_RACE_ENGINE_TREE_H_
#define ENGINE_RACE_ENGINE_TREE_H_

#include "utils.h"
#include <vector>
#include <tuple>

using polar_race::PolarString;
using polar_race::Visitor;

class Snapshot;
class Tree
{
public:
    Tree() {}
    Tree(const i8 *raw, u32 &sz) {}
    virtual ~Tree(){};
    virtual u32 get_size() = 0;
    virtual void dump(i8 *raw) = 0;
    virtual bool read(const PolarString &key, u32 &val_pos, u32 &val_len) = 0;
    virtual bool read(const PolarString &key, u32 &val_pos, u32 &val_len, u32 max_seq) = 0;
    virtual void insert(const PolarString &key, u32 seq, u32 val_pos, u32 val_len) = 0;
    virtual void range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr) = 0;
    virtual void range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr, u32 max_seq) = 0;
};

#endif