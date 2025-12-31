#ifndef BTREENODE_H
#define BTREENODE_H

#include <string>
#include <vector>

using namespace std;

const int BTREE_ORDER = 3;
const int MAX_KEYS = 2 * BTREE_ORDER - 1;
const int MIN_KEYS = BTREE_ORDER - 1;

class BTreeNode {
public:
    bool isLeaf;
    int numKeys;
    int keys[MAX_KEYS];
    string values[MAX_KEYS];
    BTreeNode* children[MAX_KEYS + 1];
    int nodeId;

    BTreeNode(bool leaf = true);
    ~BTreeNode();

    string search(int key);
    void insertNonFull(int key, const string& value);
    void splitChild(int index);
    void traverse(vector<pair<int, string>>& result);
    int findKey(int key);

    bool isFull() const { return numKeys == MAX_KEYS; }
    bool hasMinKeys() const { return numKeys == MIN_KEYS; }

    string serialize() const;
    static BTreeNode* deserialize(const string& data);
};

#endif
