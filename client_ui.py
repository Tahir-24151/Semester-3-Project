"""
Mini Google Maps Navigation Client - Streamlit UI

A beautiful web-based client interface that connects to the 
C++ navigation server via TCP sockets.

Run with: streamlit run client_ui.py
"""

import streamlit as st
import socket
import time
from dataclasses import dataclass
from enum import IntEnum
from typing import Optional, Dict, List, Tuple

# Server configuration
SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8080
BUFFER_SIZE = 4096

# Request types (must match C++ enum)
class RequestType(IntEnum):
    ADD_LOCATION = 0
    ADD_ROAD = 1
    FIND_PATH = 2
    GET_LOCATIONS = 3
    GET_ROADS = 4
    GET_LOCATION = 5
    INIT_SAMPLE = 6
    SAVE_DATA = 7
    SHUTDOWN = 8
    UNKNOWN = 9

# Response status
class ResponseStatus(IntEnum):
    SUCCESS = 0
    FAILURE = 1
    NOT_FOUND = 2
    INVALID_PARAMS = 3

@dataclass
class Response:
    client_id: int
    request_id: int
    status: ResponseStatus
    message: str
    data: str

class NavigationClient:
    """TCP Client for connecting to the C++ navigation server"""
    
    def __init__(self):
        self.socket: Optional[socket.socket] = None
        self.client_id = 0
        self.request_id = 1
        self.connected = False
    
    def connect(self) -> Tuple[bool, str]:
        """Connect to the server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5.0)
            self.socket.connect((SERVER_HOST, SERVER_PORT))
            self.connected = True
            
            # Receive welcome message
            response = self._receive_response()
            if response:
                # Extract client ID from welcome message
                if "Client ID:" in response.message:
                    try:
                        self.client_id = int(response.message.split("Client ID:")[-1].strip())
                    except:
                        self.client_id = 1
                return True, response.message
            return True, "Connected to server"
        except socket.timeout:
            return False, "Connection timed out"
        except ConnectionRefusedError:
            return False, "Connection refused. Is the server running?"
        except Exception as e:
            return False, f"Connection error: {str(e)}"
    
    def disconnect(self):
        """Disconnect from the server"""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.socket = None
        self.connected = False
    
    def _serialize_request(self, req_type: RequestType, params: Dict[str, str]) -> str:
        """Serialize a request to string format"""
        param_str = ";".join([f"{k}={v}" for k, v in params.items()])
        return f"{self.client_id}|{self.request_id}|{int(req_type)}|{param_str}\n"
    
    def _parse_response(self, data: str) -> Optional[Response]:
        """Parse response from server"""
        try:
            parts = data.strip().split("|")
            if len(parts) >= 4:
                return Response(
                    client_id=int(parts[0]),
                    request_id=int(parts[1]),
                    status=ResponseStatus(int(parts[2])),
                    message=parts[3].replace("\\p", "|"),
                    data=parts[4].replace("\\p", "|") if len(parts) > 4 else ""
                )
        except Exception as e:
            st.error(f"Parse error: {e}")
        return None
    
    def _receive_response(self) -> Optional[Response]:
        """Receive and parse response from server"""
        try:
            data = ""
            while "\n" not in data:
                chunk = self.socket.recv(BUFFER_SIZE).decode('utf-8')
                if not chunk:
                    break
                data += chunk
            return self._parse_response(data)
        except Exception as e:
            st.error(f"Receive error: {e}")
            return None
    
    def send_request(self, req_type: RequestType, params: Dict[str, str] = None) -> Optional[Response]:
        """Send request and get response"""
        if not self.connected or not self.socket:
            return None
        
        params = params or {}
        request = self._serialize_request(req_type, params)
        self.request_id += 1
        
        try:
            self.socket.send(request.encode('utf-8'))
            return self._receive_response()
        except Exception as e:
            st.error(f"Send error: {e}")
            self.connected = False
            return None
    
    def add_location(self, name: str, lat: float, lon: float, loc_type: str) -> Optional[Response]:
        """Add a new location"""
        return self.send_request(RequestType.ADD_LOCATION, {
            "name": name,
            "latitude": str(lat),
            "longitude": str(lon),
            "type": loc_type
        })
    
    def add_road(self, source_id: int, dest_id: int, distance: float, 
                 road_name: str, bidirectional: bool) -> Optional[Response]:
        """Add a new road"""
        return self.send_request(RequestType.ADD_ROAD, {
            "sourceId": str(source_id),
            "destId": str(dest_id),
            "distance": str(distance),
            "roadName": road_name,
            "bidirectional": "1" if bidirectional else "0"
        })
    
    def find_path(self, source_id: int, dest_id: int) -> Optional[Response]:
        """Find shortest path between two locations"""
        return self.send_request(RequestType.FIND_PATH, {
            "sourceId": str(source_id),
            "destId": str(dest_id)
        })
    
    def get_locations(self) -> Optional[Response]:
        """Get all locations"""
        return self.send_request(RequestType.GET_LOCATIONS)
    
    def get_roads(self) -> Optional[Response]:
        """Get all roads"""
        return self.send_request(RequestType.GET_ROADS)
    
    def init_sample_data(self) -> Optional[Response]:
        """Initialize sample data"""
        return self.send_request(RequestType.INIT_SAMPLE)
    
    def save_data(self) -> Optional[Response]:
        """Save data to disk"""
        return self.send_request(RequestType.SAVE_DATA)


def parse_locations_data(data: str) -> List[Tuple[int, str]]:
    """Parse locations from response data"""
    locations = []
    try:
        # Format: count=5;locations=1:Name1,2:Name2,...
        parts = data.split(";")
        for part in parts:
            if part.startswith("locations="):
                locs_str = part[10:]
                for loc in locs_str.split(","):
                    if ":" in loc:
                        loc_id, name = loc.split(":", 1)
                        locations.append((int(loc_id), name))
    except:
        pass
    return locations


def parse_path_data(data: str) -> Tuple[str, float]:
    """Parse path from response data"""
    path = ""
    distance = 0.0
    try:
        parts = data.split(";")
        for part in parts:
            if part.startswith("path="):
                path = part[5:]
            elif part.startswith("distance="):
                distance = float(part[9:])
    except:
        pass
    return path, distance


# ============== Streamlit UI ==============

# Page configuration
st.set_page_config(
    page_title="Mini Google Maps",
    page_icon="🗺️",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Custom CSS for styling
st.markdown("""
<style>
    .main-header {
        font-size: 2.5rem;
        font-weight: 700;
        background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        text-align: center;
        padding: 1rem 0;
    }
    .status-connected {
        color: #28a745;
        font-weight: bold;
    }
    .status-disconnected {
        color: #dc3545;
        font-weight: bold;
    }
    .path-result {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        padding: 1.5rem;
        border-radius: 10px;
        margin: 1rem 0;
    }
    .location-card {
        background: #f8f9fa;
        padding: 1rem;
        border-radius: 8px;
        border-left: 4px solid #667eea;
        margin: 0.5rem 0;
    }
    .stButton > button {
        background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
        color: white;
        border: none;
        border-radius: 5px;
        padding: 0.5rem 1rem;
    }
    .stButton > button:hover {
        background: linear-gradient(90deg, #764ba2 0%, #667eea 100%);
    }
</style>
""", unsafe_allow_html=True)

# Initialize session state
if 'client' not in st.session_state:
    st.session_state.client = NavigationClient()
if 'locations' not in st.session_state:
    st.session_state.locations = []
if 'last_path' not in st.session_state:
    st.session_state.last_path = None

client = st.session_state.client

# Header
st.markdown('<h1 class="main-header">🗺️ Mini Google Maps Navigation</h1>', unsafe_allow_html=True)

# Sidebar - Connection
with st.sidebar:
    st.header("🔌 Server Connection")
    
    if client.connected:
        st.markdown(f'<p class="status-connected">✅ Connected (Client #{client.client_id})</p>', 
                    unsafe_allow_html=True)
        if st.button("Disconnect", key="disconnect"):
            client.disconnect()
            st.rerun()
    else:
        st.markdown('<p class="status-disconnected">❌ Disconnected</p>', unsafe_allow_html=True)
        if st.button("Connect to Server", key="connect"):
            success, msg = client.connect()
            if success:
                st.success(msg)
                st.rerun()
            else:
                st.error(msg)
    
    st.divider()
    
    # Quick actions
    st.header("⚡ Quick Actions")
    
    col1, col2 = st.columns(2)
    with col1:
        if st.button("📦 Init Sample", disabled=not client.connected):
            resp = client.init_sample_data()
            if resp and resp.status == ResponseStatus.SUCCESS:
                st.success("Sample data loaded!")
            else:
                st.error("Failed to load sample data")
    
    with col2:
        if st.button("💾 Save Data", disabled=not client.connected):
            resp = client.save_data()
            if resp and resp.status == ResponseStatus.SUCCESS:
                st.success("Data saved!")
            else:
                st.error("Failed to save")
    
    if st.button("🔄 Refresh Locations", disabled=not client.connected):
        resp = client.get_locations()
        if resp and resp.status == ResponseStatus.SUCCESS:
            st.session_state.locations = parse_locations_data(resp.data)
            st.success(f"Loaded {len(st.session_state.locations)} locations")
        else:
            st.error("Failed to load locations")

# Main content
if not client.connected:
    st.info("👆 Please connect to the server using the sidebar to get started.")
    st.markdown("""
    ### How to use:
    1. Start the C++ server: `server.exe`
    2. Click **Connect to Server** in the sidebar
    3. Use the tabs below to navigate, add locations, and find paths
    """)
else:
    # Create tabs
    tab1, tab2, tab3, tab4 = st.tabs(["🧭 Find Path", "📍 Add Location", "🛣️ Add Road", "📋 View Data"])
    
    # Tab 1: Find Path
    with tab1:
        st.subheader("Find Shortest Path")
        
        # Refresh locations for dropdowns
        if len(st.session_state.locations) == 0:
            resp = client.get_locations()
            if resp and resp.status == ResponseStatus.SUCCESS:
                st.session_state.locations = parse_locations_data(resp.data)
        
        locations = st.session_state.locations
        
        if len(locations) < 2:
            st.warning("Need at least 2 locations. Add locations or load sample data.")
        else:
            col1, col2 = st.columns(2)
            
            location_options = {f"{name} (ID: {loc_id})": loc_id for loc_id, name in locations}
            
            with col1:
                source = st.selectbox("🚀 Start Location", options=list(location_options.keys()))
            with col2:
                dest = st.selectbox("🎯 Destination", options=list(location_options.keys()))
            
            if st.button("🔍 Find Shortest Path", use_container_width=True):
                source_id = location_options[source]
                dest_id = location_options[dest]
                
                with st.spinner("Finding path..."):
                    resp = client.find_path(source_id, dest_id)
                
                if resp:
                    if resp.status == ResponseStatus.SUCCESS:
                        path, distance = parse_path_data(resp.data)
                        st.session_state.last_path = (path, distance)
                        
                        st.markdown(f"""
                        <div class="path-result">
                            <h3>✅ Path Found!</h3>
                            <p><strong>Route:</strong> {path}</p>
                            <p><strong>Total Distance:</strong> {distance:.2f} km</p>
                        </div>
                        """, unsafe_allow_html=True)
                        
                        # Visual representation
                        st.subheader("📍 Route Visualization")
                        path_nodes = path.split("->")
                        cols = st.columns(len(path_nodes))
                        for i, (col, node) in enumerate(zip(cols, path_nodes)):
                            with col:
                                st.markdown(f"""
                                <div style="text-align: center; padding: 10px; 
                                     background: {'#667eea' if i == 0 else '#764ba2' if i == len(path_nodes)-1 else '#f0f0f0'}; 
                                     color: {'white' if i == 0 or i == len(path_nodes)-1 else 'black'};
                                     border-radius: 10px;">
                                    {'🚀' if i == 0 else '🎯' if i == len(path_nodes)-1 else '📍'}<br>
                                    <small>{node.strip()}</small>
                                </div>
                                """, unsafe_allow_html=True)
                                if i < len(path_nodes) - 1:
                                    st.markdown("→", unsafe_allow_html=True)
                    else:
                        st.error(f"❌ {resp.message}")
                else:
                    st.error("Failed to communicate with server")
    
    # Tab 2: Add Location
    with tab2:
        st.subheader("Add New Location")
        
        with st.form("add_location_form"):
            name = st.text_input("Location Name", placeholder="e.g., Central Park")
            
            col1, col2 = st.columns(2)
            with col1:
                lat = st.number_input("Latitude", min_value=-90.0, max_value=90.0, value=40.7829)
            with col2:
                lon = st.number_input("Longitude", min_value=-180.0, max_value=180.0, value=-73.9654)
            
            loc_type = st.selectbox("Location Type", 
                options=["station", "park", "mall", "market", "plaza", "landmark", "building"])
            
            submitted = st.form_submit_button("➕ Add Location", use_container_width=True)
            
            if submitted:
                if not name:
                    st.error("Please enter a location name")
                else:
                    resp = client.add_location(name, lat, lon, loc_type)
                    if resp and resp.status == ResponseStatus.SUCCESS:
                        st.success(f"✅ Location '{name}' added successfully! {resp.data}")
                        # Refresh locations
                        st.session_state.locations = []
                    else:
                        st.error(f"❌ Failed to add location: {resp.message if resp else 'No response'}")
    
    # Tab 3: Add Road
    with tab3:
        st.subheader("Add New Road")
        
        # Refresh locations
        if len(st.session_state.locations) == 0:
            resp = client.get_locations()
            if resp and resp.status == ResponseStatus.SUCCESS:
                st.session_state.locations = parse_locations_data(resp.data)
        
        locations = st.session_state.locations
        
        if len(locations) < 2:
            st.warning("Need at least 2 locations to add a road.")
        else:
            with st.form("add_road_form"):
                location_options = {f"{name} (ID: {loc_id})": loc_id for loc_id, name in locations}
                
                col1, col2 = st.columns(2)
                with col1:
                    source = st.selectbox("From Location", options=list(location_options.keys()))
                with col2:
                    dest = st.selectbox("To Location", options=list(location_options.keys()))
                
                col1, col2 = st.columns(2)
                with col1:
                    distance = st.number_input("Distance (km)", min_value=0.1, value=1.0, step=0.1)
                with col2:
                    road_name = st.text_input("Road Name", placeholder="e.g., Main Street")
                
                bidirectional = st.checkbox("Bidirectional (two-way road)", value=True)
                
                submitted = st.form_submit_button("🛣️ Add Road", use_container_width=True)
                
                if submitted:
                    source_id = location_options[source]
                    dest_id = location_options[dest]
                    
                    if source_id == dest_id:
                        st.error("Source and destination must be different")
                    else:
                        resp = client.add_road(source_id, dest_id, distance, road_name, bidirectional)
                        if resp and resp.status == ResponseStatus.SUCCESS:
                            st.success(f"✅ Road '{road_name}' added successfully!")
                        else:
                            st.error(f"❌ Failed: {resp.message if resp else 'No response'}")
    
    # Tab 4: View Data
    with tab4:
        st.subheader("View All Data")
        
        col1, col2 = st.columns(2)
        
        with col1:
            st.markdown("### 📍 Locations")
            if st.button("Refresh Locations"):
                resp = client.get_locations()
                if resp and resp.status == ResponseStatus.SUCCESS:
                    st.session_state.locations = parse_locations_data(resp.data)
            
            for loc_id, name in st.session_state.locations:
                st.markdown(f"""
                <div class="location-card">
                    <strong>ID {loc_id}:</strong> {name}
                </div>
                """, unsafe_allow_html=True)
        
        with col2:
            st.markdown("### 🛣️ Roads")
            if st.button("Refresh Roads"):
                resp = client.get_roads()
                if resp and resp.status == ResponseStatus.SUCCESS:
                    st.info(resp.data)

# Footer
st.divider()
st.markdown("""
<div style="text-align: center; color: #888; padding: 1rem;">
    Mini Google Maps Navigation System | Built with Streamlit & C++ Server
</div>
""", unsafe_allow_html=True)
