#ifndef BTREE_H
#define BTREE_H

#include "BTreeNode.h"
#include <string>
#include <vector>
#include <utility>

using namespace std;

class BTree {
private:
    BTreeNode* root;
    int nodeCounter;

    void assignNodeIds(BTreeNode* node, vector<BTreeNode*>& nodes);
    void collectNodes(BTreeNode* node, vector<BTreeNode*>& nodes);

public:
    BTree();
    ~BTree();

    string search(int key);
    void insert(int key, const string& value);
    bool exists(int key);
    bool update(int key, const string& value);
    vector<pair<int, string>> traverseAll();
    int getCount();
    int getMaxKey();
    bool isEmpty() const { return root == nullptr || root->numKeys == 0; }
    bool saveToFile(const string& filename);
    bool loadFromFile(const string& filename);
    void clear();
};

#endif
