#ifndef EDGE_H
#define EDGE_H

#include <string>
#include <sstream>

using namespace std;

class Edge {
public:
    int edgeId;
    int sourceId;
    int destinationId;
    double distance;
    string roadName;
    bool isBidirectional;

    Edge() : edgeId(0), sourceId(0), destinationId(0), distance(0.0), 
             roadName(""), isBidirectional(true) {}

    Edge(int edgeId, int sourceId, int destId, double dist, 
         const string& name, bool bidir = true)
        : edgeId(edgeId), sourceId(sourceId), destinationId(destId),
          distance(dist), roadName(name), isBidirectional(bidir) {}

    string serialize() const {
        ostringstream oss;
        oss << sourceId << "|" << destinationId << "|" << distance << "|" 
            << roadName << "|" << (isBidirectional ? "1" : "0");
        return oss.str();
    }

    static Edge deserialize(int edgeId, const string& data) {
        Edge edge;
        edge.edgeId = edgeId;
        
        istringstream iss(data);
        string token;
        
        if (getline(iss, token, '|')) edge.sourceId = stoi(token);
        if (getline(iss, token, '|')) edge.destinationId = stoi(token);
        if (getline(iss, token, '|')) edge.distance = stod(token);
        if (getline(iss, token, '|')) edge.roadName = token;
        if (getline(iss, token, '|')) edge.isBidirectional = (token == "1");
        
        return edge;
    }

    string toString() const {
        ostringstream oss;
        oss << "EdgeID: " << edgeId << ", " << sourceId << " -> " << destinationId
            << ", Distance: " << distance << " km, Road: " << roadName
            << ", " << (isBidirectional ? "Bidirectional" : "One-way");
        return oss.str();
    }

    bool isValid() const {
        return edgeId > 0 && sourceId > 0 && destinationId > 0 && 
               distance > 0.0 && sourceId != destinationId;
    }
};

#endif
