#include "NekoService_Fixed.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

namespace NekoCore {

    TunManager::TunManager(QObject *parent)
        : QObject(parent)
        , m_tunProcess(nullptr)
    {
        // Find core executable for TUN mode
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
    }

    TunManager::~TunManager() {
        if (m_tunProcess) {
            stop();
        }
    }

    void TunManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        emit processFinished(exitCode, exitStatus);
    }

    bool TunManager::start() {
        if (m_tunProcess && m_tunProcess->state() != QProcess::NotRunning) {
            return true; // Already running
        }

        if (m_corePath.isEmpty()) {
            qWarning() << "Core path not set for TUN mode";
            return false;
        }

        if (!generateTunConfig()) {
            qWarning() << "Failed to generate TUN config";
            return false;
        }

        return startTunProcess();
    }

    bool TunManager::stop() {
        if (!m_tunProcess || m_tunProcess->state() == QProcess::NotRunning) {
            return true;
        }

        // On Linux, we might need to stop the privileged process
#ifndef Q_OS_WIN
        // Try to stop using killall first
        QProcess killProcess;
        killProcess.start("pkexec", {"killall", "-2", "nekobox_core"});
        killProcess.waitForFinished(3000);
#endif

        m_tunProcess->terminate();
        if (!m_tunProcess->waitForFinished(5000)) {
            m_tunProcess->kill();
            m_tunProcess->waitForFinished(2000);
        }

        m_tunProcess->deleteLater();
        m_tunProcess = nullptr;

        // Clean up temp files
        if (!m_tunConfigPath.isEmpty()) {
            QFile::remove(m_tunConfigPath);
        }
        if (!m_tunScriptPath.isEmpty()) {
            QFile::remove(m_tunScriptPath);
        }

        emit logOutput("TUN process stopped");
        return true;
    }

    bool TunManager::isRunning() const {
        return m_tunProcess && m_tunProcess->state() == QProcess::Running;
    }

    void TunManager::onReadyReadStandardOutput() {
        if (m_tunProcess) {
            QByteArray data = m_tunProcess->readAllStandardOutput();
            QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                emit logOutput("[TUN] " + line.trimmed());
            }
        }
    }

    void TunManager::onReadyReadStandardError() {
        if (m_tunProcess) {
            QByteArray data = m_tunProcess->readAllStandardError();
            QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                emit logOutput("[TUN ERROR] " + line.trimmed());
            }
        }
    }

    bool TunManager::generateTunConfig() {
        // Generate a basic TUN configuration
        QJsonObject config;
        
        // Log configuration
        QJsonObject log;
        log["level"] = "info";
        config["log"] = log;
        
        // DNS configuration with fake IP
        QJsonObject dns;
        QJsonObject fakeip;
        fakeip["enabled"] = true;
        fakeip["inet4_range"] = "198.18.0.0/15";
        fakeip["inet6_range"] = "fc00::/18";
        dns["fakeip"] = fakeip;
        
        QJsonArray servers;
        QJsonObject dnsRemote;
        dnsRemote["tag"] = "dns-remote";
        dnsRemote["address"] = "8.8.8.8";
        dnsRemote["detour"] = "direct";
        servers.append(dnsRemote);
        
        QJsonObject dnsDirect;
        dnsDirect["tag"] = "dns-direct";
        dnsDirect["address"] = "223.5.5.5";
        dnsDirect["detour"] = "direct";
        servers.append(dnsDirect);
        
        QJsonObject dnsFake;
        dnsFake["address"] = "fakeip";
        dnsFake["tag"] = "dns-fake";
        servers.append(dnsFake);
        
        dns["servers"] = servers;
        config["dns"] = dns;
        
        // TUN inbound
        QJsonArray inbounds;
        QJsonObject tunInbound;
        tunInbound["type"] = "tun";
        tunInbound["tag"] = "tun-in";
        tunInbound["interface_name"] = "nekoray-tun";
        tunInbound["inet4_address"] = "172.19.0.1/28";
        tunInbound["mtu"] = 9000;
        tunInbound["auto_route"] = true;
        tunInbound["strict_route"] = true;
        tunInbound["stack"] = "system";
        tunInbound["endpoint_independent_nat"] = true;
        tunInbound["sniff"] = true;
        inbounds.append(tunInbound);
        config["inbounds"] = inbounds;
        
        // Outbounds
        QJsonArray outbounds;
        
        // SOCKS outbound (connect to main proxy)
        QJsonObject socksOutbound;
        socksOutbound["type"] = "socks";
        socksOutbound["tag"] = "proxy";
        socksOutbound["server"] = "127.0.0.1";
        socksOutbound["server_port"] = 2080;
        socksOutbound["udp_fragment"] = true;
        outbounds.append(socksOutbound);
        
        // Direct outbound
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
        route["final"] = "proxy";
        route["auto_detect_interface"] = true;
        
        QJsonArray rules;
        
        // Block multicast
        QJsonObject blockMulticast;
        QJsonArray multicastIps = {"224.0.0.0/3", "ff00::/8"};
        blockMulticast["ip_cidr"] = multicastIps;
        blockMulticast["outbound"] = "block";
        rules.append(blockMulticast);
        
        // DNS rules
        QJsonObject dnsRule;
        dnsRule["port"] = 53;
        dnsRule["outbound"] = "dns-out";
        rules.append(dnsRule);
        
        route["rules"] = rules;
        config["route"] = route;

        // Write TUN config to file
        m_tunConfigPath = QDir::temp().absoluteFilePath("nekoray_tun.json");
        QFile file(m_tunConfigPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to write TUN config file:" << m_tunConfigPath;
            return false;
        }

        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();

#ifndef Q_OS_WIN
        // Generate script for Linux/macOS privilege escalation
        m_tunScriptPath = QDir::temp().absoluteFilePath("nekoray_tun.sh");
        QFile scriptFile(m_tunScriptPath);
        if (scriptFile.open(QIODevice::WriteOnly)) {
            QTextStream stream(&scriptFile);
            stream << "#!/bin/bash\n";
            stream << "echo \"Starting NekoRay TUN mode...\"\n";
            stream << "exec \"" << m_corePath << "\" --disable-color run -c \"" << m_tunConfigPath << "\"\n";
            scriptFile.close();
            
            // Make script executable
            QProcess::execute("chmod", {"+x", m_tunScriptPath});
        }
#endif

        qDebug() << "Generated TUN config at:" << m_tunConfigPath;
        return true;
    }

    bool TunManager::startTunProcess() {
        m_tunProcess = new QProcess(this);
        connect(m_tunProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &TunManager::onProcessFinished);
        connect(m_tunProcess, &QProcess::readyReadStandardOutput,
                this, &TunManager::onReadyReadStandardOutput);
        connect(m_tunProcess, &QProcess::readyReadStandardError,
                this, &TunManager::onReadyReadStandardError);

        QStringList arguments;
        QString program;

#ifdef Q_OS_WIN
        // Windows: Run core directly with elevated privileges
        program = m_corePath;
        arguments << "--disable-color" << "run" << "-c" << m_tunConfigPath;
        
        // Note: On Windows, you might need additional privilege escalation
        m_tunProcess->start(program, arguments);
#else
        // Linux/macOS: Use script with pkexec/osascript for privilege escalation
#ifdef Q_OS_MACOS
        program = "osascript";
        arguments << "-e" << QString("do shell script \"bash %1\" with administrator privileges")
                                .arg(m_tunScriptPath);
#else
        program = "pkexec";
        arguments << "bash" << m_tunScriptPath;
#endif
        m_tunProcess->start(program, arguments);
#endif

        if (!m_tunProcess->waitForStarted(5000)) {
            qWarning() << "Failed to start TUN process:" << m_tunProcess->errorString();
            m_tunProcess->deleteLater();
            m_tunProcess = nullptr;
            return false;
        }

        emit logOutput(QString("TUN process started with PID %1").arg(m_tunProcess->processId()));
        return true;
    }

} // namespace NekoCore