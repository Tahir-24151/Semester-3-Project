#include "../BTree.h"
#include <fstream>
#include <sstream>
#include <map>
#include <queue>
#include <algorithm>
#include <functional>

using namespace std;

BTree::BTree() : root(nullptr), nodeCounter(0) {}

BTree::~BTree() {
    clear();
}

void BTree::clear() {
    delete root;
    root = nullptr;
    nodeCounter = 0;
}

string BTree::search(int key) {
    if (root == nullptr) {
        return "";
    }
    return root->search(key);
}

bool BTree::exists(int key) {
    return !search(key).empty();
}

void BTree::insert(int key, const string& value) {
    if (root == nullptr) {
        root = new BTreeNode(true);
        root->keys[0] = key;
        root->values[0] = value;
        root->numKeys = 1;
        return;
    }

    if (exists(key)) {
        update(key, value);
        return;
    }

    if (root->isFull()) {
        BTreeNode* newRoot = new BTreeNode(false);
        newRoot->children[0] = root;
        newRoot->splitChild(0);
        
        int i = 0;
        if (newRoot->keys[0] < key) {
            i = 1;
        }
        newRoot->children[i]->insertNonFull(key, value);
        
        root = newRoot;
    } else {
        root->insertNonFull(key, value);
    }
}

bool BTree::update(int key, const string& value) {
    if (root == nullptr) return false;
    
    function<bool(BTreeNode*)> updateInNode = [&](BTreeNode* node) -> bool {
        int i = 0;
        while (i < node->numKeys && key > node->keys[i]) {
            i++;
        }
        
        if (i < node->numKeys && node->keys[i] == key) {
            node->values[i] = value;
            return true;
        }
        
        if (node->isLeaf) {
            return false;
        }
        
        return updateInNode(node->children[i]);
    };
    
    return updateInNode(root);
}

vector<pair<int, string>> BTree::traverseAll() {
    vector<pair<int, string>> result;
    if (root != nullptr) {
        root->traverse(result);
    }
    return result;
}

int BTree::getCount() {
    return traverseAll().size();
}

int BTree::getMaxKey() {
    if (root == nullptr) return 0;
    
    BTreeNode* current = root;
    while (!current->isLeaf) {
        current = current->children[current->numKeys];
    }
    
    if (current->numKeys > 0) {
        return current->keys[current->numKeys - 1];
    }
    return 0;
}

void BTree::collectNodes(BTreeNode* node, vector<BTreeNode*>& nodes) {
    if (node == nullptr) return;
    
    queue<BTreeNode*> q;
    q.push(node);
    
    while (!q.empty()) {
        BTreeNode* current = q.front();
        q.pop();
        nodes.push_back(current);
        
        if (!current->isLeaf) {
            for (int i = 0; i <= current->numKeys; i++) {
                if (current->children[i] != nullptr) {
                    q.push(current->children[i]);
                }
            }
        }
    }
}

void BTree::assignNodeIds(BTreeNode* node, vector<BTreeNode*>& nodes) {
    nodes.clear();
    collectNodes(node, nodes);
    for (int i = 0; i < (int)nodes.size(); i++) {
        nodes[i]->nodeId = i;
    }
}

bool BTree::saveToFile(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    if (root == nullptr) {
        file << "ORDER=" << BTREE_ORDER << "\n";
        file << "ROOT_INDEX=-1\n";
        file << "NODE_COUNT=0\n";
        file.close();
        return true;
    }
    
    vector<BTreeNode*> nodes;
    assignNodeIds(root, nodes);
    
    file << "ORDER=" << BTREE_ORDER << "\n";
    file << "ROOT_INDEX=0\n";
    file << "NODE_COUNT=" << nodes.size() << "\n";
    file << "\n";
    
    for (BTreeNode* node : nodes) {
        file << "NODE_" << node->nodeId << "|";
        file << "LEAF=" << (node->isLeaf ? "true" : "false") << "|";
        
        file << "KEYS=[";
        for (int i = 0; i < node->numKeys; i++) {
            if (i > 0) file << ",";
            file << node->keys[i];
        }
        file << "]|";
        
        file << "VALUES=[";
        for (int i = 0; i < node->numKeys; i++) {
            if (i > 0) file << "~";
            string escaped = node->values[i];
            for (size_t pos = 0; (pos = escaped.find('|', pos)) != string::npos; pos += 2) {
                escaped.replace(pos, 1, "\\|");
            }
            for (size_t pos = 0; (pos = escaped.find('[', pos)) != string::npos; pos += 2) {
                escaped.replace(pos, 1, "\\[");
            }
            for (size_t pos = 0; (pos = escaped.find(']', pos)) != string::npos; pos += 2) {
                escaped.replace(pos, 1, "\\]");
            }
            file << escaped;
        }
        file << "]";
        
        if (!node->isLeaf) {
            file << "|CHILDREN=[";
            for (int i = 0; i <= node->numKeys; i++) {
                if (i > 0) file << ",";
                file << (node->children[i] ? node->children[i]->nodeId : -1);
            }
            file << "]";
        }
        
        file << "\n";
    }
    
    file.close();
    return true;
}

