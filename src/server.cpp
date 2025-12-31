#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include "../Location.h"
#include "../Edge.h"
#include "../BTreeNode.h"
#include "../BTree.h"
#include "../Graph.h"
#include "../Navigation.h"
#include "../DatabaseManager.h"
#include "../CircularQueue.h"
#include "../Request.h"

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace std;

const int SERVER_PORT = 8080;
const int MAX_CLIENTS = 10;
const int BUFFER_SIZE = 4096;
const int NUM_WORKER_THREADS = 4;
const size_t QUEUE_CAPACITY = 100;

DatabaseManager* g_database = nullptr;
CircularQueue<pair<Request, SOCKET>, QUEUE_CAPACITY> g_requestQueue;
atomic<bool> g_serverRunning(true);
mutex g_dbMutex;
mutex g_logMutex;

map<int, SOCKET> g_clientSockets;
mutex g_clientMutex;
atomic<int> g_nextClientId(1);

void log(const string& message) {
    lock_guard<mutex> lock(g_logMutex);
    
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    
    cout << "[" << put_time(localtime(&time), "%H:%M:%S") << "] " 
         << message << endl;
}

void sendResponse(SOCKET clientSocket, const Response& response) {
    string data = response.serialize() + "\n";
    send(clientSocket, data.c_str(), data.length(), 0);
}

Response processRequest(const Request& req) {
    lock_guard<mutex> lock(g_dbMutex);
    
    log("Processing request: " + requestTypeToString(req.type) + 
        " from client " + to_string(req.clientId));
    
    switch (req.type) {
        case RequestType::ADD_LOCATION: {
            string name = req.getParam("name");
            double lat = req.getParamDouble("latitude");
            double lon = req.getParamDouble("longitude");
            string type = req.getParam("type");
            
            if (name.empty()) {
                return Response::error(req.clientId, req.requestId, "Missing location name");
            }
            
            int id = g_database->addLocation(name, lat, lon, type);
            if (id > 0) {
                return Response::success(req.clientId, req.requestId, 
                    "Location added successfully", "id=" + to_string(id));
            } else {
                return Response::error(req.clientId, req.requestId, "Failed to add location");
            }
        }
        
        case RequestType::ADD_ROAD: {
            int sourceId = req.getParamInt("sourceId");
            int destId = req.getParamInt("destId");
            double distance = req.getParamDouble("distance");
            string roadName = req.getParam("roadName");
            bool bidir = req.getParamBool("bidirectional", true);
            
            if (sourceId <= 0 || destId <= 0) {
                return Response::error(req.clientId, req.requestId, "Invalid source or destination ID");
            }
            
            int id = g_database->addEdge(sourceId, destId, distance, roadName, bidir);
            if (id > 0) {
                return Response::success(req.clientId, req.requestId,
                    "Road added successfully", "id=" + to_string(id));
            } else {
                return Response::error(req.clientId, req.requestId, "Failed to add road");
            }
        }
        
        case RequestType::FIND_PATH: {
            int sourceId = req.getParamInt("sourceId");
            int destId = req.getParamInt("destId");
            
            if (sourceId <= 0 || destId <= 0) {
                return Response::error(req.clientId, req.requestId, "Invalid source or destination ID");
            }
            
            Navigation nav(g_database->getGraph());
            PathResult result = nav.dijkstra(sourceId, destId);
            
            if (result.found) {
                ostringstream oss;
                oss << "path=";
                for (size_t i = 0; i < result.path.size(); i++) {
                    if (i > 0) oss << "->";
                    Location loc = g_database->getLocation(result.path[i]);
                    oss << loc.name << "(" << result.path[i] << ")";
                }
                oss << ";distance=" << fixed << setprecision(2) << result.totalDistance;
                
                return Response::success(req.clientId, req.requestId,
                    "Path found", oss.str());
            } else {
                return Response::error(req.clientId, req.requestId, result.errorMessage);
            }
        }
        
        case RequestType::GET_LOCATIONS: {
            auto locations = g_database->getAllLocations();
            ostringstream oss;
            oss << "count=" << locations.size() << ";locations=";
            for (size_t i = 0; i < locations.size(); i++) {
                if (i > 0) oss << ",";
                oss << locations[i].id << ":" << locations[i].name;
            }
            return Response::success(req.clientId, req.requestId,
                "Retrieved " + to_string(locations.size()) + " locations", oss.str());
        }
        
        case RequestType::GET_ROADS: {
            auto edges = g_database->getAllEdges();
            ostringstream oss;
            oss << "count=" << edges.size() << ";roads=";
            for (size_t i = 0; i < edges.size(); i++) {
                if (i > 0) oss << ",";
                oss << edges[i].edgeId << ":" << edges[i].sourceId 
                    << "->" << edges[i].destinationId 
                    << "(" << edges[i].distance << "km)";
            }
            return Response::success(req.clientId, req.requestId,
                "Retrieved " + to_string(edges.size()) + " roads", oss.str());
        }
        
        case RequestType::GET_LOCATION: {
            int id = req.getParamInt("id");
            Location loc = g_database->getLocation(id);
            if (loc.id > 0) {
                return Response::success(req.clientId, req.requestId,
                    "Location found", loc.toString());
            } else {
                return Response::error(req.clientId, req.requestId, "Location not found");
            }
        }
        
        case RequestType::INIT_SAMPLE: {
            g_database->initializeSampleData();
            return Response::success(req.clientId, req.requestId,
                "Sample data initialized", "locations=5;roads=7");
        }
        
        case RequestType::SAVE_DATA: {
            if (g_database->saveData()) {
                return Response::success(req.clientId, req.requestId, "Data saved successfully");
            } else {
                return Response::error(req.clientId, req.requestId, "Failed to save data");
            }
        }
        
        case RequestType::SHUTDOWN: {
            g_serverRunning = false;
            return Response::success(req.clientId, req.requestId, "Server shutting down");
        }
        
        default:
            return Response::error(req.clientId, req.requestId, "Unknown request type");
    }
}

