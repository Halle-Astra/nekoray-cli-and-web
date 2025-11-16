#include "NekoService_Fixed.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace NekoCore {

    ConfigManager::ConfigManager(QObject *parent)
        : QObject(parent)
    {
    }

    ConfigManager::~ConfigManager() {
    }

    bool ConfigManager::initialize(const QString &configDir) {
        m_configDir = configDir;
        m_configFile = m_configDir + "/groups/nekobox.json";
        
        QDir dir(m_configDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qWarning() << "Failed to create config directory:" << m_configDir;
                return false;
            }
        }

        if (!dir.exists("groups")) {
            if (!dir.mkdir("groups")) {
                qWarning() << "Failed to create groups directory";
                return false;
            }
        }

        return true;
    }

    bool ConfigManager::load() {
        QFile file(m_configFile);
        if (!file.exists()) {
            // Create default config
            m_dataStore = QJsonObject();
            return save();
        }

        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open config file:" << m_configFile;
            return false;
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        file.close();

        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << error.errorString();
            return false;
        }

        m_dataStore = doc.object();
        return true;
    }

    bool ConfigManager::save() {
        QFile file(m_configFile);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to open config file for writing:" << m_configFile;
            return false;
        }

        QJsonDocument doc(m_dataStore);
        file.write(doc.toJson());
        file.close();

        return true;
    }

} // namespace NekoCore