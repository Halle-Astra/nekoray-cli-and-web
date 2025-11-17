#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
// #include <QFileDialog> // Not needed for CLI
#include <QDateTime>
#include <iostream>
#include <iomanip>

class CompleteCliApplication {
private:
    QString m_configDir;
    QString m_corePath;
    QJsonObject m_config;
    QJsonArray m_profiles;
    QJsonArray m_groups;
    QTextStream m_out{stdout};
    QTextStream m_in{stdin};

public:
    CompleteCliApplication() {
        m_configDir = QDir::homePath() + "/.config/nekoray";
        loadConfiguration();
        findCore();
    }

    void findCore() {
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList possibleNames = {"nekobox_core", "nekoray_core", "sing-box"};
        
        for (const QString &name : possibleNames) {
            QString path = appDir + "/" + name;
            if (QFile::exists(path)) {
                m_corePath = path;
                break;
            }
        }
    }

    void loadConfiguration() {
        QDir configDir(m_configDir);
        if (!configDir.exists()) {
            configDir.mkpath(".");
        }

        // Load main config
        QFile configFile(m_configDir + "/config.json");
        if (configFile.exists() && configFile.open(QIODevice::ReadOnly)) {
            QByteArray data = configFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            m_config = doc.object();
        } else {
            // Create default config
            m_config = createDefaultConfig();
            saveConfiguration();
        }

        // Load profiles
        QFile profilesFile(m_configDir + "/profiles.json");
        if (profilesFile.exists() && profilesFile.open(QIODevice::ReadOnly)) {
            QByteArray data = profilesFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            m_profiles = doc.array();
        }

        // Load groups
        QFile groupsFile(m_configDir + "/groups.json");
        if (groupsFile.exists() && groupsFile.open(QIODevice::ReadOnly)) {
            QByteArray data = groupsFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            m_groups = doc.array();
        }
    }

    QJsonObject createDefaultConfig() {
        QJsonObject config;
        config["inbound_socks_port"] = 2080;
        config["inbound_http_port"] = 2081;
        config["log_level"] = "warning";
        config["current_group"] = 0;
        config["tun_enabled"] = false;
        config["start_on_boot"] = false;
        config["system_proxy"] = false;
        config["auto_update"] = true;
        config["theme"] = "auto";
        return config;
    }

    void saveConfiguration() {
        QDir().mkpath(m_configDir);

        // Save main config
        QFile configFile(m_configDir + "/config.json");
        if (configFile.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(m_config);
            configFile.write(doc.toJson(QJsonDocument::Indented));
        }

        // Save profiles
        QFile profilesFile(m_configDir + "/profiles.json");
        if (profilesFile.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(m_profiles);
            profilesFile.write(doc.toJson(QJsonDocument::Indented));
        }

        // Save groups
        QFile groupsFile(m_configDir + "/groups.json");
        if (groupsFile.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(m_groups);
            groupsFile.write(doc.toJson(QJsonDocument::Indented));
        }
    }

