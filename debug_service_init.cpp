#include <QCoreApplication>
#include <QTextStream>
#include <QDebug>
#include <QTimer>
#include "nekoray/core/NekoService_Fixed.hpp"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QTextStream out(stdout);
    out << "=== DEBUGGING SERVICE INITIALIZATION ===" << Qt::endl;
    out.flush();
    
    out << "Step 1: Creating NekoService..." << Qt::endl;
    out.flush();
    
    NekoCore::NekoService *service = new NekoCore::NekoService();
    
    out << "Step 2: NekoService created successfully" << Qt::endl;
    out.flush();
    
    out << "Step 3: Initializing service..." << Qt::endl;
    out.flush();
    
    // Add a timer to force exit if it hangs
    QTimer::singleShot(3000, [&]() {
        out << "*** TIMEOUT - Service initialization took too long! ***" << Qt::endl;
        app.exit(1);
    });
    
    bool initResult = service->initialize("");
    
    out << "Step 4: Initialize result: " << (initResult ? "SUCCESS" : "FAILED") << Qt::endl;
    out.flush();
    
    if (initResult) {
        out << "Step 5: Getting status..." << Qt::endl;
        out.flush();
        
        QString status = service->getStatusString();
        out << "Service Status: " << status << Qt::endl;
        out.flush();
    }
    
    out << "=== DEBUGGING COMPLETE ===" << Qt::endl;
    out.flush();
    
    delete service;
    return 0;
}