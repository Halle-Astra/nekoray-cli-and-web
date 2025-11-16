#include "NekoService.hpp"
#include "../main/NekoGui.hpp"
#include "../main/NekoGui_Utils.hpp"
#include "../db/ConfigBuilder.hpp"
#include "../fmt/AbstractBean.hpp"

#include <QApplication>
#include <QDir>
#include <QTextStream>
#include <QDebug>

namespace NekoCore {

    CoreManager::CoreManager(QObject *parent)
        : QObject(parent)
        , m_process(nullptr)
        , m_currentProfileId(-1)
    {
        // Find core executable
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

        if (m_corePath.isEmpty()) {
            qWarning() << "Core executable not found";
        }
    }

    CoreManager::~CoreManager() {
        if (m_process) {
            stop();
        }
    }

    bool CoreManager::start(int profileId) {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            return true; // Already running
        }

        if (m_corePath.isEmpty()) {
            qWarning() << "Core path not set";
            return false;
        }

        if (!generateConfig(profileId)) {
            qWarning() << "Failed to generate config";
            return false;
        }

        m_process = new QProcess(this);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &CoreManager::processFinished);
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
        auto profile = NekoGui::profileManager->GetProfile(profileId);
        if (!profile) {
            qWarning() << "Profile not found:" << profileId;
            return false;
        }

        // Build configuration using existing ConfigBuilder
        NekoGui::BuildConfigStatus status;
        status.ent = profile;
        status.result = std::make_shared<NekoGui::BuildConfigResult>();

        auto core_box = NekoGui::BuildConfig(profile, false);
        if (core_box.isEmpty()) {
            qWarning() << "Failed to build config for profile:" << profileId;
            return false;
        }

        // Write config to file
        m_configPath = QDir::temp().absoluteFilePath(QString("nekoray_core_%1.json").arg(profileId));
        QFile file(m_configPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to write config file:" << m_configPath;
            return false;
        }

        file.write(core_box.toUtf8());
        file.close();

        return true;
    }

} // namespace NekoCore