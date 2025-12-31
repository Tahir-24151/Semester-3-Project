#include "../Location.h"
#include "../Edge.h"
#include "../BTreeNode.h"
#include "../BTree.h"
#include "../Graph.h"
#include "../Navigation.h"
#include "../DatabaseManager.h"

#include <iostream>
#include <string>
#include <limits>
#include <iomanip>

using namespace std;

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void displayMenu() {
    cout << "\n========================================" << endl;
    cout << "    MINI GOOGLE MAPS NAVIGATION SYSTEM" << endl;
    cout << "========================================" << endl;
    cout << "1. Add new location" << endl;
    cout << "2. Add new road/connection" << endl;
    cout << "3. Find shortest path" << endl;
    cout << "4. View all locations" << endl;
    cout << "5. View all roads" << endl;
    cout << "6. View graph structure" << endl;
    cout << "7. Initialize sample data" << endl;
    cout << "8. Save and exit" << endl;
    cout << "========================================" << endl;
    cout << "Enter your choice (1-8): ";
}

void handleAddLocation(DatabaseManager& db) {
    cout << "\n--- Add New Location ---" << endl;
    
    string name, type;
    double lat, lon;
    
    cout << "Enter location name: ";
    clearInput();
    getline(cin, name);
    
    cout << "Enter latitude (-90 to 90): ";
    cin >> lat;
    
    cout << "Enter longitude (-180 to 180): ";
    cin >> lon;
    
    cout << "Enter location type (e.g., station, park, mall, market): ";
    clearInput();
    getline(cin, type);
    
    int id = db.addLocation(name, lat, lon, type);
    
    if (id > 0) {
        cout << "\nSuccess! Location added with ID: " << id << endl;
    } else {
        cout << "\nError: Failed to add location. Check your input values." << endl;
    }
}

void handleAddRoad(DatabaseManager& db) {
    cout << "\n--- Add New Road ---" << endl;
    
    auto locations = db.getAllLocations();
    if (locations.empty()) {
        cout << "No locations exist. Add locations first!" << endl;
        return;
    }
    
    cout << "\nExisting locations:" << endl;
    for (const auto& loc : locations) {
        cout << "  ID " << loc.id << ": " << loc.name << endl;
    }
    
    int sourceId, destId;
    double distance;
    string roadName;
    char bidirChoice;
    
    cout << "\nEnter source location ID: ";
    cin >> sourceId;
    
    cout << "Enter destination location ID: ";
    cin >> destId;
    
    cout << "Enter distance (km): ";
    cin >> distance;
    
    cout << "Enter road name: ";
    clearInput();
    getline(cin, roadName);
    
    cout << "Is this road bidirectional? (y/n): ";
    cin >> bidirChoice;
    
    bool bidirectional = (bidirChoice == 'y' || bidirChoice == 'Y');
    
    int id = db.addEdge(sourceId, destId, distance, roadName, bidirectional);
    
    if (id > 0) {
        cout << "\nSuccess! Road added with ID: " << id << endl;
    } else {
        cout << "\nError: Failed to add road. Check that both locations exist." << endl;
    }
}

void handleFindPath(DatabaseManager& db) {
    cout << "\n--- Find Shortest Path ---" << endl;
    
    auto locations = db.getAllLocations();
    if (locations.empty()) {
        cout << "No locations exist. Add locations first!" << endl;
        return;
    }
    
    cout << "\nExisting locations:" << endl;
    for (const auto& loc : locations) {
        cout << "  ID " << loc.id << ": " << loc.name << endl;
    }
    
    int sourceId, destId;
    
    cout << "\nEnter starting location ID: ";
    cin >> sourceId;
    
    cout << "Enter destination location ID: ";
    cin >> destId;
    
    Navigation nav(db.getGraph());
    PathResult result = nav.dijkstra(sourceId, destId);
    
    if (result.found) {
        cout << "\n*** PATH FOUND! ***" << endl;
        
        auto directions = nav.getDirections(result);
        for (const auto& dir : directions) {
            cout << dir << endl;
        }
        
        cout << "\nPath: ";
        for (size_t i = 0; i < result.path.size(); i++) {
            if (i > 0) cout << " -> ";
            Location loc = db.getLocation(result.path[i]);
            cout << loc.name;
        }
        cout << endl;
        
        cout << fixed << setprecision(2);
        cout << "Total Distance: " << result.totalDistance << " km" << endl;
    } else {
        cout << "\n*** NO PATH FOUND ***" << endl;
        cout << "Reason: " << result.errorMessage << endl;
    }
}

