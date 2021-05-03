#ifndef ENGINE_RACE_ENGINE_BPLUS_H_
#define ENGINE_RACE_ENGINE_BPLUS_H_

#include "tree.h"

using polar_race::PolarString;
using polar_race::Visitor;

struct BPlusMeta
{
    u32 child;
    u32 key_len;
    i8 key[MAX_KEY_LENGTH];
};

struct BPlusNode
{
    i32 is_leaf;
    i32 key_num;
    i32 prev;
    i32 next;
    i32 parent;
    BPlusMeta meta[M];
};

struct BPlusData
{
    u32 seq;
    u32 val_len = 0;
    u32 pos;
    bool operator<(const BPlusData &other) const
    {
        return seq < other.seq;
    }
};

class BPlus : public Tree
{
public:
    BPlus();
    BPlus(const i8 *raw, u32 &sz);
    ~BPlus();
    u32 get_size();
    void dump(i8 *raw);
    bool read(const PolarString &key, u32 &val_pos, u32 &val_len);
    bool read(const PolarString &key, u32 &val_pos, u32 &val_len, u32 max_seq);
    void insert(const PolarString &key, u32 seq, u32 val_pos, u32 val_len);
    void range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr);
    void range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr, u32 max_seq);

private:
    u32 node_sz, data_sz, node_cnt, data_cnt;
    BPlusNode *nodes;
    BPlusData *datas;
    std::vector<std::vector<BPlusData>> history;
    i32 root;
    inline i32 _new_node();
    inline i32 _new_data();
    inline void _print();
    inline i32 _which_child(i32 child, BPlusNode *parent);
    inline bool _lower_bound(const PolarString &key, i32 &n, i32 &k);
};

#endif