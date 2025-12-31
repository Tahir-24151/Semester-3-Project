#include "../BTreeNode.h"
#include <sstream>
#include <algorithm>

using namespace std;

BTreeNode::BTreeNode(bool leaf) : isLeaf(leaf), numKeys(0), nodeId(-1) {
    for (int i = 0; i <= MAX_KEYS; i++) {
        children[i] = nullptr;
    }
}

BTreeNode::~BTreeNode() {
    if (!isLeaf) {
        for (int i = 0; i <= numKeys; i++) {
            delete children[i];
            children[i] = nullptr;
        }
    }
}

string BTreeNode::search(int key) {
    int i = 0;
    while (i < numKeys && key > keys[i]) {
        i++;
    }

    if (i < numKeys && keys[i] == key) {
        return values[i];
    }

    if (isLeaf) {
        return "";
    }

    return children[i]->search(key);
}

int BTreeNode::findKey(int key) {
    int idx = 0;
    while (idx < numKeys && keys[idx] < key) {
        idx++;
    }
    return idx;
}

void BTreeNode::insertNonFull(int key, const string& value) {
    int i = numKeys - 1;

    if (isLeaf) {
        while (i >= 0 && keys[i] > key) {
            keys[i + 1] = keys[i];
            values[i + 1] = values[i];
            i--;
        }

        keys[i + 1] = key;
        values[i + 1] = value;
        numKeys++;
    } else {
        while (i >= 0 && keys[i] > key) {
            i--;
        }
        i++;

        if (children[i]->isFull()) {
            splitChild(i);
            
            if (keys[i] < key) {
                i++;
            }
        }

        children[i]->insertNonFull(key, value);
    }
}

void BTreeNode::splitChild(int index) {
    BTreeNode* fullChild = children[index];
    BTreeNode* newChild = new BTreeNode(fullChild->isLeaf);
    
    int mid = BTREE_ORDER - 1;
    
    newChild->numKeys = BTREE_ORDER - 1;
    for (int j = 0; j < BTREE_ORDER - 1; j++) {
        newChild->keys[j] = fullChild->keys[mid + 1 + j];
        newChild->values[j] = fullChild->values[mid + 1 + j];
    }

    if (!fullChild->isLeaf) {
        for (int j = 0; j < BTREE_ORDER; j++) {
            newChild->children[j] = fullChild->children[mid + 1 + j];
            fullChild->children[mid + 1 + j] = nullptr;
        }
    }

    fullChild->numKeys = BTREE_ORDER - 1;

    for (int j = numKeys; j > index; j--) {
        children[j + 1] = children[j];
    }
    children[index + 1] = newChild;

    for (int j = numKeys - 1; j >= index; j--) {
        keys[j + 1] = keys[j];
        values[j + 1] = values[j];
    }

    keys[index] = fullChild->keys[mid];
    values[index] = fullChild->values[mid];
    numKeys++;
}

void BTreeNode::traverse(vector<pair<int, string>>& result) {
    int i;
    for (i = 0; i < numKeys; i++) {
        if (!isLeaf && children[i] != nullptr) {
            children[i]->traverse(result);
        }
        result.push_back({keys[i], values[i]});
    }

    if (!isLeaf && children[i] != nullptr) {
        children[i]->traverse(result);
    }
}

string BTreeNode::serialize() const {
    ostringstream oss;
    
    oss << nodeId << "|" << (isLeaf ? "1" : "0") << "|" << numKeys << "|";
    
    for (int i = 0; i < numKeys; i++) {
        if (i > 0) oss << ",";
        oss << keys[i];
    }
    oss << "|";
    
    for (int i = 0; i < numKeys; i++) {
        if (i > 0) oss << "~";
        string escapedValue = values[i];
        size_t pos = 0;
        while ((pos = escapedValue.find('|', pos)) != string::npos) {
            escapedValue.replace(pos, 1, "\\p");
            pos += 2;
        }
        oss << escapedValue;
    }
    oss << "|";
    
    if (!isLeaf) {
        for (int i = 0; i <= numKeys; i++) {
            if (i > 0) oss << ",";
            oss << (children[i] ? children[i]->nodeId : -1);
        }
    }
    
    return oss.str();
}
