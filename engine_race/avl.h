#ifndef ENGINE_RACE_ENGINE_AVL_H_
#define ENGINE_RACE_ENGINE_AVL_H_

#include "tree.h"

using polar_race::PolarString;
using polar_race::Visitor;

struct TreeNode
{
    i32 left;
    i32 right;
    i32 balance_factor; //height(right)-height(left)
    i32 data_head;
    u32 key_len = 0;
    i8 key[MAX_KEY_LENGTH] = {0};
};

struct TreeData
{
    u32 seq;
    u32 val_len = 0;
    u32 pos;
    bool operator<(const TreeData &other) const
    {
        return seq < other.seq;
    }
};

class AVL : public Tree
{
public:
    AVL();
    AVL(const i8 *raw, u32 &sz);
    ~AVL();
    u32 get_size();
    void dump(i8 *raw);
    bool read(const PolarString &key, u32 &val_pos, u32 &val_len);
    bool read(const PolarString &key, u32 &val_pos, u32 &val_len, u32 max_seq);
    void insert(const PolarString &key, u32 seq, u32 val_pos, u32 val_len);
    void range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr);
    void range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr, u32 max_seq);

private:
    u32 node_sz, data_sz, node_cnt, data_cnt;
    TreeNode *nodes;
    TreeData *datas;
    std::vector<std::vector<TreeData>> history;
    i32 root;
    bool _insert(i32 &root, const PolarString &key, i32 &new_node, i32 &delta);
    void _range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr, i32 current);
    void _range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr, i32 current, u32 max_seq);
    inline i32 _new_node();
    inline i32 _new_data();
    inline void _print();
};

#endif