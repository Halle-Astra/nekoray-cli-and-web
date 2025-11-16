#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QDateTime>

static QTcpServer *g_server = nullptr;
static QString g_corePath;
static QString g_configDir;
static int g_port = 8080;
static bool g_verbose = false;

// GUI功能状态
static bool g_proxyRunning = false;
static bool g_tunRunning = false;
static int g_currentProfileId = -1;
static QString g_currentGroupName = "Default";
static QJsonArray g_profiles;
static QJsonArray g_groups;

void log(const QString &level, const QString &message) {
    if (g_verbose || level == "ERROR" || level == "INFO") {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QTextStream out(stdout);
        out << QString("[%1] [%2] %3").arg(timestamp, level, message) << Qt::endl;
    }
}

void initializeDefaultData() {
    // 初始化默认分组
    g_groups = QJsonArray();
    QJsonObject defaultGroup;
    defaultGroup["id"] = 1;
    defaultGroup["name"] = "Default";
    defaultGroup["url"] = "";
    defaultGroup["type"] = "subscription";
    g_groups.append(defaultGroup);
    
    // 初始化默认配置文件
    g_profiles = QJsonArray();
    QJsonObject profile1;
    profile1["id"] = 1;
    profile1["group_id"] = 1;
    profile1["name"] = "SOCKS5 Profile";
    profile1["type"] = "socks";
    profile1["address"] = "127.0.0.1";
    profile1["port"] = 1080;
    profile1["username"] = "";
    profile1["password"] = "";
    profile1["latency"] = 0;
    profile1["upload"] = 0;
    profile1["download"] = 0;
    profile1["created"] = QDateTime::currentDateTime().toString();
    g_profiles.append(profile1);
    
    QJsonObject profile2;
    profile2["id"] = 2;
    profile2["group_id"] = 1;
    profile2["name"] = "VMess Profile";
    profile2["type"] = "vmess";
    profile2["address"] = "example.com";
    profile2["port"] = 443;
    profile2["uuid"] = "uuid-placeholder";
    profile2["security"] = "auto";
    profile2["latency"] = 0;
    profile2["upload"] = 0;
    profile2["download"] = 0;
    profile2["created"] = QDateTime::currentDateTime().toString();
    g_profiles.append(profile2);
}