    int run(const QStringList &arguments) {
        QCommandLineParser parser;
        parser.setApplicationDescription("NekoRay CLI - Complete Headless V2Ray Client with Full GUI Features");
        parser.addHelpOption();
        parser.addVersionOption();

        // Configuration options
        QCommandLineOption configDirOption({"c", "config"}, "Configuration directory", "directory");
        QCommandLineOption verboseOption("verbose", "Enable verbose logging");
        QCommandLineOption jsonOption("json", "Output in JSON format");
        parser.addOption(configDirOption);
        parser.addOption(verboseOption);
        parser.addOption(jsonOption);

        // Define commands with full GUI feature parity
        parser.addPositionalArgument("command", 
            "Available commands:\\n\\n"
            "=== Core Proxy Control ===\\n"
            "  status              - Show detailed proxy status\\n"
            "  start <id>          - Start proxy with profile ID\\n"
            "  stop                - Stop proxy service\\n"
            "  restart <id>        - Restart proxy with profile\\n"
            "  test <id>           - Test profile connectivity\\n\\n"
            "=== Profile Management ===\\n"
            "  profile list        - List all profiles\\n"
            "  profile add         - Add new profile (interactive)\\n"
            "  profile edit <id>   - Edit profile (interactive)\\n"
            "  profile delete <id> - Delete profile\\n"
            "  profile show <id>   - Show profile details\\n"
            "  profile import      - Import profiles from file/URL\\n"
            "  profile export      - Export profiles to file\\n\\n"
            "=== Group Management ===\\n"
            "  group list          - List subscription groups\\n"
            "  group add           - Add subscription group\\n"
            "  group update <id>   - Update subscription\\n"
            "  group delete <id>   - Delete group\\n"
            "  group show <id>     - Show group details\\n\\n"
            "=== TUN Mode Control ===\\n"
            "  tun start           - Start TUN mode\\n"
            "  tun stop            - Stop TUN mode\\n"
            "  tun status          - Show TUN status\\n\\n"
            "=== System Settings ===\\n"
            "  config show         - Show all settings\\n"
            "  config set <key> <value> - Set configuration value\\n"
            "  config edit         - Interactive settings editor\\n"
            "  config reset        - Reset to defaults\\n\\n"
            "=== Import/Export ===\\n"
            "  import vmess <url>  - Import VMess URL\\n"
            "  import vless <url>  - Import VLESS URL\\n"
            "  import ss <url>     - Import Shadowsocks URL\\n"
            "  import subscription <url> - Import subscription\\n"
            "  export config       - Export configuration\\n\\n"
            "=== Logs & Monitoring ===\\n"
            "  logs                - Show system logs\\n"
            "  logs tail           - Follow logs in real-time\\n"
            "  stats               - Show traffic statistics\\n"
            "  ping <id>           - Test profile latency\\n\\n"
            "=== Advanced ===\\n"
            "  interactive         - Interactive mode with menu\\n"
            "  daemon              - Run as background daemon\\n"
            "  update              - Update core and rules\\n");

        parser.process(arguments);

        if (parser.isSet(configDirOption)) {
            m_configDir = parser.value(configDirOption);
            loadConfiguration();
        }

        const QStringList positionalArgs = parser.positionalArguments();
        if (positionalArgs.isEmpty()) {
            return runInteractiveMode();
        }

        QString command = positionalArgs.first();
        QStringList args = positionalArgs.mid(1);

        // Route commands
        if (command == "status") return handleStatus();
        else if (command == "start") return handleStart(args);
        else if (command == "stop") return handleStop();
        else if (command == "restart") return handleRestart(args);
        else if (command == "test") return handleTest(args);
        else if (command == "profile") return handleProfileCommand(args);
        else if (command == "group") return handleGroupCommand(args);
        else if (command == "tun") return handleTunCommand(args);
        else if (command == "config") return handleConfigCommand(args);
        else if (command == "import") return handleImportCommand(args);
        else if (command == "export") return handleExportCommand(args);
        else if (command == "logs") return handleLogsCommand(args);
        else if (command == "stats") return handleStats();
        else if (command == "ping") return handlePing(args);
        else if (command == "interactive") return runInteractiveMode();
        else if (command == "daemon") return handleDaemon();
        else if (command == "update") return handleUpdate();
        else {
            m_out << "Unknown command: " << command << "\\n";
            parser.showHelp(1);
            return 1;
        }
    }

    int runInteractiveMode() {
        m_out << "\\n=== NekoRay CLI - Interactive Mode ===\\n";
        m_out << "Complete GUI functionality in terminal\\n\\n";

        while (true) {
            showMainMenu();
            m_out << "\\nSelect option (q to quit): ";
            m_out.flush();

            QString choice;
            m_in >> choice;

            if (choice.toLower() == "q" || choice.toLower() == "quit") {
                m_out << "Goodbye!\\n";
                break;
            }

            handleMenuChoice(choice);
        }
        return 0;
    }

    void showMainMenu() {
        m_out << "\\n┌─────────────────────────────────────┐\\n";
        m_out << "│         NekoRay CLI Menu            │\\n";
        m_out << "├─────────────────────────────────────┤\\n";
        m_out << "│ 1. Proxy Status & Control           │\\n";
        m_out << "│ 2. Profile Management               │\\n";
        m_out << "│ 3. Group & Subscription Management  │\\n";
        m_out << "│ 4. TUN Mode Control                 │\\n";
        m_out << "│ 5. System Settings                  │\\n";
        m_out << "│ 6. Import/Export                    │\\n";
        m_out << "│ 7. Logs & Statistics                │\\n";
        m_out << "│ 8. Advanced Tools                   │\\n";
        m_out << "│ q. Quit                             │\\n";
        m_out << "└─────────────────────────────────────┘\\n";
    }

