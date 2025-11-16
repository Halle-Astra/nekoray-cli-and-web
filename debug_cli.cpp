#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QTextStream out(stdout);
    out << "=== DEBUG CLI TEST ===" << Qt::endl;
    out << "Step 1: Application created" << Qt::endl;
    out.flush();
    
    // Test basic functionality without the complex service
    if (argc > 1 && QString(argv[1]) == "status") {
        out << "Service Status: Stopped" << Qt::endl;
        out << "Current Profile: -1" << Qt::endl; 
        out << "TUN Mode: Stopped" << Qt::endl;
        out << "=== STATUS COMPLETE ===" << Qt::endl;
        out.flush();
        return 0;
    }
    
    if (argc > 1 && QString(argv[1]) == "--version") {
        out << "nekoray-debug 1.0.3" << Qt::endl;
        out.flush();
        return 0;
    }
    
    out << "Usage: debug_cli [status|--version]" << Qt::endl;
    out.flush();
    return 0;
}