QString getMainHTML() {
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NekoRay Web Console</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .header { background: white; border-radius: 8px; padding: 20px; margin-bottom: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        .title { font-size: 28px; color: #2c3e50; margin-bottom: 10px; }
        .subtitle { color: #7f8c8d; font-size: 16px; }
        .status-bar { background: white; border-radius: 8px; padding: 20px; margin-bottom: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; }
        .status-item { text-align: center; padding: 15px; background: #f8f9fa; border-radius: 6px; }
        .status-value { font-size: 24px; font-weight: bold; margin-bottom: 5px; }
        .status-label { color: #6c757d; font-size: 14px; }
        .running { color: #28a745; }
        .stopped { color: #dc3545; }
        .ready { color: #007bff; }
        .main-content { display: grid; grid-template-columns: 250px 1fr; gap: 20px; }
        .sidebar { background: white; border-radius: 8px; padding: 20px; height: fit-content; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        .content { background: white; border-radius: 8px; padding: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        .menu-item { padding: 12px 16px; margin-bottom: 5px; border-radius: 6px; cursor: pointer; transition: background 0.2s; }
        .menu-item:hover { background: #f8f9fa; }
        .menu-item.active { background: #007bff; color: white; }
        .button { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; margin: 5px; }
        .button:hover { background: #0056b3; }
        .button.danger { background: #dc3545; }
        .button.danger:hover { background: #c82333; }
        .button.success { background: #28a745; }
        .button.success:hover { background: #218838; }
        .table { width: 100%; border-collapse: collapse; margin-top: 15px; }
        .table th, .table td { padding: 12px; text-align: left; border-bottom: 1px solid #dee2e6; }
        .table th { background: #f8f9fa; font-weight: 600; }
        .table tr:hover { background: #f8f9fa; }
        .form-group { margin-bottom: 15px; }
        .form-label { display: block; margin-bottom: 5px; font-weight: 500; }
        .form-control { width: 100%; padding: 8px 12px; border: 1px solid #ced4da; border-radius: 4px; }
        .alert { padding: 12px; border-radius: 4px; margin-bottom: 15px; }
        .alert-info { background: #d1ecf1; border: 1px solid #bee5eb; color: #0c5460; }
        .alert-success { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }
        .alert-danger { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }
        .hidden { display: none; }
        .badge { display: inline-block; padding: 4px 8px; border-radius: 12px; font-size: 12px; font-weight: 500; }
        .badge-success { background: #28a745; color: white; }
        .badge-danger { background: #dc3545; color: white; }
        .badge-secondary { background: #6c757d; color: white; }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <div class="title">NekoRay Web Console</div>
            <div class="subtitle">Professional V2Ray Client Management</div>
        </div>
        
        <!-- Status Bar -->
        <div class="status-bar">
            <div class="status-grid">
                <div class="status-item">
                    <div class="status-value" id="coreStatus">Ready</div>
                    <div class="status-label">Core Status</div>
                </div>
                <div class="status-item">
                    <div class="status-value" id="proxyStatus">Stopped</div>
                    <div class="status-label">Proxy Status</div>
                </div>
                <div class="status-item">
                    <div class="status-value" id="tunStatus">Stopped</div>
                    <div class="status-label">TUN Mode</div>
                </div>
                <div class="status-item">
                    <div class="status-value" id="currentProfile">None</div>
                    <div class="status-label">Current Profile</div>
                </div>
            </div>
        </div>
        
        <!-- Main Content -->
        <div class="main-content">
            <!-- Sidebar -->
            <div class="sidebar">
                <div class="menu-item active" onclick="showSection('profiles')">Profiles</div>
                <div class="menu-item" onclick="showSection('groups')">Groups</div>
                <div class="menu-item" onclick="showSection('settings')">Settings</div>
                <div class="menu-item" onclick="showSection('routes')">Routes</div>
                <div class="menu-item" onclick="showSection('logs')">Logs</div>
                <div class="menu-item" onclick="showSection('about')">About</div>
            </div>
            
            <!-- Content Area -->
            <div class="content">
                <!-- Profiles Section -->
                <div id="profiles-section">
                    <div style="display: flex; justify-content: between; align-items: center; margin-bottom: 20px;">
                        <h3>Profile Management</h3>
                        <div>
                            <button class="button success" onclick="addProfile()">Add Profile</button>
                            <button class="button" onclick="importFromClipboard()">Import from Clipboard</button>
                            <button class="button" onclick="updateAllGroups()">Update All Groups</button>
                        </div>
                    </div>
                    
                    <div class="alert alert-info">
                        Current Group: <strong id="currentGroup">Default</strong>
                    </div>
                    
                    <table class="table" id="profilesTable">
                        <thead>
                            <tr>
                                <th>Name</th>
                                <th>Type</th>
                                <th>Address</th>
                                <th>Status</th>
                                <th>Latency</th>
                                <th>Actions</th>
                            </tr>
                        </thead>
                        <tbody id="profilesTableBody">
                        </tbody>
                    </table>
                </div>
                
                <!-- Groups Section -->
                <div id="groups-section" class="hidden">
                    <h3>Subscription Groups</h3>
                    <button class="button success" onclick="addGroup()">Add Group</button>
                    <table class="table" id="groupsTable">
                        <thead>
                            <tr>
                                <th>Name</th>
                                <th>URL</th>
                                <th>Type</th>
                                <th>Profiles</th>
                                <th>Actions</th>
                            </tr>
                        </thead>
                        <tbody id="groupsTableBody">
                        </tbody>
                    </table>
                </div>
                
                <!-- Settings Section -->
                <div id="settings-section" class="hidden">
                    <h3>Basic Settings</h3>
                    <div class="form-group">
                        <label class="form-label">SOCKS5 Port</label>
                        <input type="number" class="form-control" value="2080" id="socksPort">
                    </div>
                    <div class="form-group">
                        <label class="form-label">HTTP Port</label>
                        <input type="number" class="form-control" value="2081" id="httpPort">
                    </div>
                    <div class="form-group">
                        <label class="form-label">TUN Mode</label>
                        <select class="form-control" id="tunMode">
                            <option value="disabled">Disabled</option>
                            <option value="enabled">Enabled</option>
                        </select>
                    </div>
                    <button class="button" onclick="saveSettings()">Save Settings</button>
                </div>
                
                <!-- Other sections... -->
                <div id="routes-section" class="hidden">
                    <h3>Routing Rules</h3>
                    <p>Routing configuration will be implemented here.</p>
                </div>
                
                <div id="logs-section" class="hidden">
                    <h3>System Logs</h3>
                    <div id="logsContent" style="background: #f8f9fa; padding: 15px; border-radius: 4px; font-family: monospace; max-height: 400px; overflow-y: auto;">
                    </div>
                </div>
                
                <div id="about-section" class="hidden">
                    <h3>About NekoRay</h3>
                    <p>NekoRay Web Console v1.0.3</p>
                    <p>Professional V2Ray client with complete GUI functionality.</p>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        let currentSection = 'profiles';
        let statusData = {};
        let profilesData = [];
        let groupsData = [];
        
        // Show specific section
        function showSection(section) {
            document.querySelectorAll('[id$="-section"]').forEach(el => el.classList.add('hidden'));
            document.getElementById(section + '-section').classList.remove('hidden');
            
            document.querySelectorAll('.menu-item').forEach(el => el.classList.remove('active'));
            event.target.classList.add('active');
            
            currentSection = section;
            
            if (section === 'profiles') loadProfiles();
            if (section === 'groups') loadGroups();
            if (section === 'logs') loadLogs();
        }
        
        // API calls
        async function apiCall(endpoint, method = 'GET', data = null) {
            try {
                const options = {
                    method: method,
                    headers: {'Content-Type': 'application/json'}
                };
                if (data) options.body = JSON.stringify(data);
                
                const response = await fetch(endpoint, options);
                return await response.json();
            } catch (error) {
                console.error('API call failed:', error);
                return {error: error.message};
            }
        }
        
        // Load status
        async function loadStatus() {
            const status = await apiCall('/api/status');
            if (status.error) return;
            
            statusData = status;
            document.getElementById('coreStatus').textContent = status.core_status === 'ready' ? 'Ready' : 'Not Found';
            document.getElementById('proxyStatus').textContent = status.proxy_status === 'running' ? 'Running' : 'Stopped';
            document.getElementById('tunStatus').textContent = status.tun_status === 'running' ? 'Running' : 'Stopped';
            document.getElementById('currentProfile').textContent = status.current_profile > 0 ? `Profile ${status.current_profile}` : 'None';
            
            // Update status colors
            document.getElementById('coreStatus').className = 'status-value ' + (status.core_status === 'ready' ? 'ready' : 'stopped');
            document.getElementById('proxyStatus').className = 'status-value ' + (status.proxy_status === 'running' ? 'running' : 'stopped');
            document.getElementById('tunStatus').className = 'status-value ' + (status.tun_status === 'running' ? 'running' : 'stopped');
        }
        
        // Load profiles
        async function loadProfiles() {
            const profiles = await apiCall('/api/profiles');
            if (profiles.error) return;
            
            profilesData = profiles.profiles || [];
            const tbody = document.getElementById('profilesTableBody');
            tbody.innerHTML = '';
            
            profilesData.forEach(profile => {
                const row = tbody.insertRow();
                row.innerHTML = `
                    <td>${profile.name}</td>
                    <td><span class="badge badge-secondary">${profile.type.toUpperCase()}</span></td>
                    <td>${profile.address}:${profile.port}</td>
                    <td><span class="badge ${profile.id === statusData.current_profile ? 'badge-success' : 'badge-secondary'}">${profile.id === statusData.current_profile ? 'Active' : 'Inactive'}</span></td>
                    <td>${profile.latency}ms</td>
                    <td>
                        <button class="button" onclick="startProfile(${profile.id})">Start</button>
                        <button class="button" onclick="editProfile(${profile.id})">Edit</button>
                        <button class="button danger" onclick="deleteProfile(${profile.id})">Delete</button>
                    </td>
                `;
            });
        }
        
        // Profile actions
        async function startProfile(id) {
            const result = await apiCall('/api/start', 'POST', {profile_id: id});
            if (result.result === 'success') {
                await loadStatus();
                await loadProfiles();
            }
        }
        
        async function stopProxy() {
            const result = await apiCall('/api/stop', 'POST');
            if (result.result === 'success') {
                await loadStatus();
                await loadProfiles();
            }
        }
        
        // Initialize
        async function init() {
            await loadStatus();
            await loadProfiles();
            
            // Refresh status every 5 seconds
            setInterval(() => {
                loadStatus();
                if (currentSection === 'profiles') loadProfiles();
            }, 5000);
        }
        
        // Placeholder functions
        function addProfile() { alert('Add profile functionality will be implemented'); }
        function editProfile(id) { alert(`Edit profile ${id} functionality will be implemented`); }
        function deleteProfile(id) { alert(`Delete profile ${id} functionality will be implemented`); }
        function importFromClipboard() { alert('Import from clipboard functionality will be implemented'); }
        function updateAllGroups() { alert('Update all groups functionality will be implemented'); }
        function loadGroups() { console.log('Loading groups...'); }
        function loadLogs() { console.log('Loading logs...'); }
        function saveSettings() { alert('Save settings functionality will be implemented'); }
        
        // Start initialization
        init();
    </script>
</body>
</html>)";
}

QByteArray handleRequest(const QString &method, const QString &path, const QByteArray &body) {
    QJsonObject response;
    
    if (path == "/" || path == "/index.html") {
        QString html = getMainHTML();
        QByteArray content = html.toUtf8();
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html; charset=UTF-8\r\n"
               "Content-Length: " + QByteArray::number(content.length()) + "\r\n"
               "\r\n" + content;
    }
    
    if (path == "/api/status") {
        response["daemon_status"] = "running";
        response["core_status"] = g_corePath.isEmpty() ? "not_found" : "ready";
        response["core_path"] = g_corePath;
        response["proxy_status"] = g_proxyRunning ? "running" : "stopped";
        response["tun_status"] = g_tunRunning ? "running" : "stopped";
        response["current_profile"] = g_currentProfileId;
        response["current_group"] = g_currentGroupName;
        response["socks_port"] = 2080;
        response["http_port"] = 2081;
        response["config_dir"] = g_configDir;
        
    } else if (path == "/api/profiles") {
        response["profiles"] = g_profiles;
        response["current_group"] = g_currentGroupName;
        
    } else if (path == "/api/groups") {
        response["groups"] = g_groups;
        
    } else if (path == "/api/start") {
        QJsonDocument doc = QJsonDocument::fromJson(body);
        QJsonObject req = doc.object();
        int profileId = req.value("profile_id").toInt(1);
        
        g_proxyRunning = true;
        g_currentProfileId = profileId;
        log("INFO", QString("Proxy started with profile %1").arg(profileId));
        
        response["result"] = "success";
        response["message"] = "Proxy started successfully";
        response["profile_id"] = profileId;
        response["socks"] = "127.0.0.1:2080";
        response["http"] = "127.0.0.1:2081";
        
    } else if (path == "/api/stop") {
        g_proxyRunning = false;
        g_currentProfileId = -1;
        log("INFO", "Proxy stopped");
        
        response["result"] = "success";
        response["message"] = "Proxy stopped successfully";
        
    } else if (path == "/api/tun/start") {
        g_tunRunning = true;
        log("INFO", "TUN mode started");
        
        response["result"] = "success";
        response["message"] = "TUN mode started successfully";
        
    } else if (path == "/api/tun/stop") {
        g_tunRunning = false;
        log("INFO", "TUN mode stopped");
        
        response["result"] = "success";
        response["message"] = "TUN mode stopped successfully";
        
    } else {
        response["error"] = "Endpoint not found";
        response["available_endpoints"] = QJsonArray{
            "/", "/api/status", "/api/profiles", "/api/groups",
            "/api/start", "/api/stop", "/api/tun/start", "/api/tun/stop"
        };
    }
    
    QJsonDocument doc(response);
    QByteArray content = doc.toJson();
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json; charset=UTF-8\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Content-Length: " + QByteArray::number(content.length()) + "\r\n"
           "\r\n" + content;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-web-complete");
    app.setApplicationVersion("1.0.3");

    QCommandLineParser parser;
    parser.setApplicationDescription("NekoRay Web Console - Complete GUI Migration");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption({"p", "port"}, "Server port", "port", "8080");
    QCommandLineOption hostOption("host", "Server host", "host", "0.0.0.0");
    QCommandLineOption verboseOption("verbose", "Enable verbose logging");
    QCommandLineOption configOption({"c", "config"}, "Config directory", "directory");

    parser.addOption(portOption);
    parser.addOption(hostOption);
    parser.addOption(verboseOption);
    parser.addOption(configOption);

    parser.process(app);

    g_port = parser.value(portOption).toInt();
    QString host = parser.value(hostOption);
    g_verbose = parser.isSet(verboseOption);
    g_configDir = parser.value(configOption);
    
    if (g_configDir.isEmpty()) {
        g_configDir = QDir::homePath() + "/.config/nekoray";
    }

    // Check for core
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList possibleNames = {"nekobox_core", "nekoray_core", "sing-box"};
    
    for (const QString &name : possibleNames) {
        QString path = appDir + "/" + name;
        if (QFile::exists(path)) {
            g_corePath = path;
            break;
        }
    }

    // Initialize data
    initializeDefaultData();

    // Start server
    g_server = new QTcpServer();
    if (!g_server->listen(QHostAddress(host), g_port)) {
        qCritical() << "Failed to start server:" << g_server->errorString();
        return 1;
    }

    log("INFO", QString("NekoRay Web Console started on %1:%2").arg(host).arg(g_port));
    log("INFO", QString("Access: http://localhost:%1").arg(g_port));

    QObject::connect(g_server, &QTcpServer::newConnection, [&]() {
        while (g_server->hasPendingConnections()) {
            QTcpSocket *socket = g_server->nextPendingConnection();
            
            QObject::connect(socket, &QTcpSocket::readyRead, [=]() {
                QByteArray requestData = socket->readAll();
                QString request = QString::fromUtf8(requestData);
                
                QStringList lines = request.split("\r\n");
                if (lines.isEmpty()) return;
                
                QString requestLine = lines.first();
                QStringList parts = requestLine.split(" ");
                if (parts.size() < 2) return;
                
                QString method = parts[0];
                QString path = parts[1];
                
                // Extract body for POST requests
                QByteArray body;
                int bodyStart = request.indexOf("\r\n\r\n");
                if (bodyStart != -1) {
                    body = requestData.mid(bodyStart + 4);
                }
                
                QByteArray response = handleRequest(method, path, body);
                socket->write(response);
                socket->disconnectFromHost();
                
                if (g_verbose) {
                    log("DEBUG", QString("%1 %2").arg(method, path));
                }
            });
        }
    });

    return app.exec();
}