void handleViewLocations(DatabaseManager& db) {
    cout << "\n--- All Locations ---" << endl;
    
    auto locations = db.getAllLocations();
    
    if (locations.empty()) {
        cout << "No locations in database." << endl;
        return;
    }
    
    cout << fixed << setprecision(4);
    cout << "\nTotal locations: " << locations.size() << endl;
    cout << string(60, '-') << endl;
    
    for (const auto& loc : locations) {
        cout << loc.toString() << endl;
    }
    
    cout << string(60, '-') << endl;
}

void handleViewRoads(DatabaseManager& db) {
    cout << "\n--- All Roads ---" << endl;
    
    auto edges = db.getAllEdges();
    
    if (edges.empty()) {
        cout << "No roads in database." << endl;
        return;
    }
    
    cout << "\nTotal roads: " << edges.size() << endl;
    cout << string(70, '-') << endl;
    
    for (const auto& edge : edges) {
        Location source = db.getLocation(edge.sourceId);
        Location dest = db.getLocation(edge.destinationId);
        
        cout << "Road #" << edge.edgeId << ": " << edge.roadName << endl;
        cout << "  " << source.name << " (" << edge.sourceId << ") ";
        cout << (edge.isBidirectional ? "<-->" : "-->");
        cout << " " << dest.name << " (" << edge.destinationId << ")" << endl;
        cout << "  Distance: " << edge.distance << " km" << endl;
        cout << endl;
    }
    
    cout << string(70, '-') << endl;
}

int main() {
    cout << "\n";
    cout << "  __  __ _       _   _____                      _          __  __                  " << endl;
    cout << " |  \\/  (_)     (_) / ____|                    | |        |  \\/  |                 " << endl;
    cout << " | \\  / |_ _ __  _| |  __  ___   ___   __ _  | | ___    | \\  / | __ _ _ __  ___  " << endl;
    cout << " | |\\/| | | '_ \\| | | |_ |/ _ \\ / _ \\ / _` | | |/ _ \\   | |\\/| |/ _` | '_ \\/ __|" << endl;
    cout << " | |  | | | | | | | |__| | (_) | (_) | (_| | | |  __/   | |  | | (_| | |_) \\__ \\" << endl;
    cout << " |_|  |_|_|_| |_|_|\\_____|\\___/ \\___/ \\__, | |_|\\___|   |_|  |_|\\__,_| .__/|___/" << endl;
    cout << "                                       __/ |                         | |          " << endl;
    cout << "                                      |___/                          |_|          " << endl;
    cout << "\n";
    cout << "            Navigation System with B-Tree Database" << endl;
    cout << "\n";
    
    DatabaseManager db("data");
    
    if (!db.initialize()) {
        cerr << "Failed to initialize database. Exiting." << endl;
        return 1;
    }
    
    int choice;
    bool running = true;
    
    while (running) {
        displayMenu();
        cin >> choice;
        
        if (cin.fail()) {
            clearInput();
            cout << "Invalid input. Please enter a number 1-8." << endl;
            continue;
        }
        
        switch (choice) {
            case 1:
                handleAddLocation(db);
                break;
                
            case 2:
                handleAddRoad(db);
                break;
                
            case 3:
                handleFindPath(db);
                break;
                
            case 4:
                handleViewLocations(db);
                break;
                
            case 5:
                handleViewRoads(db);
                break;
                
            case 6:
                db.getGraph()->printGraph();
                break;
                
            case 7:
                db.initializeSampleData();
                break;
                
            case 8:
                cout << "\nSaving data..." << endl;
                db.saveData();
                cout << "Thank you for using Mini Google Maps!" << endl;
                running = false;
                break;
                
            default:
                cout << "Invalid choice. Please enter a number 1-8." << endl;
        }
    }
    
    return 0;
}
