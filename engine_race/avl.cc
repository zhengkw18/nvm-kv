#include "avl.h"
#include <cassert>
AVL::AVL()
{
    root = -1;
    node_cnt = data_cnt = 0;
    node_sz = data_sz = 1;
    nodes = new TreeNode[node_sz];
    datas = new TreeData[data_sz];
}
AVL::AVL(Tree *tree)
{
    AVL *other = (AVL *)tree;
    node_sz = other->node_sz;
    data_sz = other->data_sz;
    node_cnt = other->node_cnt;
    data_cnt = other->data_cnt;
    root = other->root;
    nodes = new TreeNode[node_sz];
    datas = new TreeData[data_sz];
    memcpy(nodes, other->nodes, node_cnt * sizeof(TreeNode));
    memcpy(datas, other->datas, data_cnt * sizeof(TreeData));
}
AVL::AVL(const i8 *raw, u32 &sz)
{
    u32 *_raw = (u32 *)raw;
    node_sz = _raw[0];
    data_sz = _raw[1];
    node_cnt = _raw[2];
    data_cnt = _raw[3];
    root = _raw[4];
    _raw += 5;
    nodes = new TreeNode[node_sz];
    datas = new TreeData[data_sz];
    TreeNode *__raw = (TreeNode *)_raw;
    memcpy(nodes, __raw, node_cnt * sizeof(TreeNode));
    __raw += node_cnt;
    memcpy(datas, __raw, data_cnt * sizeof(TreeData));
    sz = 5 * sizeof(u32) + node_cnt * sizeof(TreeNode) + data_cnt * sizeof(TreeData);
}
AVL::~AVL()
{
    delete[] nodes;
    delete[] datas;
}
u32 AVL::get_size()
{
    return 5 * sizeof(u32) + node_cnt * sizeof(TreeNode) + data_cnt * sizeof(TreeData);
}
void AVL::dump(i8 *raw)
{
    u32 *_raw = (u32 *)raw;
    _raw[0] = node_sz;
    _raw[1] = data_sz;
    _raw[2] = node_cnt;
    _raw[3] = data_cnt;
    _raw[4] = root;
    _raw += 5;
    TreeNode *__raw = (TreeNode *)_raw;
    memcpy(__raw, nodes, node_cnt * sizeof(TreeNode));
    __raw += node_cnt;
    memcpy(__raw, datas, data_cnt * sizeof(TreeData));
}
bool AVL::read(const PolarString &key, u32 &val_len, u32 &val_pos)
{
    i32 cur = root;
    while (cur != -1)
    {
        TreeNode &_cur = nodes[cur];
        i32 res = key.compare(PolarString(_cur.key, _cur.key_len));
        if (res == 0)
        {
            break;
        }
        cur = res < 0 ? _cur.left : _cur.right;
    }
    if (cur == -1)
        return false;
    i32 pos = nodes[cur].data_head;
    val_len = datas[pos].val_len;
    val_pos = datas[pos].pos;
    return true;
}
void AVL::insert(const PolarString &key, u32 val_len, u32 val_pos)
{
    i32 new_node, delta;
    _insert(root, key, new_node, delta);
    TreeNode &_new_node = nodes[new_node];
    TreeData *__new_data;
    if (_new_node.data_head == -1)
    {
        i32 new_data = _new_data();
        __new_data = &datas[new_data];
        _new_node.data_head = new_data;
    }
    else
    {
        __new_data = &datas[_new_node.data_head];
    }
    __new_data->val_len = val_len;
    __new_data->pos = val_pos;
}
void AVL::range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr)
{
    _range(lower, upper, arr, root);
}
bool AVL::_insert(i32 &root, const PolarString &key, i32 &new_node, i32 &delta)
{
    if (root == -1)
    {
        root = new_node = _new_node();
        TreeNode &node_new = nodes[root];
        node_new.left = node_new.right = node_new.data_head = -1;
        node_new.balance_factor = 0;
        node_new.key_len = key.size();
        memcpy(node_new.key, key.data(), node_new.key_len);
        delta = 1;
        return false;
    }
    TreeNode &__root = nodes[root];
    i32 res = key.compare(PolarString(__root.key, __root.key_len));
    res = res < 0 ? -1 : res > 0 ? 1
                                 : 0;
    i32 sub_delta;
    bool hit;
    i32 child;
    if (res == 0)
    {
        new_node = root;
        return true;
    }
    else
    {
        child = res == -1 ? __root.left : __root.right;
        hit = _insert(child, key, new_node, sub_delta);
    }
    TreeNode &_root = nodes[root];
    (res == -1 ? _root.left : _root.right) = child;
    if (hit)
        return true;

    sub_delta *= res;

    _root.balance_factor += sub_delta;

    if (sub_delta == 0 || _root.balance_factor == 0)
    {
        delta = 0;
    }
    else if (sub_delta == _root.balance_factor)
    {
        delta = 1;
    }
    else
    {
        delta = 0;
        if (_root.balance_factor < -1)
        {
            i32 &p = _root.left;
            TreeNode &_p = nodes[p];
            if (_p.balance_factor == -1)
            {
                // R rotation
                i32 old_root = root;
                root = p;
                i32 &suBPlus = _p.right;
                p = suBPlus;
                suBPlus = old_root;
                _p.balance_factor = _root.balance_factor = 0;
            }
            else if (_p.balance_factor == 1)
            {
                // LR rotation
                i32 old_root = root;
                root = _p.right;
                TreeNode &_root_new = nodes[root];
                i32 old_p = p;
                p = _root_new.right;
                _p.right = _root_new.left;
                _root_new.right = old_root;
                _root_new.left = old_p;
                _p.balance_factor = _root_new.balance_factor == -1 ? 0 : -1;
                _root.balance_factor = _root_new.balance_factor == -1 ? 1 : 0;
                _root_new.balance_factor = 0;
            }
            else
            {
                assert(false);
            }
        }
        else if (_root.balance_factor > 1)
        {
            i32 &p = _root.right;
            TreeNode &_p = nodes[p];
            if (_p.balance_factor == -1)
            {
                // RL rotation
                i32 old_root = root;
                root = _p.left;
                TreeNode &_root_new = nodes[root];
                i32 old_p = p;
                p = _root_new.left;
                _p.left = _root_new.right;
                _root_new.left = old_root;
                _root_new.right = old_p;
                _root.balance_factor = _root_new.balance_factor == -1 ? 0 : -1;
                _p.balance_factor = _root_new.balance_factor == -1 ? 1 : 0;
                _root_new.balance_factor = 0;
            }
            else if (_p.balance_factor == 1)
            {
                // L rotation
                i32 old_root = root;
                root = p;
                i32 &suBPlus = _p.left;
                p = suBPlus;
                suBPlus = old_root;
                _p.balance_factor = _root.balance_factor = 0;
            }
            else
            {
                assert(false);
            }
        }
        else
        {
            assert(false);
        }
    }
    return false;
}