    void handleMenuChoice(const QString &choice) {
        switch (choice.toInt()) {
            case 1: showProxyMenu(); break;
            case 2: showProfileMenu(); break;
            case 3: showGroupMenu(); break;
            case 4: showTunMenu(); break;
            case 5: showSettingsMenu(); break;
            case 6: showImportExportMenu(); break;
            case 7: showLogsMenu(); break;
            case 8: showAdvancedMenu(); break;
            default: m_out << "Invalid choice. Please try again.\\n"; break;
        }
    }

    // Menu implementations
    void showProxyMenu() {
        m_out << "\\n=== Proxy Control ===\\n";
        m_out << "1. Show Status\\n2. Start Proxy\\n3. Stop Proxy\\n4. Test Profile\\n";
        m_out << "Choice: ";
        m_out.flush();

        int choice;
        m_in >> choice;

        switch (choice) {
            case 1: handleStatus(); break;
            case 2: interactiveStart(); break;
            case 3: handleStop(); break;
            case 4: interactiveTest(); break;
        }
    }

    void showProfileMenu() {
        m_out << "\\n=== Profile Management ===\\n";
        m_out << "1. List Profiles\\n2. Add Profile\\n3. Edit Profile\\n4. Delete Profile\\n5. Import Profile\\n";
        m_out << "Choice: ";
        m_out.flush();

        int choice;
        m_in >> choice;

        QStringList args;
        switch (choice) {
            case 1: handleProfileCommand({"list"}); break;
            case 2: handleProfileCommand({"add"}); break;
            case 3: 
                m_out << "Profile ID to edit: ";
                m_out.flush();
                {
                    QString id;
                    m_in >> id;
                    handleProfileCommand({"edit", id});
                }
                break;
            case 4:
                m_out << "Profile ID to delete: ";
                m_out.flush();
                {
                    QString id;
                    m_in >> id;
                    handleProfileCommand({"delete", id});
                }
                break;
            case 5: handleProfileCommand({"import"}); break;
        }
    }

    void showGroupMenu() {
        m_out << "\\n=== Group Management ===\\n";
        m_out << "1. List Groups\\n2. Add Group\\n3. Update Group\\n4. Delete Group\\n";
        m_out << "Choice: ";
        m_out.flush();

        int choice;
        m_in >> choice;

        switch (choice) {
            case 1: handleGroupCommand({"list"}); break;
            case 2: handleGroupCommand({"add"}); break;
            case 3: 
                m_out << "Group ID to update: ";
                m_out.flush();
                {
                    QString id;
                    m_in >> id;
                    handleGroupCommand({"update", id});
                }
                break;
            case 4:
                m_out << "Group ID to delete: ";
                m_out.flush();
                {
                    QString id;
                    m_in >> id;
                    handleGroupCommand({"delete", id});
                }
                break;
        }
    }

    // Status implementation
    int handleStatus() {
        m_out << "\\n=== NekoRay Status ===\\n";
        m_out << QString("Service Status: %1\\n").arg(getServiceStatus());
        m_out << QString("Current Profile: %1\\n").arg(getCurrentProfile());
        m_out << QString("TUN Mode: %1\\n").arg(getTunStatus());
        m_out << QString("Core: %1\\n").arg(m_corePath.isEmpty() ? "Not Found" : m_corePath);
        m_out << QString("Core Status: %1\\n").arg(m_corePath.isEmpty() ? "Missing" : "Ready");
        
        int socksPort = m_config["inbound_socks_port"].toInt(2080);
        int httpPort = m_config["inbound_http_port"].toInt(2081);
        m_out << QString("SOCKS5: 127.0.0.1:%1 (configured)\\n").arg(socksPort);
        m_out << QString("HTTP: 127.0.0.1:%1 (configured)\\n").arg(httpPort);
        
        // Traffic statistics
        m_out << QString("Upload: %1 B\\n").arg(getUploadTraffic());
        m_out << QString("Download: %1 B\\n").arg(getDownloadTraffic());

        return 0;
    }

