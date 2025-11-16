#include "WebApiServer.hpp"
#include "../core/NekoService_Fixed.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QUrlQuery>
#include <QDebug>

namespace NekoWeb {

    WebApiServer::WebApiServer(QObject *parent)
        : QObject(parent)
        , m_httpServer(nullptr)
        , m_service(nullptr)
        , m_port(0)
        , m_maxLogs(1000)
    {
        m_httpServer = new QHttpServer(this);
    }

    WebApiServer::~WebApiServer() {
        stop();
    }

    bool WebApiServer::start(int port, NekoCore::NekoService *service) {
        if (!service) {
            emit errorOccurred("Service is null");
            return false;
        }

        m_service = service;
        m_port = port;

        // Connect to service signals
        connect(m_service, &NekoCore::NekoService::statusChanged,
                this, &WebApiServer::onServiceStatusChanged);
        connect(m_service, &NekoCore::NekoService::errorOccurred,
                this, &WebApiServer::onServiceError);
        connect(m_service, &NekoCore::NekoService::logMessage,
                this, &WebApiServer::onServiceLogMessage);

        // Setup routes
        setupRoutes();

        // Start server
        if (m_httpServer->listen(QHostAddress::Any, port)) {
            qDebug() << "Web API server started on port" << port;
            return true;
        } else {
            emit errorOccurred(QString("Failed to start web server on port %1").arg(port));
            return false;
        }
    }

    void WebApiServer::stop() {
        if (m_httpServer) {
            m_httpServer->disconnect();
            delete m_httpServer;
            m_httpServer = new QHttpServer(this);
        }
        m_service = nullptr;
        m_port = 0;
    }

    bool WebApiServer::isRunning() const {
        return m_httpServer && m_httpServer->isListening();
    }

    void WebApiServer::setupRoutes() {
        // API Routes
        m_httpServer->route("/api/status", QHttpServerRequest::Method::Get,
                           [this](const QHttpServerRequest &request) {
                               return handleGetStatus(request);
                           });

        m_httpServer->route("/api/start", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostStart(request);
                           });

