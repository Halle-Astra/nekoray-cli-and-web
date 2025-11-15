#include "NekoService_Fixed.hpp"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QMutexLocker>
#include <QThread>

namespace NekoCore {

    NekoService::NekoService(QObject *parent)
        : QObject(parent)
        , m_status(ServiceStatus::Stopped)
        , m_currentProfileId(-1)
        , m_uploadBytes(0)
        , m_downloadBytes(0)
        , m_lastUploadBytes(0)
        , m_lastDownloadBytes(0)
    {
        m_coreManager = QSharedPointer<CoreManager>::create(this);
        m_tunManager = QSharedPointer<TunManager>::create(this);
        m_configManager = QSharedPointer<ConfigManager>::create(this);

        m_trafficTimer = new QTimer(this);
        m_trafficTimer->setInterval(1000); // Update every second
        connect(m_trafficTimer, &QTimer::timeout, this, &NekoService::updateTraffic);

        // Connect signals
        connect(m_coreManager.get(), &CoreManager::processFinished,
                this, &NekoService::onCoreProcessFinished);
        connect(m_tunManager.get(), &TunManager::processFinished,
                this, &NekoService::onTunProcessFinished);

        // Initialize default settings
        m_defaultConfig["inbound_address"] = "127.0.0.1";
        m_defaultConfig["inbound_socks_port"] = 2080;
        m_defaultConfig["inbound_http_port"] = 2081;
        m_defaultConfig["spmode_vpn"] = false;
        m_defaultConfig["vpn_internal_tun"] = true;
    }

    NekoService::~NekoService() {
        shutdown();
    }

    bool NekoService::initialize(const QString &configDir) {
        QMutexLocker locker(&m_mutex);
        
        if (configDir.isEmpty()) {
            m_configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/nekoray";
        } else {
            m_configDir = configDir;
        }

        if (!initializeDirectories()) {
            return false;
        }

        if (!m_configManager->initialize(m_configDir)) {
            return false;
        }

        if (!loadDataStore()) {
            return false;
        }

        setStatus(ServiceStatus::Stopped);
        return true;
    }

    void NekoService::shutdown() {
        QMutexLocker locker(&m_mutex);
        
        if (m_status == ServiceStatus::Stopped) {
            return;
        }

        setStatus(ServiceStatus::Stopping);
        
        if (m_trafficTimer->isActive()) {
            m_trafficTimer->stop();
        }

        if (m_tunManager->isRunning()) {
            m_tunManager->stop();
        }

        if (m_coreManager->isRunning()) {
            m_coreManager->stop();
        }

        m_configManager->save();
        setStatus(ServiceStatus::Stopped);
    }

    QString NekoService::getStatusString() const {
        switch (m_status) {
            case ServiceStatus::Stopped: return "Stopped";
            case ServiceStatus::Starting: return "Starting";
            case ServiceStatus::Running: return "Running";
            case ServiceStatus::Stopping: return "Stopping";
            case ServiceStatus::Error: return "Error";
            default: return "Unknown";
        }
    }

    bool NekoService::loadProfile(int profileId) {
        QMutexLocker locker(&m_mutex);
        
        // For now, we'll use a simplified profile validation
        // In real implementation, this would load from profile files
        if (profileId <= 0 || profileId > 100) {
            emit errorOccurred(QString("Invalid profile ID: %1").arg(profileId));
            return false;
        }

        m_currentProfileId = profileId;
        emit profileChanged(profileId);
        emit logMessage("info", QString("Profile %1 loaded").arg(profileId));
        return true;
    }

    bool NekoService::startProxy() {
        QMutexLocker locker(&m_mutex);
        
        if (m_status == ServiceStatus::Running) {
            return true;
        }

        if (m_currentProfileId < 0) {
            emit errorOccurred("No profile selected");
            return false;
        }

        setStatus(ServiceStatus::Starting);

        if (!m_coreManager->start(m_currentProfileId)) {
            setStatus(ServiceStatus::Error);
            emit errorOccurred("Failed to start core process");
            return false;
        }

        // Start TUN mode if enabled
        bool spModeVpn = m_currentConfig.value("spmode_vpn", false).toBool();
        bool vpnInternalTun = m_currentConfig.value("vpn_internal_tun", true).toBool();
        
        if (spModeVpn && vpnInternalTun) {
            if (!m_tunManager->start()) {
                emit logMessage("warn", "Failed to start TUN mode, continuing without TUN");
            }
        }

        m_trafficTimer->start();
        setStatus(ServiceStatus::Running);
        emit logMessage("info", "Proxy started successfully");
        return true;
    }

    bool NekoService::stopProxy() {
        QMutexLocker locker(&m_mutex);
        
        if (m_status == ServiceStatus::Stopped) {
            return true;
        }

        setStatus(ServiceStatus::Stopping);

        if (m_trafficTimer->isActive()) {
            m_trafficTimer->stop();
        }

        if (m_tunManager->isRunning()) {
            m_tunManager->stop();
        }

        if (m_coreManager->isRunning()) {
            m_coreManager->stop();
        }

        setStatus(ServiceStatus::Stopped);
        emit logMessage("info", "Proxy stopped successfully");
        return true;
    }

