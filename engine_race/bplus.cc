#include "bplus.h"
#include <cassert>
BPlus::BPlus()
{
    node_cnt = data_cnt = 0;
    node_sz = data_sz = 1;
    nodes = new BPlusNode[node_sz];
    datas = new BPlusData[data_sz];
    root = _new_node();
    BPlusNode &_root = nodes[root];
    _root.is_leaf = 1;
    _root.key_num = 0;
    _root.parent = -1;
    _root.next = -1;
    _root.prev = -1;
}
BPlus::BPlus(Tree *tree)
{
    BPlus *other = (BPlus *)tree;
    node_sz = other->node_sz;
    data_sz = other->data_sz;
    node_cnt = other->node_cnt;
    data_cnt = other->data_cnt;
    root = other->root;
    nodes = new BPlusNode[node_sz];
    datas = new BPlusData[data_sz];
    memcpy(nodes, other->nodes, node_cnt * sizeof(BPlusNode));
    memcpy(datas, other->datas, data_cnt * sizeof(BPlusData));
}
BPlus::BPlus(const i8 *raw, u32 &sz)
{
    u32 *_raw = (u32 *)raw;
    node_sz = _raw[0];
    data_sz = _raw[1];
    node_cnt = _raw[2];
    data_cnt = _raw[3];
    root = _raw[4];
    _raw += 5;
    nodes = new BPlusNode[node_sz];
    datas = new BPlusData[data_sz];
    BPlusNode *__raw = (BPlusNode *)_raw;
    memcpy(nodes, __raw, node_cnt * sizeof(BPlusNode));
    __raw += node_cnt;
    memcpy(datas, __raw, data_cnt * sizeof(BPlusData));
    sz = 5 * sizeof(u32) + node_cnt * sizeof(BPlusNode) + data_cnt * sizeof(BPlusData);
}
BPlus::~BPlus()
{
    delete[] nodes;
    delete[] datas;
}
u32 BPlus::get_size()
{
    return 5 * sizeof(u32) + node_cnt * sizeof(BPlusNode) + data_cnt * sizeof(BPlusData);
}
void BPlus::dump(i8 *raw)
{
    u32 *_raw = (u32 *)raw;
    _raw[0] = node_sz;
    _raw[1] = data_sz;
    _raw[2] = node_cnt;
    _raw[3] = data_cnt;
    _raw[4] = root;
    _raw += 5;
    BPlusNode *__raw = (BPlusNode *)_raw;
    memcpy(__raw, nodes, node_cnt * sizeof(BPlusNode));
    __raw += node_cnt;
    memcpy(__raw, datas, data_cnt * sizeof(BPlusData));
}
bool BPlus::read(const PolarString &key, u32 &val_len, u32 &val_pos)
{
    i32 node = root;
    while (1)
    {
        BPlusNode &_node = nodes[node];
        if (!_node.is_leaf)
        {
            for (i32 i = _node.key_num - 1; i >= 0; i--)
            {
                if (i == 0 || key.compare(PolarString(_node.meta[i].key, _node.meta[i].key_len)) >= 0)
                {
                    node = _node.meta[i].child;
                    break;
                }
            }
        }
        else
        {
            for (u32 i = 0; i < _node.key_num; i++)
            {
                if (key.compare(PolarString(_node.meta[i].key, _node.meta[i].key_len)) == 0)
                {
                    BPlusData &data = datas[_node.meta[i].child];
                    val_len = data.val_len;
                    val_pos = data.pos;
                    return true;
                }
            }
            break;
        }
    }
    return false;
}
void BPlus::insert(const PolarString &key, u32 val_len, u32 val_pos)
{
    i32 node = root;
    while (1)
    {
        BPlusNode &_node = nodes[node];
        if (!_node.is_leaf)
        {
            for (i32 i = _node.key_num - 1; i >= 0; i--)
            {
                if (i == 0 || key.compare(PolarString(_node.meta[i].key, _node.meta[i].key_len)) >= 0)
                {
                    node = _node.meta[i].child;
                    break;
                }
            }
        }
        else
        {
            i32 pos = 0;
            for (i32 i = _node.key_num - 1; i >= 0; i--)
            {
                i32 res = key.compare(PolarString(_node.meta[i].key, _node.meta[i].key_len));
                if (res == 0)
                {
                    BPlusData &data = datas[_node.meta[i].child];
                    data.val_len = val_len;
                    data.pos = val_pos;
                    return;
                }
                else if (res > 0)
                {
                    pos = i + 1;
                    break;
                }
            }
            for (i32 i = _node.key_num - 1; i >= pos; i--)
            {
                memcpy(&_node.meta[i + 1], &_node.meta[i], sizeof(BPlusMeta));
            }
            BPlusMeta &meta = _node.meta[pos];
            i32 data_head = _new_data();
            BPlusData &data = datas[data_head];
            data.val_len = val_len;
            data.pos = val_pos;
            meta.child = data_head;
            meta.key_len = key.size();
            memcpy(meta.key, key.data(), key.size());

            _node.key_num++;
            BPlusNode *_parent, *__node;
            i32 parent;
            while (nodes[node].key_num == M)
            {
                parent = nodes[node].parent;
                if (parent == -1)
                {
                    parent = _new_node();
                    root = parent;
                    _parent = &nodes[parent];
                    __node = &nodes[node];
                    _parent->is_leaf = 0;
                    _parent->key_num = 1;
                    _parent->parent = -1;
                    _parent->next = -1;
                    _parent->prev = -1;
                    memcpy(&_parent->meta[0], &__node->meta[0], sizeof(BPlusMeta));
                    _parent->meta[0].child = node;
                    __node->parent = root;
                }
                else
                {
                    _parent = &nodes[parent];
                    __node = &nodes[node];
                }
                i32 which_child = _which_child(node, _parent);
                if (which_child == -1)
                    assert(false);
                for (i32 i = _parent->key_num; i > which_child + 1; i--)
                {
                    memcpy(&_parent->meta[i], &_parent->meta[i - 1], sizeof(BPlusMeta));
                }

                _parent->key_num++;
                i32 newnode = _new_node();
                _parent = &nodes[parent];
                __node = &nodes[node];
                _parent->meta[which_child + 1].child = newnode;
                BPlusNode *_newnode = &nodes[newnode];
                _newnode->is_leaf = __node->is_leaf;
                _newnode->key_num = __node->key_num - __node->key_num / 2;
                _newnode->parent = parent;
                __node->key_num /= 2;
                for (u32 i = 0; i < _newnode->key_num; i++)
                {
                    memcpy(&_newnode->meta[i], &__node->meta[i + __node->key_num], sizeof(BPlusMeta));
                    if (!_newnode->is_leaf)
                    {
                        nodes[_newnode->meta[i].child].parent = newnode;
                    }
                }
                if (__node->is_leaf)
                {
                    _newnode->next = __node->next;
                    _newnode->prev = node;
                    if (__node->next != -1)
                    {
                        nodes[__node->next].prev = newnode;
                    }
                    __node->next = newnode;
                }
                _parent->meta[which_child].key_len = __node->meta[0].key_len;
                _parent->meta[which_child + 1].key_len = _newnode->meta[0].key_len;
                memcpy(_parent->meta[which_child].key, __node->meta[0].key, MAX_KEY_LENGTH);
                memcpy(_parent->meta[which_child + 1].key, _newnode->meta[0].key, MAX_KEY_LENGTH);
                node = parent;
            }
            while (node != root)
            {
                __node = &nodes[node];
                parent = __node->parent;
                _parent = &nodes[parent];
                i32 which_child = _which_child(node, _parent);
                if (which_child == -1)
                    assert(false);
                _parent->meta[which_child].key_len = __node->meta[0].key_len;
                memcpy(_parent->meta[which_child].key, __node->meta[0].key, MAX_KEY_LENGTH);
                node = parent;
            }
            break;
        }
    }
}
void BPlus::range(const PolarString &lower, const PolarString &upper, std::vector<std::tuple<std::string, u32, u32>> &arr)
{
    i32 s_n, s_k, e_n, e_k;
    if (_lower_bound(lower, s_n, s_k))
    {
        if (upper.empty() || !_lower_bound(upper, e_n, e_k))
        {
            while (s_n != -1)
            {
                BPlusNode &_node = nodes[s_n];
                BPlusMeta &_meta = _node.meta[s_k];
                BPlusData &_data = datas[_meta.child];
                arr.push_back(std::make_tuple(std::string(_meta.key, _meta.key_len), _data.pos, _data.val_len));
                if (s_k < _node.key_num - 1)
                    s_k++;
                else
                {
                    s_n = _node.next;
                    s_k = 0;
                }
            }
        }
        else
        {
            while (s_n != e_n || s_k != e_k)
            {
                BPlusNode &_node = nodes[s_n];
                BPlusMeta &_meta = _node.meta[s_k];
                BPlusData &_data = datas[_meta.child];
                arr.push_back(std::make_tuple(std::string(_meta.key, _meta.key_len), _data.pos, _data.val_len));
                if (s_k < _node.key_num - 1)
                    s_k++;
                else
                {
                    s_n = _node.next;
                    s_k = 0;
                }
            }
        }
    }
}

