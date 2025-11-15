#pragma once

#include <QString>
#include <QObject>

namespace NekoCore {

    // Safety utilities for network operations
    class SafetyUtils : public QObject {
        Q_OBJECT

    public:
        explicit SafetyUtils(QObject *parent = nullptr);

        // Check if we're in a safe environment for testing
        static bool isSafeTestEnvironment();
        
        // Check if TUN operations are safe
        static bool isTunOperationSafe();
        
        // Validate configuration before applying
        static bool validateConfig(const QString &configPath);
        
        // Check for existing network connections that might be disrupted
        static QStringList getActiveNetworkConnections();
        
        // Backup current network state
        static bool backupNetworkState(const QString &backupPath);
        
        // Restore network state from backup
        static bool restoreNetworkState(const QString &backupPath);
        
        // Check if port is available
        static bool isPortAvailable(int port, const QString &interface = "127.0.0.1");
        
        // Dry run mode - validate operations without executing
        static bool validateOperationDryRun(const QString &operation, const QVariantMap &params);

    signals:
        void safetyWarning(const QString &warning);
        void criticalError(const QString &error);

    private:
        static bool checkVirtualMachine();
        static bool checkSSHConnection();
        static bool checkProductionEnvironment();
    };

} // namespace NekoCore