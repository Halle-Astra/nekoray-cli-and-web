#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QMutex>
#include <QSharedPointer>

namespace NekoCore {

    // Service status
    enum class ServiceStatus {
        Stopped,
        Starting,
        Running,
        Stopping,
        Error
    };

    // Proxy mode
    enum class ProxyMode {
        System,  // System proxy
        TUN,     // TUN mode
        Manual   // Manual mode
    };

    class CoreManager;
    class TunManager;
    class ConfigManager;

    // Main service class - headless core functionality
    class NekoService : public QObject {
        Q_OBJECT

    public:
        explicit NekoService(QObject *parent = nullptr);
        ~NekoService();

        // Service lifecycle
        bool initialize(const QString &configDir = "");
        void shutdown();

        // Status
        ServiceStatus getStatus() const { return m_status; }
        QString getStatusString() const;
        
        // Profile management
        bool loadProfile(int profileId);
        bool startProxy();
        bool stopProxy();
        bool restartProxy();
        
        // TUN mode
        bool startTunMode();
        bool stopTunMode();
        bool isTunModeRunning() const;
        
        // Config management
        bool loadConfig(const QString &configPath = "");
        bool saveConfig();
        QJsonObject getCurrentConfig() const;
        
        // Get current running profile ID
        int getCurrentProfileId() const { return m_currentProfileId; }
        
        // Get proxy information
        QString getSocksAddress() const;
        int getSocksPort() const;
        QString getHttpAddress() const;
        int getHttpPort() const;
        
        // Statistics
        qint64 getUploadBytes() const;
        qint64 getDownloadBytes() const;
        void resetTraffic();

    public slots:
        void onCoreProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void onTunProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    signals:
        void statusChanged(ServiceStatus status);
        void profileChanged(int profileId);
        void trafficUpdated(qint64 upload, qint64 download);
        void errorOccurred(const QString &error);
        void logMessage(const QString &level, const QString &message);

    private slots:
        void updateTraffic();

    private:
        void setStatus(ServiceStatus status);
        bool initializeDirectories();
        bool loadDataStore();

        ServiceStatus m_status;
        QString m_configDir;
        int m_currentProfileId;
        
        QSharedPointer<CoreManager> m_coreManager;
        QSharedPointer<TunManager> m_tunManager;
        QSharedPointer<ConfigManager> m_configManager;
        
        QTimer *m_trafficTimer;
        QMutex m_mutex;

        // Traffic statistics
        qint64 m_uploadBytes;
        qint64 m_downloadBytes;
        qint64 m_lastUploadBytes;
        qint64 m_lastDownloadBytes;
    };

    // Core process manager
    class CoreManager : public QObject {
        Q_OBJECT

    public:
        explicit CoreManager(QObject *parent = nullptr);
        ~CoreManager();

        bool start(int profileId);
        bool stop();
        bool isRunning() const;
        QString getConfigPath() const { return m_configPath; }
        int getProcessId() const;

    signals:
        void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void logOutput(const QString &output);

    private slots:
        void onReadyReadStandardOutput();
        void onReadyReadStandardError();

    private:
        bool generateConfig(int profileId);

        QProcess *m_process;
        QString m_configPath;
        QString m_corePath;
        int m_currentProfileId;
    };

    // TUN manager for headless TUN support
    class TunManager : public QObject {
        Q_OBJECT

    public:
        explicit TunManager(QObject *parent = nullptr);
        ~TunManager();

        bool start();
        bool stop();
        bool isRunning() const;
        QString getTunConfigPath() const { return m_tunConfigPath; }

    signals:
        void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void logOutput(const QString &output);

    private slots:
        void onReadyReadStandardOutput();
        void onReadyReadStandardError();

    private:
        bool generateTunConfig();
        bool startTunProcess();

        QProcess *m_tunProcess;
        QString m_tunConfigPath;
        QString m_tunScriptPath;
        QString m_corePath;
    };

    // Configuration manager
    class ConfigManager : public QObject {
        Q_OBJECT

    public:
        explicit ConfigManager(QObject *parent = nullptr);
        ~ConfigManager();

        bool initialize(const QString &configDir);
        bool load();
        bool save();
        
        QJsonObject getDataStore() const { return m_dataStore; }
        void setDataStore(const QJsonObject &dataStore) { m_dataStore = dataStore; }
        
        QString getConfigDir() const { return m_configDir; }

    private:
        QJsonObject m_dataStore;
        QString m_configDir;
        QString m_configFile;
    };

} // namespace NekoCore