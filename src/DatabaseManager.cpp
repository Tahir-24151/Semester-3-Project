#include "../DatabaseManager.h"
#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

DatabaseManager::DatabaseManager(const string& dataDir)
    : locationBTree(nullptr), edgeBTree(nullptr), graph(nullptr),
      dataDirectory(dataDir), nextLocationId(1), nextEdgeId(1), dataModified(false) {
    
    locationFile = dataDirectory + "/locations_btree.dat";
    edgeFile = dataDirectory + "/edges_btree.dat";
}

DatabaseManager::~DatabaseManager() {
    if (dataModified) {
        saveData();
    }
    
    delete locationBTree;
    delete edgeBTree;
    delete graph;
}

bool DatabaseManager::dataFilesExist() {
    return fs::exists(locationFile) && fs::exists(edgeFile);
}

bool DatabaseManager::initialize() {
    if (!fs::exists(dataDirectory)) {
        fs::create_directories(dataDirectory);
    }
    
    locationBTree = new BTree();
    edgeBTree = new BTree();
    graph = new Graph();
    
    if (dataFilesExist()) {
        return loadData();
    } else {
        cout << "No existing data files found. Starting with empty database." << endl;
        return true;
    }
}

bool DatabaseManager::loadData() {
    bool success = true;
    
    if (!locationBTree->loadFromFile(locationFile)) {
        cerr << "Warning: Could not load locations file." << endl;
        success = false;
    }
    
    if (!edgeBTree->loadFromFile(edgeFile)) {
        cerr << "Warning: Could not load edges file." << endl;
        success = false;
    }
    
    nextLocationId = locationBTree->getMaxKey() + 1;
    nextEdgeId = edgeBTree->getMaxKey() + 1;
    
    buildGraph();
    
    cout << "Data loaded: " << getLocationCount() << " locations, " 
         << getEdgeCount() << " roads." << endl;
    
    return success;
}

bool DatabaseManager::saveData() {
    bool success = true;
    
    if (!locationBTree->saveToFile(locationFile)) {
        cerr << "Error: Could not save locations file." << endl;
        success = false;
    }
    
    if (!edgeBTree->saveToFile(edgeFile)) {
        cerr << "Error: Could not save edges file." << endl;
        success = false;
    }
    
    if (success) {
        dataModified = false;
        cout << "Data saved successfully." << endl;
    }
    
    return success;
}

int DatabaseManager::addLocation(const string& name, double lat, double lon, const string& type) {
    Location loc(nextLocationId, name, lat, lon, type);
    
    if (!loc.isValid()) {
        return -1;
    }
    
    locationBTree->insert(loc.id, loc.serialize());
    graph->addNode(loc);
    
    dataModified = true;
    return nextLocationId++;
}

bool DatabaseManager::addLocation(const Location& location) {
    if (!location.isValid()) {
        return false;
    }
    
    locationBTree->insert(location.id, location.serialize());
    graph->addNode(location);
    
    if (location.id >= nextLocationId) {
        nextLocationId = location.id + 1;
    }
    
    dataModified = true;
    return true;
}

int DatabaseManager::addEdge(int sourceId, int destId, double distance, 
                             const string& roadName, bool bidirectional) {
    if (!locationExists(sourceId) || !locationExists(destId)) {
        return -1;
    }
    
    Edge edge(nextEdgeId, sourceId, destId, distance, roadName, bidirectional);
    
    if (!edge.isValid()) {
        return -1;
    }
    
    edgeBTree->insert(edge.edgeId, edge.serialize());
    graph->addEdge(sourceId, destId, distance, bidirectional);
    
    dataModified = true;
    return nextEdgeId++;
}

bool DatabaseManager::addEdge(const Edge& edge) {
    if (!edge.isValid()) {
        return false;
    }
    
    edgeBTree->insert(edge.edgeId, edge.serialize());
    graph->addEdge(edge.sourceId, edge.destinationId, edge.distance, edge.isBidirectional);
    
    if (edge.edgeId >= nextEdgeId) {
        nextEdgeId = edge.edgeId + 1;
    }
    
    dataModified = true;
    return true;
}

Location DatabaseManager::getLocation(int locationId) {
    string data = locationBTree->search(locationId);
    if (data.empty()) {
        return Location();
    }
    return Location::deserialize(locationId, data);
}

Edge DatabaseManager::getEdge(int edgeId) {
    string data = edgeBTree->search(edgeId);
    if (data.empty()) {
        return Edge();
    }
    return Edge::deserialize(edgeId, data);
}

bool DatabaseManager::locationExists(int locationId) {
    return locationBTree->exists(locationId);
}

bool DatabaseManager::edgeExists(int edgeId) {
    return edgeBTree->exists(edgeId);
}

vector<Location> DatabaseManager::getAllLocations() {
    vector<Location> locations;
    auto data = locationBTree->traverseAll();
    
    for (const auto& pair : data) {
        locations.push_back(Location::deserialize(pair.first, pair.second));
    }
    
    return locations;
}

vector<Edge> DatabaseManager::getAllEdges() {
    vector<Edge> edges;
    auto data = edgeBTree->traverseAll();
    
    for (const auto& pair : data) {
        edges.push_back(Edge::deserialize(pair.first, pair.second));
    }
    
    return edges;
}

int DatabaseManager::getLocationCount() {
    return locationBTree->getCount();
}

int DatabaseManager::getEdgeCount() {
    return edgeBTree->getCount();
}

void DatabaseManager::buildGraph() {
    graph->clear();
    
    for (const Location& loc : getAllLocations()) {
        graph->addNode(loc);
    }
    
    for (const Edge& edge : getAllEdges()) {
        graph->addEdge(edge.sourceId, edge.destinationId, edge.distance, edge.isBidirectional);
    }
}

void DatabaseManager::clearAll() {
    locationBTree->clear();
    edgeBTree->clear();
    graph->clear();
    nextLocationId = 1;
    nextEdgeId = 1;
    dataModified = true;
}

void DatabaseManager::initializeSampleData() {
    clearAll();
    
    cout << "Initializing sample map data..." << endl;
    
    addLocation("Central Station", 40.7128, -74.0060, "station");
    addLocation("North Park", 40.7200, -74.0000, "park");
    addLocation("East Market", 40.7100, -73.9950, "market");
    addLocation("West Plaza", 40.7150, -74.0150, "plaza");
    addLocation("South Mall", 40.7050, -74.0100, "mall");
    
    addEdge(1, 2, 1.2, "North Street", true);
    addEdge(1, 3, 1.5, "East Avenue", true);
    addEdge(1, 4, 0.9, "West Road", true);
    addEdge(2, 3, 1.8, "Park Lane", true);
    addEdge(2, 4, 2.0, "Cross Street", true);
    addEdge(3, 5, 1.3, "Market Road", true);
    addEdge(4, 5, 1.1, "Plaza Way", true);
    
    saveData();
    
    cout << "Sample data initialized:" << endl;
    cout << "- 5 locations added" << endl;
    cout << "- 7 bidirectional roads added" << endl;
}