    bool NekoService::restartProxy() {
        if (!stopProxy()) {
            return false;
        }
        
        // Give some time for processes to fully stop
        QThread::msleep(500);
        
        return startProxy();
    }

    bool NekoService::startTunMode() {
        QMutexLocker locker(&m_mutex);
        
        if (m_tunManager->isRunning()) {
            return true;
        }

        return m_tunManager->start();
    }

    bool NekoService::stopTunMode() {
        QMutexLocker locker(&m_mutex);
        return m_tunManager->stop();
    }

    bool NekoService::isTunModeRunning() const {
        return m_tunManager->isRunning();
    }

    bool NekoService::loadConfig(const QString &configPath) {
        QMutexLocker locker(&m_mutex);
        
        if (!configPath.isEmpty()) {
            // Load specific config file
            QFile file(configPath);
            if (!file.open(QIODevice::ReadOnly)) {
                return false;
            }
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                return false;
            }
            
            m_currentConfig = doc.object();
            return true;
        }

        return m_configManager->load();
    }

    bool NekoService::saveConfig() {
        QMutexLocker locker(&m_mutex);
        return m_configManager->save();
    }

    QJsonObject NekoService::getCurrentConfig() const {
        QJsonObject config = m_currentConfig;
        
        // Merge with defaults
        for (auto it = m_defaultConfig.begin(); it != m_defaultConfig.end(); ++it) {
            if (!config.contains(it.key())) {
                config[it.key()] = it.value();
            }
        }
        
        return config;
    }

    QString NekoService::getSocksAddress() const {
        return m_currentConfig.value("inbound_address", 
            m_defaultConfig.value("inbound_address")).toString();
    }

    int NekoService::getSocksPort() const {
        return m_currentConfig.value("inbound_socks_port", 
            m_defaultConfig.value("inbound_socks_port")).toInt();
    }

    QString NekoService::getHttpAddress() const {
        return m_currentConfig.value("inbound_address", 
            m_defaultConfig.value("inbound_address")).toString();
    }

    int NekoService::getHttpPort() const {
        return m_currentConfig.value("inbound_http_port", 
            m_defaultConfig.value("inbound_http_port")).toInt();
    }

    qint64 NekoService::getUploadBytes() const {
        return m_uploadBytes;
    }

    qint64 NekoService::getDownloadBytes() const {
        return m_downloadBytes;
    }

    void NekoService::resetTraffic() {
        QMutexLocker locker(&m_mutex);
        m_uploadBytes = 0;
        m_downloadBytes = 0;
        m_lastUploadBytes = 0;
        m_lastDownloadBytes = 0;
        emit trafficUpdated(0, 0);
    }

    void NekoService::onCoreProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitCode)
        Q_UNUSED(exitStatus)
        
        if (m_status == ServiceStatus::Running) {
            setStatus(ServiceStatus::Error);
            emit errorOccurred("Core process crashed unexpectedly");
        }
    }

    void NekoService::onTunProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitCode)
        Q_UNUSED(exitStatus)
        
        emit logMessage("warn", "TUN process finished");
    }

    void NekoService::updateTraffic() {
        // Get traffic stats from gRPC or other method
        // For now, we'll simulate traffic update
        if (m_coreManager->isRunning()) {
            // TODO: Implement actual traffic monitoring
            // This is a placeholder - in real implementation, you'd query the core
            m_uploadBytes += qrand() % 1000;
            m_downloadBytes += qrand() % 5000;
            emit trafficUpdated(m_uploadBytes, m_downloadBytes);
        }
    }

    void NekoService::setStatus(ServiceStatus status) {
        if (m_status != status) {
            m_status = status;
            emit statusChanged(status);
            emit logMessage("debug", QString("Service status changed to: %1").arg(getStatusString()));
        }
    }

    bool NekoService::initializeDirectories() {
        QDir dir(m_configDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                emit errorOccurred(QString("Failed to create config directory: %1").arg(m_configDir));
                return false;
            }
        }

        QStringList requiredDirs = {"profiles", "groups", "routes"};
        for (const QString &subdir : requiredDirs) {
            if (!dir.exists(subdir)) {
                if (!dir.mkdir(subdir)) {
                    emit errorOccurred(QString("Failed to create directory: %1/%2").arg(m_configDir, subdir));
                    return false;
                }
            }
        }

        return true;
    }

    bool NekoService::loadDataStore() {
        // Load configuration from files instead of global variables
        QString configFile = m_configDir + "/groups/nekobox.json";
        
        QFile file(configFile);
        if (!file.exists()) {
            // Create default config
            m_currentConfig = m_defaultConfig;
            return saveConfig();
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        file.close();

        if (error.error != QJsonParseError::NoError) {
            return false;
        }

        m_currentConfig = doc.object();
        
        // Merge with defaults for missing keys
        for (auto it = m_defaultConfig.begin(); it != m_defaultConfig.end(); ++it) {
            if (!m_currentConfig.contains(it.key())) {
                m_currentConfig[it.key()] = it.value();
            }
        }

        return true;
    }

} // namespace NekoCore