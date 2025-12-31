#ifndef GRAPH_H
#define GRAPH_H

#include "Location.h"
#include <map>
#include <vector>
#include <utility>

using namespace std;

struct Neighbor {
    int nodeId;
    double distance;
    
    Neighbor(int id, double dist) : nodeId(id), distance(dist) {}
};

class Graph {
private:
    map<int, Location> nodes;
    map<int, vector<Neighbor>> adjacencyList;

public:
    Graph();
    ~Graph();

    void addNode(const Location& location);
    void addEdge(int sourceId, int destId, double distance, bool bidirectional = true);
    bool nodeExists(int nodeId) const;
    Location getNode(int nodeId) const;
    vector<Neighbor> getNeighbors(int nodeId) const;
    vector<Location> getAllNodes() const;
    int getNodeCount() const { return nodes.size(); }
    int getEdgeCount() const;
    bool isEmpty() const { return nodes.empty(); }
    void clear();
    void printGraph() const;
};

#endif
