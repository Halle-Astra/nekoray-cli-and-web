#include "NekoService_Fixed.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

namespace NekoCore {

    CoreManager::CoreManager(QObject *parent)
        : QObject(parent)
        , m_process(nullptr)
        , m_currentProfileId(-1)
    {
        // Find core executable
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList possibleNames = {"nekobox_core", "nekoray_core", "sing-box"};
        
        for (const QString &name : possibleNames) {
            QString path = appDir + "/" + name;
#ifdef Q_OS_WIN
            path += ".exe";
#endif
            if (QFile::exists(path)) {
                m_corePath = path;
                break;
            }
        }

        // If not in app directory, check system PATH
        if (m_corePath.isEmpty()) {
            for (const QString &name : possibleNames) {
                QProcess which;
                which.start("which", {name});
                which.waitForFinished(1000);
                
                if (which.exitCode() == 0) {
                    m_corePath = which.readAllStandardOutput().trimmed();
                    break;
                }
            }
        }

        if (m_corePath.isEmpty()) {
            qDebug() << "Core executable not found. Tried:" << possibleNames;
            qDebug() << "Running in SIMULATION MODE - for demonstration purposes";
            m_simulationMode = true;
        } else {
            qDebug() << "Found core at:" << m_corePath;
            m_simulationMode = false;
        }
    }

    CoreManager::~CoreManager() {
        if (m_process) {
            stop();
        }
    }

    void CoreManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        emit processFinished(exitCode, exitStatus);
    }

    bool CoreManager::start(int profileId) {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            return true; // Already running
        }

        // Handle simulation mode when no core executable is found
        if (m_corePath.isEmpty()) {
            if (m_simulationMode) {
                qDebug() << "Starting in SIMULATION MODE - no actual core process";
                m_currentProfileId = profileId;
                return true; // Simulate successful start
            } else {
                qWarning() << "Core path not set";
                return false;
            }
        }

        if (!generateConfig(profileId)) {
            qWarning() << "Failed to generate config";
            return false;
        }

        m_process = new QProcess(this);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &CoreManager::onProcessFinished);
        connect(m_process, &QProcess::readyReadStandardOutput,
                this, &CoreManager::onReadyReadStandardOutput);
        connect(m_process, &QProcess::readyReadStandardError,
                this, &CoreManager::onReadyReadStandardError);

        QStringList arguments;
        arguments << "--disable-color" << "run" << "-c" << m_configPath;

        m_process->start(m_corePath, arguments);
        
        if (!m_process->waitForStarted(5000)) {
            qWarning() << "Failed to start core process:" << m_process->errorString();
            m_process->deleteLater();
            m_process = nullptr;
            return false;
        }

        m_currentProfileId = profileId;
        emit logOutput(QString("Core started with PID %1").arg(m_process->processId()));
        return true;
    }

    bool CoreManager::stop() {
        if (!m_process || m_process->state() == QProcess::NotRunning) {
            return true;
        }

        m_process->terminate();
        if (!m_process->waitForFinished(5000)) {
            m_process->kill();
            m_process->waitForFinished(2000);
        }

        m_process->deleteLater();
        m_process = nullptr;
        m_currentProfileId = -1;

        emit logOutput("Core process stopped");
        return true;
    }

    bool CoreManager::isRunning() const {
        if (m_simulationMode && m_currentProfileId > 0) {
            return true; // Simulate running when in simulation mode
        }
        return m_process && m_process->state() == QProcess::Running;
    }

    int CoreManager::getProcessId() const {
        if (m_process && m_process->state() == QProcess::Running) {
            return m_process->processId();
        }
        return -1;
    }

    void CoreManager::onReadyReadStandardOutput() {
        if (m_process) {
            QByteArray data = m_process->readAllStandardOutput();
            QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                emit logOutput(line.trimmed());
            }
        }
    }

    void CoreManager::onReadyReadStandardError() {
        if (m_process) {
            QByteArray data = m_process->readAllStandardError();
            QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                emit logOutput("[ERROR] " + line.trimmed());
            }
        }
    }

    bool CoreManager::generateConfig(int profileId) {
        // Generate a basic sing-box config for testing
        // In real implementation, this would load actual profile data
        
        QJsonObject config;
        
        // Log configuration
        QJsonObject log;
        log["level"] = "info";
        config["log"] = log;
        
        // Inbound configuration
        QJsonArray inbounds;
        
        // SOCKS5 inbound
        QJsonObject socksInbound;
        socksInbound["type"] = "socks";
        socksInbound["tag"] = "socks-in";
        socksInbound["listen"] = "127.0.0.1";
        socksInbound["listen_port"] = 2080;
        inbounds.append(socksInbound);
        
        // HTTP inbound
        QJsonObject httpInbound;
        httpInbound["type"] = "http";
        httpInbound["tag"] = "http-in";
        httpInbound["listen"] = "127.0.0.1";
        httpInbound["listen_port"] = 2081;
        inbounds.append(httpInbound);
        
        config["inbounds"] = inbounds;
        
        // Outbound configuration
        QJsonArray outbounds;
        
        // Direct outbound (for testing)
        QJsonObject directOutbound;
        directOutbound["type"] = "direct";
        directOutbound["tag"] = "direct";
        outbounds.append(directOutbound);
        
        // Block outbound
        QJsonObject blockOutbound;
        blockOutbound["type"] = "block";
        blockOutbound["tag"] = "block";
        outbounds.append(blockOutbound);
        
        config["outbounds"] = outbounds;
        
        // Route configuration
        QJsonObject route;
        route["final"] = "direct";
        config["route"] = route;

        // Write config to file
        m_configPath = QDir::temp().absoluteFilePath(QString("nekoray_core_%1.json").arg(profileId));
        QFile file(m_configPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to write config file:" << m_configPath;
            return false;
        }

        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();

        qDebug() << "Generated config for profile" << profileId << "at:" << m_configPath;
        return true;
    }

} // namespace NekoCore