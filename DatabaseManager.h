#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "BTree.h"
#include "Graph.h"
#include "Location.h"
#include "Edge.h"
#include <string>
#include <vector>

using namespace std;

class DatabaseManager {
private:
    BTree* locationBTree;
    BTree* edgeBTree;
    Graph* graph;
    
    string dataDirectory;
    string locationFile;
    string edgeFile;
    
    int nextLocationId;
    int nextEdgeId;
    bool dataModified;

public:
    DatabaseManager(const string& dataDir = "data");
    ~DatabaseManager();

    bool initialize();
    bool saveData();
    bool loadData();
    bool dataFilesExist();

    int addLocation(const string& name, double lat, double lon, const string& type);
    bool addLocation(const Location& location);
    int addEdge(int sourceId, int destId, double distance, 
                const string& roadName, bool bidirectional = true);
    bool addEdge(const Edge& edge);

    Location getLocation(int locationId);
    Edge getEdge(int edgeId);
    bool locationExists(int locationId);
    bool edgeExists(int edgeId);
    vector<Location> getAllLocations();
    vector<Edge> getAllEdges();
    int getLocationCount();
    int getEdgeCount();

    void buildGraph();
    Graph* getGraph() { return graph; }
    bool isModified() const { return dataModified; }
    void initializeSampleData();
    void clearAll();
};

#endif