void workerThread(int workerId) {
    log("Worker " + to_string(workerId) + " started");
    
    while (g_serverRunning) {
        pair<Request, SOCKET> item;
        
        if (g_requestQueue.dequeue(item)) {
            Request& req = item.first;
            SOCKET clientSocket = item.second;
            
            log("Worker " + to_string(workerId) + " processing request " + 
                to_string(req.requestId) + " from client " + to_string(req.clientId));
            
            Response response = processRequest(req);
            
            sendResponse(clientSocket, response);
            
            log("Worker " + to_string(workerId) + " completed request " + 
                to_string(req.requestId));
        }
    }
    
    log("Worker " + to_string(workerId) + " stopped");
}

void handleClient(SOCKET clientSocket, int clientId) {
    log("Client " + to_string(clientId) + " connected");
    
    Response welcome(clientId, 0, ResponseStatus::SUCCESS, 
        "Welcome to Mini Google Maps Server. Client ID: " + to_string(clientId));
    sendResponse(clientSocket, welcome);
    
    char buffer[BUFFER_SIZE];
    string partialData;
    int requestCounter = 1;
    
    while (g_serverRunning) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesReceived <= 0) {
            break;
        }
        
        partialData += string(buffer, bytesReceived);
        
        size_t pos;
        while ((pos = partialData.find('\n')) != string::npos) {
            string message = partialData.substr(0, pos);
            partialData = partialData.substr(pos + 1);
            
            if (message.empty()) continue;
            
            Request req = Request::deserialize(message);
            req.clientId = clientId;
            req.requestId = requestCounter++;
            
            log("Received request: " + requestTypeToString(req.type) + 
                " from client " + to_string(clientId));
            
            if (!g_requestQueue.tryEnqueue({req, clientSocket})) {
                Response busy(clientId, req.requestId, ResponseStatus::FAILURE, 
                    "Server busy, request queue full");
                sendResponse(clientSocket, busy);
            }
        }
    }
    
    log("Client " + to_string(clientId) + " disconnected");
    closesocket(clientSocket);
}

bool initializeSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cerr << "WSAStartup failed: " << result << endl;
        return false;
    }
#endif
    return true;
}

void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    cout << "\n";
    cout << "========================================" << endl;
    cout << "  MINI GOOGLE MAPS NAVIGATION SERVER" << endl;
    cout << "========================================" << endl;
    cout << "\n";
    
    if (!initializeSockets()) {
        return 1;
    }
    
    log("Initializing database...");
    g_database = new DatabaseManager("data");
    if (!g_database->initialize()) {
        cerr << "Failed to initialize database" << endl;
        delete g_database;
        cleanupSockets();
        return 1;
    }
    log("Database initialized: " + to_string(g_database->getLocationCount()) + 
        " locations, " + to_string(g_database->getEdgeCount()) + " roads");
    
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket" << endl;
        delete g_database;
        cleanupSockets();
        return 1;
    }
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Failed to bind to port " << SERVER_PORT << endl;
        closesocket(serverSocket);
        delete g_database;
        cleanupSockets();
        return 1;
    }
    
    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR) {
        cerr << "Failed to listen on socket" << endl;
        closesocket(serverSocket);
        delete g_database;
        cleanupSockets();
        return 1;
    }
    
    log("Server listening on port " + to_string(SERVER_PORT));
    log("Request queue capacity: " + to_string(QUEUE_CAPACITY));
    
    vector<thread> workers;
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        workers.emplace_back(workerThread, i + 1);
    }
    log("Started " + to_string(NUM_WORKER_THREADS) + " worker threads");
    
    cout << "\nServer ready! Waiting for connections..." << endl;
    cout << "Press Ctrl+C to shutdown\n" << endl;
    
    while (g_serverRunning) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket == INVALID_SOCKET) {
            if (g_serverRunning) {
                log("Accept failed");
            }
            continue;
        }
        
        int clientId = g_nextClientId++;
        
        thread(handleClient, clientSocket, clientId).detach();
    }
    
    log("Shutting down server...");
    
    g_requestQueue.close();
    
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    g_database->saveData();
    
    closesocket(serverSocket);
    delete g_database;
    cleanupSockets();
    
    log("Server stopped");
    return 0;
}
