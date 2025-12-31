#include "../Navigation.h"
#include <queue>
#include <map>
#include <set>
#include <limits>
#include <cmath>
#include <algorithm>

using namespace std;

Navigation::Navigation(Graph* g) : graph(g) {}

Navigation::~Navigation() {}

vector<int> Navigation::reconstructPath(const map<int, int>& previous, int start, int end) {
    vector<int> path;
    int current = end;
    
    while (current != -1) {
        path.push_back(current);
        auto it = previous.find(current);
        if (it != previous.end()) {
            current = it->second;
        } else {
            break;
        }
    }
    
    reverse(path.begin(), path.end());
    return path;
}

PathResult Navigation::dijkstra(int sourceId, int destinationId) {
    PathResult result;
    
    if (graph == nullptr || graph->isEmpty()) {
        result.errorMessage = "Graph is empty. Add locations first.";
        return result;
    }
    
    if (!graph->nodeExists(sourceId)) {
        result.errorMessage = "Source location ID " + to_string(sourceId) + " does not exist.";
        return result;
    }
    
    if (!graph->nodeExists(destinationId)) {
        result.errorMessage = "Destination location ID " + to_string(destinationId) + " does not exist.";
        return result;
    }
    
    if (sourceId == destinationId) {
        result.found = true;
        result.path.push_back(sourceId);
        result.totalDistance = 0.0;
        return result;
    }
    
    const double INFINITY_DIST = numeric_limits<double>::infinity();
    
    map<int, double> distances;
    map<int, int> previous;
    set<int> visited;
    
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    
    for (const Location& loc : graph->getAllNodes()) {
        distances[loc.id] = INFINITY_DIST;
        previous[loc.id] = -1;
    }
    
    distances[sourceId] = 0;
    pq.push({0.0, sourceId});
    
    while (!pq.empty()) {
        auto [currentDist, currentNode] = pq.top();
        pq.pop();
        
        if (visited.count(currentNode)) {
            continue;
        }
        
        if (currentNode == destinationId) {
            result.found = true;
            result.path = reconstructPath(previous, sourceId, destinationId);
            result.totalDistance = distances[destinationId];
            return result;
        }
        
        visited.insert(currentNode);
        
        for (const Neighbor& neighbor : graph->getNeighbors(currentNode)) {
            if (visited.count(neighbor.nodeId)) {
                continue;
            }
            
            double newDist = currentDist + neighbor.distance;
            
            if (newDist < distances[neighbor.nodeId]) {
                distances[neighbor.nodeId] = newDist;
                previous[neighbor.nodeId] = currentNode;
                pq.push({newDist, neighbor.nodeId});
            }
        }
    }
    
    result.errorMessage = "No path found from " + graph->getNode(sourceId).name + 
                          " to " + graph->getNode(destinationId).name + ".";
    return result;
}

vector<string> Navigation::getDirections(const PathResult& result) {
    vector<string> directions;
    
    if (!result.found || result.path.size() < 2) {
        if (!result.found) {
            directions.push_back("No route available.");
        } else {
            directions.push_back("You are already at your destination.");
        }
        return directions;
    }
    
    directions.push_back("=== Navigation Directions ===");
    directions.push_back("Start at: " + graph->getNode(result.path[0]).name);
    
    for (size_t i = 0; i < result.path.size() - 1; i++) {
        int fromId = result.path[i];
        int toId = result.path[i + 1];
        
        Location from = graph->getNode(fromId);
        Location to = graph->getNode(toId);
        
        double segmentDist = 0.0;
        for (const Neighbor& n : graph->getNeighbors(fromId)) {
            if (n.nodeId == toId) {
                segmentDist = n.distance;
                break;
            }
        }
        
        string step = to_string(i + 1) + ". Go from " + from.name + 
                      " to " + to.name + " (" + 
                      to_string(segmentDist) + " km)";
        directions.push_back(step);
    }
    
    directions.push_back("Arrive at: " + graph->getNode(result.path.back()).name);
    directions.push_back("Total distance: " + to_string(result.totalDistance) + " km");
    directions.push_back("=============================");
    
    return directions;
}

double Navigation::haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;
    const double PI = 3.14159265358979323846;
    
    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;
    
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
               sin(dLon / 2) * sin(dLon / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c;
}
