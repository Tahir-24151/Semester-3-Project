#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "Graph.h"
#include <vector>
#include <string>
#include <utility>
#include <map>

using namespace std;

struct PathResult {
    bool found;
    vector<int> path;
    double totalDistance;
    string errorMessage;
    
    PathResult() : found(false), totalDistance(0.0), errorMessage("") {}
};

class Navigation {
private:
    Graph* graph;
    
    vector<int> reconstructPath(const map<int, int>& previous, int start, int end);

public:
    Navigation(Graph* graph);
    ~Navigation();

    PathResult dijkstra(int sourceId, int destinationId);
    vector<string> getDirections(const PathResult& result);
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2);
};

#endif
