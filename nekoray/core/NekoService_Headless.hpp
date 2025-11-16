#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QMutex>
#include <QSharedPointer>

// 包含headless版本的依赖
#include "db/Database_Headless.hpp"
#include "fmt/includes_headless.h"
#include "rpc/gRPC_Headless.hpp"

namespace NekoCore {

    // Service status
    enum class ServiceStatus {
        Stopped,
        Starting,
        Running,
        Stopping,
        Error
    };

    // Forward declarations
    class CoreManager;
    class TunManager;
    class ConfigManager;

    class NekoService : public QObject {
        Q_OBJECT

    public:
        explicit NekoService(QObject *parent = nullptr);
        ~NekoService();

        // 初始化和关闭
        bool initialize(const QString &configDir = "");
        void shutdown();

        // 状态管理
        ServiceStatus getStatus() const { return m_status; }
        QString getStatusString() const;

        // Profile管理  
        bool loadProfile(int profileId);
        int getCurrentProfileId() const { return m_currentProfileId; }

        // 代理控制
        bool startProxy();
        bool stopProxy();
        bool restartProxy();

        // TUN模式控制
        bool startTunMode();
        bool stopTunMode();
        bool isTunModeRunning() const;

        // 配置管理
        bool loadConfig(const QString &configPath = "");
        bool saveConfig();
        QJsonObject getCurrentConfig() const;

        // 网络配置获取
        QString getSocksAddress() const;
        int getSocksPort() const;
        QString getHttpAddress() const;
        int getHttpPort() const;

        // 流量统计
        qint64 getUploadBytes() const;
        qint64 getDownloadBytes() const;
        void resetTraffic();

    signals:
        void statusChanged(ServiceStatus status);
        void profileChanged(int profileId);
        void trafficUpdated(qint64 upload, qint64 download);
        void errorOccurred(const QString &error);
        void logMessage(const QString &level, const QString &message);

    private slots:
        void onCoreProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void onTunProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void updateTraffic();

    private:
        void setStatus(ServiceStatus status);
        bool initializeDirectories();
        bool loadDataStore();

        // 状态
        ServiceStatus m_status;
        int m_currentProfileId;
        QString m_configDir;
        
        // 管理器
        QSharedPointer<CoreManager> m_coreManager;
        QSharedPointer<TunManager> m_tunManager;
        QSharedPointer<ConfigManager> m_configManager;
        
        // 流量统计
        QTimer *m_trafficTimer;
        qint64 m_uploadBytes;
        qint64 m_downloadBytes;
        qint64 m_lastUploadBytes;
        qint64 m_lastDownloadBytes;
        
        // 配置数据 (替代全局变量)
        QJsonObject m_currentConfig;
        QJsonObject m_defaultConfig;
        
        // 线程安全
        mutable QMutex m_mutex;
    };

} // namespace NekoCore