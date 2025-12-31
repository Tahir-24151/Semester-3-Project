#ifndef LOCATION_H
#define LOCATION_H

#include <string>
#include <sstream>

using namespace std;

class Location {
public:
    int id;
    string name;
    double latitude;
    double longitude;
    string type;

    Location() : id(0), name(""), latitude(0.0), longitude(0.0), type("") {}

    Location(int id, const string& name, double lat, double lon, const string& type)
        : id(id), name(name), latitude(lat), longitude(lon), type(type) {}

    string serialize() const {
        ostringstream oss;
        oss << name << "|" << latitude << "|" << longitude << "|" << type;
        return oss.str();
    }

    static Location deserialize(int id, const string& data) {
        Location loc;
        loc.id = id;
        
        istringstream iss(data);
        string token;
        
        if (getline(iss, token, '|')) loc.name = token;
        if (getline(iss, token, '|')) loc.latitude = stod(token);
        if (getline(iss, token, '|')) loc.longitude = stod(token);
        if (getline(iss, token, '|')) loc.type = token;
        
        return loc;
    }

    string toString() const {
        ostringstream oss;
        oss << "ID: " << id << ", Name: " << name 
            << ", Lat: " << latitude << ", Lon: " << longitude 
            << ", Type: " << type;
        return oss.str();
    }

    bool isValid() const {
        return id > 0 && !name.empty() && 
               latitude >= -90.0 && latitude <= 90.0 &&
               longitude >= -180.0 && longitude <= 180.0;
    }
};

#endif
