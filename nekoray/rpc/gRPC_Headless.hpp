#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>
#include <functional>

namespace NekoCore {
    
    // 简化的gRPC客户端，使用简单的TCP通信替代真正的gRPC
    class SimpleGRPCClient : public QObject {
        Q_OBJECT
        
    public:
        explicit SimpleGRPCClient(QObject *parent = nullptr);
        ~SimpleGRPCClient();
        
        // 连接管理
        bool connectToCore(const QString &address = "127.0.0.1", int port = 50051);
        void disconnectFromCore();
        bool isConnected() const;
        
        // 核心操作
        bool startCore(const QJsonObject &config);
        bool stopCore();
        bool restartCore(const QJsonObject &config);
        
        // 状态查询
        QJsonObject getStatus();
        QJsonObject getTrafficStats();
        bool testLatency(const QString &profileId);
        
        // 配置操作
        bool updateConfig(const QJsonObject &config);
        bool selectProfile(int profileId);
        
        // 错误处理
        QString lastError() const { return m_lastError; }
        
    signals:
        void connected();
        void disconnected();
        void statusChanged(const QJsonObject &status);
        void trafficUpdated(qint64 upload, qint64 download);
        void errorOccurred(const QString &error);
        
    private slots:
        void onSocketConnected();
        void onSocketDisconnected();
        void onSocketError();
        void onDataReceived();
        void updateTrafficStats();
        
    private:
        // 简化的通信协议
        bool sendCommand(const QString &command, const QJsonObject &data = QJsonObject());
        QJsonObject receiveResponse();
        
        QTcpSocket *m_socket;
        QTimer *m_trafficTimer;
        QString m_lastError;
        QString m_serverAddress;
        int m_serverPort;
        
        // 模拟的统计数据
        qint64 m_uploadBytes;
        qint64 m_downloadBytes;
        qint64 m_lastUpload;
        qint64 m_lastDownload;
        
        bool m_coreRunning;
        int m_currentProfileId;
    };
    
    // 全局gRPC客户端实例
    extern SimpleGRPCClient *grpcClient;
    
    // 初始化和清理函数
    bool InitializeGRPC();
    void CleanupGRPC();
    
} // namespace NekoCore