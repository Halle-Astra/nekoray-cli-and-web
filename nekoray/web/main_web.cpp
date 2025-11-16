// NekoRay Web API Server 主入口点
#include "SimpleWebServer.hpp"
#include "../core/NekoService_Fixed.hpp"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <iostream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("nekoray-web");
    app.setApplicationVersion("1.0");
    // app.setApplicationDisplayName("NekoRay Web API Server"); // Not available in Qt5

    QCommandLineParser parser;
    parser.setApplicationDescription("NekoRay Web API Server - Headless V2Ray Client");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption({"p", "port"}, "Web server port", "port", "8080");
    QCommandLineOption hostOption("host", "Web server host", "host", "127.0.0.1");
    QCommandLineOption verboseOption("verbose", "Enable verbose output");
    QCommandLineOption configDirOption({"c", "config-dir"}, "Configuration directory", "dir");

    parser.addOption(portOption);
    parser.addOption(hostOption);
    parser.addOption(verboseOption);
    parser.addOption(configDirOption);

    parser.process(app);

    // 获取参数
    bool verbose = parser.isSet(verboseOption);
    int port = parser.value(portOption).toInt();
    QString host = parser.value(hostOption);
    QString configDir = parser.value(configDirOption);

    if (port <= 0 || port > 65535) {
        std::cerr << "Error: Invalid port number: " << port << std::endl;
        return 1;
    }

    if (verbose) {
        std::cout << "NekoRay Web API Server v1.0" << std::endl;
        std::cout << "============================" << std::endl;
        std::cout << "Host: " << host.toStdString() << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Config Dir: " << configDir.toStdString() << std::endl;
    }

    // 创建核心服务
    NekoCore::NekoService service;
    if (!service.initialize(configDir)) {
        std::cerr << "Error: Failed to initialize NekoRay service" << std::endl;
        return 1;
    }

    // 创建 Web API 服务器
    NekoWeb::SimpleWebServer server(&service);

    // 启动服务器
    if (!server.start(host, port)) {
        std::cerr << "Error: Failed to start web server on " 
                  << host.toStdString() << ":" << port << std::endl;
        return 1;
    }

    std::cout << "🚀 NekoRay Web API Server started!" << std::endl;
    std::cout << "   URL: http://" << host.toStdString() << ":" << port << std::endl;
    std::cout << "   API: http://" << host.toStdString() << ":" << port << "/api/" << std::endl;
    std::cout << "   Press Ctrl+C to stop" << std::endl;

    // 运行事件循环
    int result = app.exec();

    if (verbose) {
        std::cout << "Web server stopped." << std::endl;
    }

    return result;
}