i32 BPlus::_new_node()
{
    if (node_cnt == node_sz)
    {
        BPlusNode *new_nodes = new BPlusNode[node_sz * 2];
        memcpy(new_nodes, nodes, node_sz * sizeof(BPlusNode));
        delete[] nodes;
        nodes = new_nodes;
        node_sz *= 2;
    }
    return node_cnt++;
}
i32 BPlus::_new_data()
{
    if (data_cnt == data_sz)
    {
        BPlusData *new_datas = new BPlusData[data_sz * 2];
        memcpy(new_datas, datas, data_sz * sizeof(BPlusData));
        delete[] datas;
        datas = new_datas;
        data_sz *= 2;
    }
    return data_cnt++;
}

void BPlus::_print()
{
}

i32 BPlus::_which_child(i32 child, BPlusNode *parent)
{
    for (u32 i = 0; i < parent->key_num; i++)
    {
        if (parent->meta[i].child == child)
            return i;
    }
    return -1;
}

bool BPlus::_lower_bound(const PolarString &key, i32 &n, i32 &k)
{
    i32 node = root;
    // if empty, return the leftmost leaf
    if (key.empty())
    {
        while (1)
        {
            BPlusNode &_node = nodes[node];
            if (!_node.is_leaf)
            {
                node = _node.meta[0].child;
            }
            else
            {
                if (_node.key_num > 0)
                {
                    n = node;
                    k = 0;
                    return true;
                }
                return false;
            }
        }
    }
    while (1)
    {
        BPlusNode &_node = nodes[node];
        if (!_node.is_leaf)
        {
            for (i32 i = _node.key_num - 1; i >= 0; i--)
            {
                if (i == 0 || key.compare(PolarString(_node.meta[i].key, _node.meta[i].key_len)) >= 0)
                {
                    node = _node.meta[i].child;
                    break;
                }
            }
        }
        else
        {
            n = node;
            k = -1;
            for (u32 i = 0; i < _node.key_num; i++)
            {
                if (key.compare(PolarString(_node.meta[i].key, _node.meta[i].key_len)) <= 0)
                {
                    k = i;
                    break;
                }
            }
            if (k == -1)
            {
                n = nodes[node].next;
                k = 0;
            }
            if (n == -1)
                return false;
            return true;
        }
    }
}