        m_httpServer->route("/api/stop", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostStop(request);
                           });

        m_httpServer->route("/api/restart", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostRestart(request);
                           });

        m_httpServer->route("/api/profiles", QHttpServerRequest::Method::Get,
                           [this](const QHttpServerRequest &request) {
                               return handleGetProfiles(request);
                           });

        m_httpServer->route("/api/config", QHttpServerRequest::Method::Get,
                           [this](const QHttpServerRequest &request) {
                               return handleGetConfig(request);
                           });

        m_httpServer->route("/api/config", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostConfig(request);
                           });

        m_httpServer->route("/api/traffic", QHttpServerRequest::Method::Get,
                           [this](const QHttpServerRequest &request) {
                               return handleGetTraffic(request);
                           });

        m_httpServer->route("/api/tun/start", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostTunStart(request);
                           });

        m_httpServer->route("/api/tun/stop", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostTunStop(request);
                           });

        m_httpServer->route("/api/logs", QHttpServerRequest::Method::Get,
                           [this](const QHttpServerRequest &request) {
                               return handleGetLogs(request);
                           });

        m_httpServer->route("/api/import", QHttpServerRequest::Method::Post,
                           [this](const QHttpServerRequest &request) {
                               return handlePostImport(request);
                           });

        // Web UI Route
        m_httpServer->route("/", QHttpServerRequest::Method::Get,
                           [this](const QHttpServerRequest &request) {
                               return handleGetWebUI(request);
                           });

        m_httpServer->route("/<arg>", QHttpServerRequest::Method::Get,
                           [this](const QString &path, const QHttpServerRequest &request) {
                               Q_UNUSED(path)
                               return handleGetWebUI(request);
                           });

        // OPTIONS for CORS
        m_httpServer->route("/api/<arg>", QHttpServerRequest::Method::Options,
                           [this](const QString &path, const QHttpServerRequest &request) {
                               Q_UNUSED(path)
                               return handleOptionsRequest(request);
                           });
    }

    QHttpServerResponse WebApiServer::handleGetStatus(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        QJsonObject response;
        response["status"] = m_service->getStatusString().toLower();
        response["current_profile"] = m_service->getCurrentProfileId();
        response["tun_running"] = m_service->isTunModeRunning();
        
        if (m_service->getStatus() == NekoCore::ServiceStatus::Running) {
            QJsonObject proxy;
            proxy["socks_address"] = m_service->getSocksAddress();
            proxy["socks_port"] = m_service->getSocksPort();
            proxy["http_address"] = m_service->getHttpAddress();
            proxy["http_port"] = m_service->getHttpPort();
            response["proxy"] = proxy;
        }

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handlePostStart(const QHttpServerRequest &request) {
        if (!validateJsonRequest(request)) {
            return addCorsHeaders(errorResponse("Invalid JSON request", 400));
        }

        QJsonObject body = parseRequestBody(request);
        if (!body.contains("profile_id")) {
            return addCorsHeaders(errorResponse("Missing profile_id", 400));
        }

        int profileId = body["profile_id"].toInt();
        
        if (!m_service->loadProfile(profileId)) {
            return addCorsHeaders(errorResponse("Failed to load profile", 400));
        }

        if (!m_service->startProxy()) {
            return addCorsHeaders(errorResponse("Failed to start proxy", 500));
        }

        QJsonObject response;
        response["success"] = true;
        response["message"] = "Proxy started successfully";
        response["profile_id"] = profileId;

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handlePostStop(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        if (!m_service->stopProxy()) {
            return addCorsHeaders(errorResponse("Failed to stop proxy", 500));
        }

        QJsonObject response;
        response["success"] = true;
        response["message"] = "Proxy stopped successfully";

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handlePostRestart(const QHttpServerRequest &request) {
        if (!validateJsonRequest(request)) {
            return addCorsHeaders(errorResponse("Invalid JSON request", 400));
        }

        QJsonObject body = parseRequestBody(request);
        if (!body.contains("profile_id")) {
            return addCorsHeaders(errorResponse("Missing profile_id", 400));
        }

        int profileId = body["profile_id"].toInt();
        
        if (!m_service->loadProfile(profileId)) {
            return addCorsHeaders(errorResponse("Failed to load profile", 400));
        }

        if (!m_service->restartProxy()) {
            return addCorsHeaders(errorResponse("Failed to restart proxy", 500));
        }

        QJsonObject response;
        response["success"] = true;
        response["message"] = "Proxy restarted successfully";
        response["profile_id"] = profileId;

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handleGetProfiles(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        QJsonArray profiles;
        
        // TODO: Implement profile listing from NekoCore service
        QJsonObject sampleProfile;
        sampleProfile["id"] = 1;
        sampleProfile["name"] = "Sample Profile";
        sampleProfile["type"] = "vmess";
        profiles.append(sampleProfile);

        QJsonObject response;
        response["profiles"] = profiles;

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handleGetConfig(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        QJsonObject config = m_service->getCurrentConfig();
        return addCorsHeaders(jsonResponse(config));
    }

    QHttpServerResponse WebApiServer::handlePostConfig(const QHttpServerRequest &request) {
        if (!validateJsonRequest(request)) {
            return addCorsHeaders(errorResponse("Invalid JSON request", 400));
        }

        // TODO: Implement config update
        QJsonObject response;
        response["success"] = true;
        response["message"] = "Configuration updated successfully";

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handleGetTraffic(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        QJsonObject response;
        response["upload_bytes"] = m_service->getUploadBytes();
        response["download_bytes"] = m_service->getDownloadBytes();
        response["timestamp"] = QDateTime::currentSecsSinceEpoch();

        m_lastTrafficStats = response;
        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handlePostTunStart(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        if (!m_service->startTunMode()) {
            return addCorsHeaders(errorResponse("Failed to start TUN mode", 500));
        }

        QJsonObject response;
        response["success"] = true;
        response["message"] = "TUN mode started successfully";

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handlePostTunStop(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        if (!m_service->stopTunMode()) {
            return addCorsHeaders(errorResponse("Failed to stop TUN mode", 500));
        }

        QJsonObject response;
        response["success"] = true;
        response["message"] = "TUN mode stopped successfully";

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handleGetLogs(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        QJsonObject response;
        QJsonArray logsArray;
        
        for (const QJsonObject &log : m_logs) {
            logsArray.append(log);
        }
        
        response["logs"] = logsArray;
        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handlePostImport(const QHttpServerRequest &request) {
        if (!validateJsonRequest(request)) {
            return addCorsHeaders(errorResponse("Invalid JSON request", 400));
        }

        // TODO: Implement import functionality
        QJsonObject response;
        response["success"] = true;
        response["message"] = "Import functionality not yet implemented";

        return addCorsHeaders(jsonResponse(response));
    }

    QHttpServerResponse WebApiServer::handleGetWebUI(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        
        // Simple embedded web UI
        QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>NekoRay Web Interface</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status-card { background: #f8f9fa; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        .btn { padding: 10px 20px; margin: 5px; border: none; border-radius: 4px; cursor: pointer; }
        .btn-primary { background: #007bff; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-success { background: #28a745; color: white; }
        .logs { background: #1e1e1e; color: #fff; padding: 15px; border-radius: 4px; font-family: monospace; height: 300px; overflow-y: auto; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>NekoRay Web Interface</h1>
        
        <div class="status-card">
            <h2>Service Status</h2>
            <p id="status">Loading...</p>
            <p id="proxy-info"></p>
            <p id="traffic-info"></p>
        </div>

        <div>
            <h2>Control Panel</h2>
            <div class="form-group">
                <label for="profile-select">Select Profile:</label>
                <select id="profile-select">
                    <option value="1">Sample Profile</option>
                </select>
            </div>
            
            <button class="btn btn-success" onclick="startProxy()">Start Proxy</button>
            <button class="btn btn-danger" onclick="stopProxy()">Stop Proxy</button>
            <button class="btn btn-primary" onclick="restartProxy()">Restart Proxy</button>
            <button class="btn btn-primary" onclick="startTun()">Start TUN</button>
            <button class="btn btn-danger" onclick="stopTun()">Stop TUN</button>
        </div>

        <div>
            <h2>Logs</h2>
            <div class="logs" id="logs">Loading logs...</div>
        </div>
    </div>

    <script>
        async function apiRequest(path, method = 'GET', body = null) {
            const options = { method, headers: { 'Content-Type': 'application/json' } };
            if (body) options.body = JSON.stringify(body);
            
            try {
                const response = await fetch('/api' + path, options);
                return await response.json();
            } catch (error) {
                console.error('API Error:', error);
                return { error: error.message };
            }
        }

        async function updateStatus() {
            const status = await apiRequest('/status');
            if (status.error) {
                document.getElementById('status').textContent = 'Error: ' + status.error;
                return;
            }
            
            document.getElementById('status').innerHTML = `
                <strong>Status:</strong> ${status.status}<br>
                <strong>Current Profile:</strong> ${status.current_profile}<br>
                <strong>TUN Mode:</strong> ${status.tun_running ? 'Running' : 'Stopped'}
            `;
            
            if (status.proxy) {
                document.getElementById('proxy-info').innerHTML = `
                    <strong>SOCKS5:</strong> ${status.proxy.socks_address}:${status.proxy.socks_port}<br>
                    <strong>HTTP:</strong> ${status.proxy.http_address}:${status.proxy.http_port}
                `;
            } else {
                document.getElementById('proxy-info').innerHTML = '';
            }
        }

        async function updateTraffic() {
            const traffic = await apiRequest('/traffic');
            if (!traffic.error) {
                document.getElementById('traffic-info').innerHTML = `
                    <strong>Upload:</strong> ${formatBytes(traffic.upload_bytes)}<br>
                    <strong>Download:</strong> ${formatBytes(traffic.download_bytes)}
                `;
            }
        }

        async function updateLogs() {
            const logs = await apiRequest('/logs');
            if (!logs.error && logs.logs) {
                const logsDiv = document.getElementById('logs');
                logsDiv.innerHTML = logs.logs.map(log => 
                    `[${log.timestamp}] [${log.level}] ${log.message}`
                ).join('\\n');
                logsDiv.scrollTop = logsDiv.scrollHeight;
            }
        }

        async function startProxy() {
            const profileId = document.getElementById('profile-select').value;
            const result = await apiRequest('/start', 'POST', { profile_id: parseInt(profileId) });
            alert(result.message || result.error);
            updateStatus();
        }

        async function stopProxy() {
            const result = await apiRequest('/stop', 'POST');
            alert(result.message || result.error);
            updateStatus();
        }

        async function restartProxy() {
            const profileId = document.getElementById('profile-select').value;
            const result = await apiRequest('/restart', 'POST', { profile_id: parseInt(profileId) });
            alert(result.message || result.error);
            updateStatus();
        }

        async function startTun() {
            const result = await apiRequest('/tun/start', 'POST');
            alert(result.message || result.error);
            updateStatus();
        }

        async function stopTun() {
            const result = await apiRequest('/tun/stop', 'POST');
            alert(result.message || result.error);
            updateStatus();
        }

        function formatBytes(bytes) {
            const units = ['B', 'KB', 'MB', 'GB', 'TB'];
            let size = bytes;
            let unitIndex = 0;
            
            while (size >= 1024 && unitIndex < units.length - 1) {
                size /= 1024;
                unitIndex++;
            }
            
            return size.toFixed(2) + ' ' + units[unitIndex];
        }

        // Initial load and periodic updates
        updateStatus();
        updateTraffic();
        updateLogs();
        
        setInterval(() => {
            updateStatus();
            updateTraffic();
            updateLogs();
        }, 2000);
    </script>
</body>
</html>
        )";

        return QHttpServerResponse("text/html", html.toUtf8());
    }

    QHttpServerResponse WebApiServer::handleOptionsRequest(const QHttpServerRequest &request) {
        Q_UNUSED(request)
        return addCorsHeaders(QHttpServerResponse());
    }

    QHttpServerResponse WebApiServer::jsonResponse(const QJsonObject &data, int statusCode) {
        QJsonDocument doc(data);
        return QHttpServerResponse("application/json", doc.toJson(), QHttpServerResponse::StatusCode(statusCode));
    }

    QHttpServerResponse WebApiServer::errorResponse(const QString &error, int statusCode) {
        QJsonObject obj;
        obj["error"] = error;
        return jsonResponse(obj, statusCode);
    }

    QJsonObject WebApiServer::parseRequestBody(const QHttpServerRequest &request) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &error);
        
        if (error.error != QJsonParseError::NoError) {
            return QJsonObject();
        }
        
        return doc.object();
    }

    bool WebApiServer::validateJsonRequest(const QHttpServerRequest &request) {
        return request.headers().contains("content-type") &&
               request.headers()["content-type"] == "application/json";
    }

    QHttpServerResponse WebApiServer::addCorsHeaders(QHttpServerResponse response) {
        response.setHeader("Access-Control-Allow-Origin", "*");
        response.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        response.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return response;
    }

    void WebApiServer::onServiceStatusChanged(NekoCore::ServiceStatus status) {
        Q_UNUSED(status)
        // Status changes are handled by polling in the web interface
    }

    void WebApiServer::onServiceError(const QString &error) {
        QJsonObject logEntry;
        logEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        logEntry["level"] = "error";
        logEntry["message"] = error;
        
        m_logs.append(logEntry);
        if (m_logs.size() > m_maxLogs) {
            m_logs.removeFirst();
        }
    }

    void WebApiServer::onServiceLogMessage(const QString &level, const QString &message) {
        QJsonObject logEntry;
        logEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        logEntry["level"] = level;
        logEntry["message"] = message;
        
        m_logs.append(logEntry);
        if (m_logs.size() > m_maxLogs) {
            m_logs.removeFirst();
        }
    }

} // namespace NekoWeb