void AVL::_range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr, i32 current)
{
    if (current == -1)
        return;
    TreeNode &_current = nodes[current];
    std::string s(_current.key, _current.key_len);
    PolarString key(s);
    i32 res1 = lower.empty() ? 1 : key.compare(lower);
    i32 res2 = upper.empty() ? -1 : key.compare(upper);
    TreeData &_data = datas[_current.data_head];
    if (res1 > 0)
        _range(lower, upper, arr, _current.left);
    if (res1 >= 0 && res2 < 0)
        arr.push_back(std::make_tuple(std::move(s), _data.pos, _data.val_len));
    if (res2 < 0)
        _range(lower, upper, arr, _current.right);
}

i32 AVL::_new_node()
{
    if (node_cnt == node_sz)
    {
        TreeNode *new_nodes = new TreeNode[node_sz * 2];
        memcpy(new_nodes, nodes, node_sz * sizeof(TreeNode));
        delete[] nodes;
        nodes = new_nodes;
        node_sz *= 2;
    }
    return node_cnt++;
}
i32 AVL::_new_data()
{
    if (data_cnt == data_sz)
    {
        TreeData *new_datas = new TreeData[data_sz * 2];
        memcpy(new_datas, datas, data_sz * sizeof(TreeData));
        delete[] datas;
        datas = new_datas;
        data_sz *= 2;
    }
    return data_cnt++;
}

inline void AVL::_print()
{
    printf("nodes size %d cnt %d\n", node_sz, node_cnt);
    printf("datas size %d cnt %d\n", data_sz, data_cnt);
    for (int i = 0; i < node_cnt; i++)
    {
        printf("Node %d: left %d right %d dataheader %d balance %d\n", i, nodes[i].left, nodes[i].right, nodes[i].data_head, nodes[i].balance_factor);
    }
    for (int i = 0; i < data_cnt; i++)
    {
        printf("Data %d: pos %d len %d\n", i, datas[i].pos, datas[i].val_len);
    }
}