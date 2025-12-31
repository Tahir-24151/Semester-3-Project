#include "../Graph.h"
#include <iostream>
#include <algorithm>

using namespace std;

Graph::Graph() {}

Graph::~Graph() {
    clear();
}

void Graph::clear() {
    nodes.clear();
    adjacencyList.clear();
}

void Graph::addNode(const Location& location) {
    nodes[location.id] = location;
    
    if (adjacencyList.find(location.id) == adjacencyList.end()) {
        adjacencyList[location.id] = vector<Neighbor>();
    }
}

void Graph::addEdge(int sourceId, int destId, double distance, bool bidirectional) {
    if (adjacencyList.find(sourceId) == adjacencyList.end()) {
        adjacencyList[sourceId] = vector<Neighbor>();
    }
    
    bool exists = false;
    for (const auto& neighbor : adjacencyList[sourceId]) {
        if (neighbor.nodeId == destId) {
            exists = true;
            break;
        }
    }
    
    if (!exists) {
        adjacencyList[sourceId].push_back(Neighbor(destId, distance));
    }
    
    if (bidirectional) {
        if (adjacencyList.find(destId) == adjacencyList.end()) {
            adjacencyList[destId] = vector<Neighbor>();
        }
        
        exists = false;
        for (const auto& neighbor : adjacencyList[destId]) {
            if (neighbor.nodeId == sourceId) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            adjacencyList[destId].push_back(Neighbor(sourceId, distance));
        }
    }
}

bool Graph::nodeExists(int nodeId) const {
    return nodes.find(nodeId) != nodes.end();
}

Location Graph::getNode(int nodeId) const {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        return it->second;
    }
    return Location();
}

vector<Neighbor> Graph::getNeighbors(int nodeId) const {
    auto it = adjacencyList.find(nodeId);
    if (it != adjacencyList.end()) {
        return it->second;
    }
    return vector<Neighbor>();
}

vector<Location> Graph::getAllNodes() const {
    vector<Location> result;
    for (const auto& pair : nodes) {
        result.push_back(pair.second);
    }
    return result;
}

int Graph::getEdgeCount() const {
    int count = 0;
    for (const auto& pair : adjacencyList) {
        count += pair.second.size();
    }
    return count;
}

void Graph::printGraph() const {
    cout << "\n=== Graph Structure ===" << endl;
    cout << "Nodes: " << nodes.size() << ", Edges: " << getEdgeCount() << endl;
    
    for (const auto& nodePair : nodes) {
        cout << "\n" << nodePair.second.toString() << endl;
        cout << "  Neighbors: ";
        
        auto it = adjacencyList.find(nodePair.first);
        if (it != adjacencyList.end() && !it->second.empty()) {
            for (const auto& neighbor : it->second) {
                auto neighborNode = nodes.find(neighbor.nodeId);
                string name = (neighborNode != nodes.end()) ? neighborNode->second.name : "Unknown";
                cout << name << "(" << neighbor.distance << "km) ";
            }
        } else {
            cout << "None";
        }
        cout << endl;
    }
    cout << "======================\n" << endl;
}