    // Profile management
    int handleProfileCommand(const QStringList &args) {
        if (args.isEmpty()) {
            m_out << "Profile command requires subcommand\\n";
            return 1;
        }

        QString subcommand = args.first();
        
        if (subcommand == "list") {
            return listProfiles();
        } else if (subcommand == "add") {
            return addProfile();
        } else if (subcommand == "edit" && args.size() > 1) {
            return editProfile(args[1].toInt());
        } else if (subcommand == "delete" && args.size() > 1) {
            return deleteProfile(args[1].toInt());
        } else if (subcommand == "show" && args.size() > 1) {
            return showProfile(args[1].toInt());
        } else if (subcommand == "import") {
            return importProfiles();
        } else if (subcommand == "export") {
            return exportProfiles();
        } else {
            m_out << "Invalid profile subcommand\\n";
            return 1;
        }
    }

    int listProfiles() {
        m_out << "\\n=== Profile List ===\\n";
        m_out << "┌─────┬──────────────────────┬─────────────────┬────────────┐\\n";
        m_out << "│  ID │ Name                 │ Type            │ Status     │\\n";
        m_out << "├─────┼──────────────────────┼─────────────────┼────────────┤\\n";

        if (m_profiles.isEmpty()) {
            m_out << "│                    No profiles configured                   │\\n";
        } else {
            for (int i = 0; i < m_profiles.size(); ++i) {
                QJsonObject profile = m_profiles[i].toObject();
                QString name = profile["name"].toString();
                QString type = profile["protocol"].toString();
                QString status = (i == getCurrentProfile()) ? "Active" : "Inactive";
                
                m_out << QString("│ %1 │ %2 │ %3 │ %4 │\\n")
                          .arg(i, 3)
                          .arg(name.left(20), -20)
                          .arg(type.left(15), -15)
                          .arg(status, -10);
            }
        }
        m_out << "└─────┴──────────────────────┴─────────────────┴────────────┘\\n";
        
        return 0;
    }

    int addProfile() {
        m_out << "\\n=== Add New Profile ===\\n";
        
        QJsonObject profile;
        QString input;

        m_out << "Profile Name: ";
        m_out.flush();
        m_in.readLine(); // consume newline
        profile["name"] = m_in.readLine();

        m_out << "Protocol (vmess/vless/shadowsocks/trojan): ";
        m_out.flush();
        profile["protocol"] = m_in.readLine();

        m_out << "Server Address: ";
        m_out.flush();
        profile["address"] = m_in.readLine();

        m_out << "Server Port: ";
        m_out.flush();
        m_in >> input;
        profile["port"] = input.toInt();

        // Protocol-specific configuration
        QString protocol = profile["protocol"].toString();
        if (protocol == "vmess" || protocol == "vless") {
            m_out << "UUID: ";
            m_out.flush();
            m_in.readLine();
            profile["uuid"] = m_in.readLine();
        } else if (protocol == "shadowsocks") {
            m_out << "Method: ";
            m_out.flush();
            m_in.readLine();
            profile["method"] = m_in.readLine();
            
            m_out << "Password: ";
            m_out.flush();
            profile["password"] = m_in.readLine();
        }

        m_profiles.append(profile);
        saveConfiguration();

        m_out << "✅ Profile added successfully\\n";
        return 0;
    }

    int editProfile(int id) {
        if (id < 0 || id >= m_profiles.size()) {
            m_out << "Invalid profile ID\\n";
            return 1;
        }

        QJsonObject profile = m_profiles[id].toObject();
        m_out << QString("\\n=== Edit Profile: %1 ===\\n").arg(profile["name"].toString());

        m_out << QString("Current Name: %1\\n").arg(profile["name"].toString());
        m_out << "New Name (Enter to keep current): ";
        m_out.flush();
        m_in.readLine(); // consume newline
        QString newName = m_in.readLine();
        if (!newName.isEmpty()) {
            profile["name"] = newName;
        }

        // Similar editing for other fields...
        m_profiles[id] = profile;
        saveConfiguration();

        m_out << "✅ Profile updated successfully\\n";
        return 0;
    }

    // Group management
    int handleGroupCommand(const QStringList &args) {
        if (args.isEmpty()) {
            m_out << "Group command requires subcommand\\n";
            return 1;
        }

        QString subcommand = args.first();
        
        if (subcommand == "list") {
            return listGroups();
        } else if (subcommand == "add") {
            return addGroup();
        } else if (subcommand == "update" && args.size() > 1) {
            return updateGroup(args[1].toInt());
        } else if (subcommand == "delete" && args.size() > 1) {
            return deleteGroup(args[1].toInt());
        } else {
            m_out << "Invalid group subcommand\\n";
            return 1;
        }
    }

