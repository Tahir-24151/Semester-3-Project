#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
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

#include "../Request.h"

#include <iostream>
#include <string>
#include <sstream>
#include <limits>
#include <thread>
#include <atomic>

using namespace std;

const char* SERVER_HOST = "127.0.0.1";
const int SERVER_PORT = 8080;
const int BUFFER_SIZE = 4096;

atomic<bool> g_connected(false);
atomic<int> g_clientId(0);
int g_requestId = 1;

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void displayMenu() {
    cout << "\n========================================" << endl;
    cout << "    NAVIGATION CLIENT - Client #" << g_clientId << endl;
    cout << "========================================" << endl;
    cout << "1. Add new location" << endl;
    cout << "2. Add new road/connection" << endl;
    cout << "3. Find shortest path" << endl;
    cout << "4. View all locations" << endl;
    cout << "5. View all roads" << endl;
    cout << "6. Initialize sample data" << endl;
    cout << "7. Save data" << endl;
    cout << "8. Disconnect and exit" << endl;
    cout << "========================================" << endl;
    cout << "Enter your choice (1-8): ";
}

bool sendRequest(SOCKET sock, const Request& req) {
    string data = req.serialize() + "\n";
    int result = send(sock, data.c_str(), data.length(), 0);
    return result != SOCKET_ERROR;
}

Response receiveResponse(SOCKET sock) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    string data;
    while (true) {
        int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived <= 0) {
            break;
        }
        data += string(buffer, bytesReceived);
        if (data.find('\n') != string::npos) {
            break;
        }
    }
    
    if (!data.empty() && data.back() == '\n') {
        data.pop_back();
    }
    
    return Response::deserialize(data);
}

void displayResponse(const Response& resp) {
    cout << "\n--- Server Response ---" << endl;
    if (resp.status == ResponseStatus::SUCCESS) {
        cout << "[SUCCESS] " << resp.message << endl;
        if (!resp.data.empty()) {
            cout << "Data: " << resp.data << endl;
        }
    } else {
        cout << "[ERROR] " << resp.message << endl;
    }
    cout << "-----------------------" << endl;
}

void handleAddLocation(SOCKET sock) {
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
    
    cout << "Enter location type (e.g., station, park, mall): ";
    clearInput();
    getline(cin, type);
    
    Request req(g_clientId, g_requestId++, RequestType::ADD_LOCATION);
    req.setParam("name", name);
    req.setParam("latitude", lat);
    req.setParam("longitude", lon);
    req.setParam("type", type);
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
}

void handleAddRoad(SOCKET sock) {
    cout << "\n--- Add New Road ---" << endl;
    
    int sourceId, destId;
    double distance;
    string roadName;
    char bidirChoice;
    
    cout << "Enter source location ID: ";
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
    
    Request req(g_clientId, g_requestId++, RequestType::ADD_ROAD);
    req.setParam("sourceId", sourceId);
    req.setParam("destId", destId);
    req.setParam("distance", distance);
    req.setParam("roadName", roadName);
    req.setParam("bidirectional", (bidirChoice == 'y' || bidirChoice == 'Y'));
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
}

void handleFindPath(SOCKET sock) {
    cout << "\n--- Find Shortest Path ---" << endl;
    
    int sourceId, destId;
    
    cout << "Enter starting location ID: ";
    cin >> sourceId;
    
    cout << "Enter destination location ID: ";
    cin >> destId;
    
    Request req(g_clientId, g_requestId++, RequestType::FIND_PATH);
    req.setParam("sourceId", sourceId);
    req.setParam("destId", destId);
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
}

void handleViewLocations(SOCKET sock) {
    Request req(g_clientId, g_requestId++, RequestType::GET_LOCATIONS);
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
}

void handleViewRoads(SOCKET sock) {
    Request req(g_clientId, g_requestId++, RequestType::GET_ROADS);
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
}

void handleInitSample(SOCKET sock) {
    Request req(g_clientId, g_requestId++, RequestType::INIT_SAMPLE);
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
}

void handleSaveData(SOCKET sock) {
    Request req(g_clientId, g_requestId++, RequestType::SAVE_DATA);
    
    if (sendRequest(sock, req)) {
        displayResponse(receiveResponse(sock));
    } else {
        cout << "Failed to send request" << endl;
    }
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
    cout << "  MINI GOOGLE MAPS NAVIGATION CLIENT" << endl;
    cout << "========================================" << endl;
    cout << "\n";
    
    if (!initializeSockets()) {
        return 1;
    }
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "Failed to create socket" << endl;
        cleanupSockets();
        return 1;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_HOST, &serverAddr.sin_addr);
    
    cout << "Connecting to server at " << SERVER_HOST << ":" << SERVER_PORT << "..." << endl;
    
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Failed to connect to server. Is the server running?" << endl;
        closesocket(sock);
        cleanupSockets();
        return 1;
    }
    
    g_connected = true;
    cout << "Connected!" << endl;
    
    Response welcome = receiveResponse(sock);
    cout << welcome.message << endl;
    
    size_t idPos = welcome.message.find("Client ID: ");
    if (idPos != string::npos) {
        g_clientId = stoi(welcome.message.substr(idPos + 11));
    }
    
    int choice;
    bool running = true;
    
    while (running && g_connected) {
        displayMenu();
        cin >> choice;
        
        if (cin.fail()) {
            clearInput();
            cout << "Invalid input. Please enter a number 1-8." << endl;
            continue;
        }
        
        switch (choice) {
            case 1:
                handleAddLocation(sock);
                break;
            case 2:
                handleAddRoad(sock);
                break;
            case 3:
                handleFindPath(sock);
                break;
            case 4:
                handleViewLocations(sock);
                break;
            case 5:
                handleViewRoads(sock);
                break;
            case 6:
                handleInitSample(sock);
                break;
            case 7:
                handleSaveData(sock);
                break;
            case 8:
                cout << "\nDisconnecting..." << endl;
                running = false;
                break;
            default:
                cout << "Invalid choice. Please enter a number 1-8." << endl;
        }
    }
    
    closesocket(sock);
    cleanupSockets();
    
    cout << "Goodbye!" << endl;
    return 0;
}