bool BTree::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    clear();
    
    string line;
    int order = 0, rootIndex = -1, nodeCount = 0;
    
    while (getline(file, line) && !line.empty()) {
        if (line.find("ORDER=") == 0) {
            order = stoi(line.substr(6));
        } else if (line.find("ROOT_INDEX=") == 0) {
            rootIndex = stoi(line.substr(11));
        } else if (line.find("NODE_COUNT=") == 0) {
            nodeCount = stoi(line.substr(11));
        }
    }
    
    if (nodeCount == 0 || rootIndex < 0) {
        file.close();
        return true;
    }
    
    vector<BTreeNode*> nodes(nodeCount, nullptr);
    vector<vector<int>> childIndices(nodeCount);
    
    while (getline(file, line)) {
        if (line.empty() || line.find("NODE_") != 0) continue;
        
        size_t pipePos = line.find('|');
        int nodeId = stoi(line.substr(5, pipePos - 5));
        
        string rest = line.substr(pipePos + 1);
        
        bool isLeaf = rest.find("LEAF=true") != string::npos;
        
        BTreeNode* node = new BTreeNode(isLeaf);
        node->nodeId = nodeId;
        
        size_t keysStart = rest.find("KEYS=[") + 6;
        size_t keysEnd = rest.find("]", keysStart);
        string keysStr = rest.substr(keysStart, keysEnd - keysStart);
        
        if (!keysStr.empty()) {
            istringstream keysStream(keysStr);
            string keyToken;
            int keyIdx = 0;
            while (getline(keysStream, keyToken, ',')) {
                node->keys[keyIdx++] = stoi(keyToken);
            }
            node->numKeys = keyIdx;
        }
        
        size_t valuesStart = rest.find("VALUES=[") + 8;
        size_t valuesEnd = rest.find("]", valuesStart);
        while (valuesEnd > 0 && rest[valuesEnd - 1] == '\\') {
            valuesEnd = rest.find("]", valuesEnd + 1);
        }
        string valuesStr = rest.substr(valuesStart, valuesEnd - valuesStart);
        
        if (!valuesStr.empty()) {
            istringstream valuesStream(valuesStr);
            string valueToken;
            int valIdx = 0;
            while (getline(valuesStream, valueToken, '~')) {
                string unescaped = valueToken;
                for (size_t pos = 0; (pos = unescaped.find("\\|", pos)) != string::npos; pos += 1) {
                    unescaped.replace(pos, 2, "|");
                }
                for (size_t pos = 0; (pos = unescaped.find("\\[", pos)) != string::npos; pos += 1) {
                    unescaped.replace(pos, 2, "[");
                }
                for (size_t pos = 0; (pos = unescaped.find("\\]", pos)) != string::npos; pos += 1) {
                    unescaped.replace(pos, 2, "]");
                }
                node->values[valIdx++] = unescaped;
            }
        }
        
        if (!isLeaf) {
            size_t childStart = rest.find("CHILDREN=[");
            if (childStart != string::npos) {
                childStart += 10;
                size_t childEnd = rest.find("]", childStart);
                string childStr = rest.substr(childStart, childEnd - childStart);
                
                istringstream childStream(childStr);
                string childToken;
                while (getline(childStream, childToken, ',')) {
                    childIndices[nodeId].push_back(stoi(childToken));
                }
            }
        }
        
        nodes[nodeId] = node;
    }
    
    file.close();
    
    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i] != nullptr && !nodes[i]->isLeaf) {
            for (int j = 0; j < (int)childIndices[i].size(); j++) {
                int childIdx = childIndices[i][j];
                if (childIdx >= 0 && childIdx < nodeCount) {
                    nodes[i]->children[j] = nodes[childIdx];
                }
            }
        }
    }
    
    if (rootIndex >= 0 && rootIndex < nodeCount) {
        root = nodes[rootIndex];
    }
    
    return true;
}