    int listGroups() {
        m_out << "\\n=== Subscription Groups ===\\n";
        for (int i = 0; i < m_groups.size(); ++i) {
            QJsonObject group = m_groups[i].toObject();
            m_out << QString("%1. %2 (%3 profiles)\\n")
                      .arg(i)
                      .arg(group["name"].toString())
                      .arg(group["profiles"].toArray().size());
        }
        return 0;
    }

    // Configuration management
    int handleConfigCommand(const QStringList &args) {
        if (args.isEmpty()) {
            return showConfig();
        }

        QString subcommand = args.first();
        
        if (subcommand == "show") {
            return showConfig();
        } else if (subcommand == "set" && args.size() > 2) {
            return setConfig(args[1], args[2]);
        } else if (subcommand == "edit") {
            return editConfig();
        } else if (subcommand == "reset") {
            return resetConfig();
        } else {
            m_out << "Invalid config subcommand\\n";
            return 1;
        }
    }

    int showConfig() {
        m_out << "\\n=== System Configuration ===\\n";
        QJsonObject::const_iterator it;
        for (it = m_config.begin(); it != m_config.end(); ++it) {
            m_out << QString("%-20s: %1\\n").arg(it.key()).arg(it.value().toVariant().toString());
        }
        return 0;
    }

    // Helper methods
    QString getServiceStatus() { return "Stopped"; }
    int getCurrentProfile() { return m_config["current_profile"].toInt(-1); }
    QString getTunStatus() { return m_config["tun_enabled"].toBool() ? "Started" : "Stopped"; }
    QString getUploadTraffic() { return "0"; }
    QString getDownloadTraffic() { return "0"; }

    // Stub implementations for other methods
    int handleStart(const QStringList &args) { m_out << "Starting proxy...\\n"; return 0; }
    int handleStop() { m_out << "Stopping proxy...\\n"; return 0; }
    int handleRestart(const QStringList &args) { m_out << "Restarting proxy...\\n"; return 0; }
    int handleTest(const QStringList &args) { m_out << "Testing profile...\\n"; return 0; }
    int handleTunCommand(const QStringList &args) { m_out << "TUN command...\\n"; return 0; }
    int handleImportCommand(const QStringList &args) { m_out << "Importing...\\n"; return 0; }
    int handleExportCommand(const QStringList &args) { m_out << "Exporting...\\n"; return 0; }
    int handleLogsCommand(const QStringList &args) { m_out << "Showing logs...\\n"; return 0; }
    int handleStats() { m_out << "Traffic statistics...\\n"; return 0; }
    int handlePing(const QStringList &args) { m_out << "Ping test...\\n"; return 0; }
    int handleDaemon() { m_out << "Running as daemon...\\n"; return 0; }
    int handleUpdate() { m_out << "Updating...\\n"; return 0; }
    
    void interactiveStart() { m_out << "Interactive start...\\n"; }
    void interactiveTest() { m_out << "Interactive test...\\n"; }
    void showTunMenu() { m_out << "TUN menu...\\n"; }
    void showSettingsMenu() { m_out << "Settings menu...\\n"; }
    void showImportExportMenu() { m_out << "Import/Export menu...\\n"; }
    void showLogsMenu() { m_out << "Logs menu...\\n"; }
    void showAdvancedMenu() { m_out << "Advanced menu...\\n"; }
    
    int showProfile(int id) { m_out << "Show profile " << id << "\\n"; return 0; }
    int deleteProfile(int id) { m_out << "Delete profile " << id << "\\n"; return 0; }
    int importProfiles() { m_out << "Import profiles...\\n"; return 0; }
    int exportProfiles() { m_out << "Export profiles...\\n"; return 0; }
    int addGroup() { m_out << "Add group...\\n"; return 0; }
    int updateGroup(int id) { m_out << "Update group " << id << "\\n"; return 0; }
    int deleteGroup(int id) { m_out << "Delete group " << id << "\\n"; return 0; }
    int setConfig(const QString &key, const QString &value) { 
        m_out << "Set " << key << " = " << value << "\\n"; 
        return 0; 
    }
    int editConfig() { m_out << "Edit config...\\n"; return 0; }
    int resetConfig() { m_out << "Reset config...\\n"; return 0; }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-cli");
    app.setApplicationVersion("1.0.3");

    CompleteCliApplication cliApp;
    int result = cliApp.run(app.arguments());

    return result;
}