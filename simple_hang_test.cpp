#include <QCoreApplication>
#include <QTextStream>
#include <QDebug>
#include <QTimer>
#include <QCommandLineParser>
#include <QThread>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("hang-test");
    app.setApplicationVersion("1.0");
    
    QTextStream out(stdout);
    out << "=== HANG DIAGNOSTIC TEST ===" << Qt::endl;
    out.flush();
    
    // Test QCommandLineParser
    out << "Testing QCommandLineParser..." << Qt::endl;
    out.flush();
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Test");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Process arguments like original CLI does
    parser.process(app.arguments());
    
    out << "QCommandLineParser completed successfully" << Qt::endl;
    out.flush();
    
    // Check if we got a status command
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty() && args.first() == "status") {
        out << "Processing status command..." << Qt::endl;
        out << "Service Status: Stopped" << Qt::endl;
        out << "Current Profile: -1" << Qt::endl;
        out << "TUN Mode: Stopped" << Qt::endl;
        out.flush();
        return 0;
    }
    
    out << "No status command provided" << Qt::endl;
    out.flush();
    return 0;
}