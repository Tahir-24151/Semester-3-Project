#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <sstream>
#include <map>
#include <vector>

using namespace std;

enum class RequestType {
    ADD_LOCATION,
    ADD_ROAD,
    FIND_PATH,
    GET_LOCATIONS,
    GET_ROADS,
    GET_LOCATION,
    INIT_SAMPLE,
    SAVE_DATA,
    SHUTDOWN,
    UNKNOWN
};

enum class ResponseStatus {
    SUCCESS,
    FAILURE,
    NOT_FOUND,
    INVALID_PARAMS
};

struct Request {
    int clientId;
    int requestId;
    RequestType type;
    map<string, string> params;
    
    Request() : clientId(0), requestId(0), type(RequestType::UNKNOWN) {}
    
    Request(int cId, int rId, RequestType t) 
        : clientId(cId), requestId(rId), type(t) {}
    
    string serialize() const {
        ostringstream oss;
        oss << clientId << "|" << requestId << "|" << static_cast<int>(type) << "|";
        
        bool first = true;
        for (const auto& [key, value] : params) {
            if (!first) oss << ";";
            oss << key << "=" << value;
            first = false;
        }
        
        return oss.str();
    }
    
    static Request deserialize(const string& data) {
        Request req;
        istringstream iss(data);
        string token;
        
        if (getline(iss, token, '|')) {
            req.clientId = stoi(token);
        }
        
        if (getline(iss, token, '|')) {
            req.requestId = stoi(token);
        }
        
        if (getline(iss, token, '|')) {
            req.type = static_cast<RequestType>(stoi(token));
        }
        
        if (getline(iss, token, '|')) {
            istringstream paramStream(token);
            string param;
            while (getline(paramStream, param, ';')) {
                size_t eqPos = param.find('=');
                if (eqPos != string::npos) {
                    string key = param.substr(0, eqPos);
                    string value = param.substr(eqPos + 1);
                    req.params[key] = value;
                }
            }
        }
        
        return req;
    }
    
    string getParam(const string& key) const {
        auto it = params.find(key);
        return (it != params.end()) ? it->second : "";
    }
    
    int getParamInt(const string& key, int defaultValue = 0) const {
        string val = getParam(key);
        if (val.empty()) return defaultValue;
        try {
            return stoi(val);
        } catch (...) {
            return defaultValue;
        }
    }
    
    double getParamDouble(const string& key, double defaultValue = 0.0) const {
        string val = getParam(key);
        if (val.empty()) return defaultValue;
        try {
            return stod(val);
        } catch (...) {
            return defaultValue;
        }
    }
    
    bool getParamBool(const string& key, bool defaultValue = false) const {
        string val = getParam(key);
        if (val.empty()) return defaultValue;
        return (val == "1" || val == "true" || val == "yes");
    }
    
    void setParam(const string& key, const string& value) {
        params[key] = value;
    }
    
    void setParam(const string& key, int value) {
        params[key] = to_string(value);
    }
    
    void setParam(const string& key, double value) {
        params[key] = to_string(value);
    }
    
    void setParam(const string& key, bool value) {
        params[key] = value ? "1" : "0";
    }
};

struct Response {
    int clientId;
    int requestId;
    ResponseStatus status;
    string message;
    string data;
    
    Response() : clientId(0), requestId(0), status(ResponseStatus::SUCCESS) {}
    
    Response(int cId, int rId, ResponseStatus s, const string& msg = "") 
        : clientId(cId), requestId(rId), status(s), message(msg) {}
    
    string serialize() const {
        ostringstream oss;
        oss << clientId << "|" << requestId << "|" << static_cast<int>(status) << "|";
        
        string escapedMsg = message;
        string escapedData = data;
        size_t pos;
        while ((pos = escapedMsg.find('|')) != string::npos) {
            escapedMsg.replace(pos, 1, "\\p");
        }
        while ((pos = escapedData.find('|')) != string::npos) {
            escapedData.replace(pos, 1, "\\p");
        }
        
        oss << escapedMsg << "|" << escapedData;
        return oss.str();
    }
    
    static Response deserialize(const string& str) {
        Response resp;
        istringstream iss(str);
        string token;
        
        if (getline(iss, token, '|')) resp.clientId = stoi(token);
        if (getline(iss, token, '|')) resp.requestId = stoi(token);
        if (getline(iss, token, '|')) resp.status = static_cast<ResponseStatus>(stoi(token));
        if (getline(iss, token, '|')) resp.message = token;
        if (getline(iss, token, '|')) resp.data = token;
        
        size_t pos;
        while ((pos = resp.message.find("\\p")) != string::npos) {
            resp.message.replace(pos, 2, "|");
        }
        while ((pos = resp.data.find("\\p")) != string::npos) {
            resp.data.replace(pos, 2, "|");
        }
        
        return resp;
    }
    
    static Response success(int cId, int rId, const string& msg, const string& data = "") {
        Response r(cId, rId, ResponseStatus::SUCCESS, msg);
        r.data = data;
        return r;
    }
    
    static Response error(int cId, int rId, const string& msg) {
        return Response(cId, rId, ResponseStatus::FAILURE, msg);
    }
};

inline string requestTypeToString(RequestType type) {
    switch (type) {
        case RequestType::ADD_LOCATION: return "ADD_LOCATION";
        case RequestType::ADD_ROAD: return "ADD_ROAD";
        case RequestType::FIND_PATH: return "FIND_PATH";
        case RequestType::GET_LOCATIONS: return "GET_LOCATIONS";
        case RequestType::GET_ROADS: return "GET_ROADS";
        case RequestType::GET_LOCATION: return "GET_LOCATION";
        case RequestType::INIT_SAMPLE: return "INIT_SAMPLE";
        case RequestType::SAVE_DATA: return "SAVE_DATA";
        case RequestType::SHUTDOWN: return "SHUTDOWN";
        default: return "UNKNOWN";
    }
}

#endif
