#include "NekoService.hpp"
#include "../main/NekoGui.hpp"
#include "../main/NekoGui_Utils.hpp"
#include "../db/ConfigBuilder.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStandardPaths>

namespace NekoCore {

    TunManager::TunManager(QObject *parent)
        : QObject(parent)
        , m_tunProcess(nullptr)
    {
        // Find core executable for TUN mode
        QString appDir = QApplication::applicationDirPath();
        QStringList possibleNames = {"nekobox_core", "nekoray_core"};
        
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
        // Use existing VPN config generation from NekoGui
        m_tunConfigPath = NekoGui::WriteVPNSingBoxConfig();
        
        if (m_tunConfigPath.isEmpty() || !QFile::exists(m_tunConfigPath)) {
            qWarning() << "Failed to generate TUN config";
            return false;
        }

#ifndef Q_OS_WIN
        // Generate script for Linux/macOS privilege escalation
        m_tunScriptPath = NekoGui::WriteVPNLinuxScript(m_tunConfigPath);
        if (m_tunScriptPath.isEmpty() || !QFile::exists(m_tunScriptPath)) {
            qWarning() << "Failed to generate TUN script";
            return false;
        }
#endif

        return true;
    }

    bool TunManager::startTunProcess() {
        m_tunProcess = new QProcess(this);
        connect(m_tunProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &TunManager::processFinished);
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
        // This is a simplified version - in production, you'd use WinCommander or similar
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