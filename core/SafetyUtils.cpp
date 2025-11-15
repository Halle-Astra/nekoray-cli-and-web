#include "SafetyUtils.hpp"
#include <QTcpServer>
#include <QNetworkInterface>
#include <QProcess>
#include <QFile>
#include <QDebug>
#include <QStandardPaths>

namespace NekoCore {

    SafetyUtils::SafetyUtils(QObject *parent) : QObject(parent) {}

    bool SafetyUtils::isSafeTestEnvironment() {
        // Check multiple indicators for safe environment
        bool isVM = checkVirtualMachine();
        bool hasSSH = checkSSHConnection();
        bool isProd = checkProductionEnvironment();
        
        if (!isVM && hasSSH) {
            qWarning() << "Warning: Detected SSH connection on non-VM environment";
            return false;
        }
        
        if (isProd) {
            qWarning() << "Warning: Detected production environment";
            return false;
        }
        
        return true;
    }

    bool SafetyUtils::isTunOperationSafe() {
        // Check if TUN operations are safe to perform
        if (!isSafeTestEnvironment()) {
            return false;
        }
        
        // Check for existing TUN interfaces
        QProcess process;
        process.start("ip", {"addr", "show", "type", "tun"});
        process.waitForFinished(3000);
        
        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput();
            if (!output.trimmed().isEmpty()) {
                qWarning() << "Warning: Existing TUN interfaces detected";
                return false;
            }
        }
        
        return true;
    }

    bool SafetyUtils::validateConfig(const QString &configPath) {
        QFile file(configPath);
        if (!file.exists()) {
            qWarning() << "Config file does not exist:" << configPath;
            return false;
        }
        
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot read config file:" << configPath;
            return false;
        }
        
        // Basic JSON validation
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        file.close();
        
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << error.errorString();
            return false;
        }
        
        // Check for required fields
        QJsonObject obj = doc.object();
        if (!obj.contains("inbounds") && !obj.contains("outbounds")) {
            qWarning() << "Config missing required inbound/outbound sections";
            return false;
        }
        
        return true;
    }

    QStringList SafetyUtils::getActiveNetworkConnections() {
        QStringList connections;
        
        QProcess process;
        process.start("netstat", {"-tuln"});
        process.waitForFinished(3000);
        
        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split('\n');
            
            for (const QString &line : lines) {
                if (line.contains("LISTEN") || line.contains("ESTABLISHED")) {
                    connections.append(line.trimmed());
                }
            }
        }
        
        return connections;
    }

    bool SafetyUtils::backupNetworkState(const QString &backupPath) {
        // Backup routing table
        QProcess routeProcess;
        routeProcess.start("ip", {"route", "show"});
        routeProcess.waitForFinished(3000);
        
        if (routeProcess.exitCode() != 0) {
            qWarning() << "Failed to get routing table";
            return false;
        }
        
        QString routes = routeProcess.readAllStandardOutput();
        
        // Backup DNS settings
        QProcess dnsProcess;
        dnsProcess.start("systemd-resolve", {"--status"});
        dnsProcess.waitForFinished(3000);
        
        QString dns;
        if (dnsProcess.exitCode() == 0) {
            dns = dnsProcess.readAllStandardOutput();
        }
        
        // Write backup file
        QFile backupFile(backupPath);
        if (!backupFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to create backup file:" << backupPath;
            return false;
        }
        
        QTextStream stream(&backupFile);
        stream << "# Network State Backup\n";
        stream << "# Generated: " << QDateTime::currentDateTime().toString() << "\n\n";
        stream << "# Routes:\n" << routes << "\n";
        stream << "# DNS:\n" << dns << "\n";
        
        backupFile.close();
        qDebug() << "Network state backed up to:" << backupPath;
        return true;
    }

    bool SafetyUtils::restoreNetworkState(const QString &backupPath) {
        Q_UNUSED(backupPath)
        // This is a placeholder - in real implementation, you'd parse the backup
        // and restore network settings. This is complex and dangerous, so we
        // just log a warning for now.
        qWarning() << "Network state restoration not implemented - manual recovery required";
        return false;
    }

    bool SafetyUtils::isPortAvailable(int port, const QString &interface) {
        QTcpServer server;
        QHostAddress address(interface);
        
        if (server.listen(address, port)) {
            server.close();
            return true;
        }
        
        return false;
    }

    bool SafetyUtils::validateOperationDryRun(const QString &operation, const QVariantMap &params) {
        qDebug() << "DRY RUN: Would perform operation:" << operation << "with params:" << params;
        
        if (operation == "start_proxy") {
            int profileId = params.value("profile_id", -1).toInt();
            if (profileId <= 0) {
                qWarning() << "DRY RUN: Invalid profile ID:" << profileId;
                return false;
            }
            qDebug() << "DRY RUN: Would start proxy with profile" << profileId;
        } else if (operation == "start_tun") {
            if (!isTunOperationSafe()) {
                qWarning() << "DRY RUN: TUN operation would not be safe";
                return false;
            }
            qDebug() << "DRY RUN: Would start TUN mode";
        } else if (operation == "bind_port") {
            int port = params.value("port", 0).toInt();
            QString interface = params.value("interface", "127.0.0.1").toString();
            if (!isPortAvailable(port, interface)) {
                qWarning() << "DRY RUN: Port" << port << "not available on" << interface;
                return false;
            }
            qDebug() << "DRY RUN: Would bind to" << interface << ":" << port;
        }
        
        return true;
    }

    bool SafetyUtils::checkVirtualMachine() {
        // Check common VM indicators
        QFile file("/proc/cpuinfo");
        if (file.open(QIODevice::ReadOnly)) {
            QString content = file.readAll();
            if (content.contains("hypervisor") || 
                content.contains("VMware") || 
                content.contains("VirtualBox") ||
                content.contains("QEMU")) {
                return true;
            }
        }
        
        // Check DMI info
        QProcess process;
        process.start("dmidecode", {"-s", "system-manufacturer"});
        process.waitForFinished(3000);
        
        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().toLower();
            if (output.contains("vmware") || 
                output.contains("virtualbox") ||
                output.contains("qemu") ||
                output.contains("microsoft corporation")) {
                return true;
            }
        }
        
        return false;
    }

    bool SafetyUtils::checkSSHConnection() {
        QString sshClient = qgetenv("SSH_CLIENT");
        QString sshConnection = qgetenv("SSH_CONNECTION");
        
        return !sshClient.isEmpty() || !sshConnection.isEmpty();
    }

    bool SafetyUtils::checkProductionEnvironment() {
        // Check for production environment indicators
        
        // Check hostname
        QProcess process;
        process.start("hostname", {"-f"});
        process.waitForFinished(3000);
        
        if (process.exitCode() == 0) {
            QString hostname = process.readAllStandardOutput().trimmed().toLower();
            if (hostname.contains("prod") || 
                hostname.contains("production") ||
                hostname.contains("live") ||
                hostname.contains("master")) {
                return true;
            }
        }
        
        // Check for production files/directories
        QStringList prodIndicators = {
            "/etc/production",
            "/var/www",
            "/opt/production",
            "/.production"
        };
        
        for (const QString &indicator : prodIndicators) {
            if (QFile::exists(indicator)) {
                return true;
            }
        }
        
        // Check running services
        process.start("systemctl", {"list-units", "--type=service", "--state=running"});
        process.waitForFinished(3000);
        
        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().toLower();
            if (output.contains("nginx") || 
                output.contains("apache") ||
                output.contains("mysql") ||
                output.contains("postgresql") ||
                output.contains("redis")) {
                return true;
            }
        }
        
        return false;
    }

} // namespace NekoCore