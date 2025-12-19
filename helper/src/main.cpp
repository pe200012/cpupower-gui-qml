// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <signal.h>

#include "helperservice.h"

static void signalHandler(int)
{
    qInfo() << "Signal received, shutting down...";
    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    app.setApplicationName(QStringLiteral("cpupower-gui-helper"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationDomain(QStringLiteral("github.io"));
    
    // Handle signals for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create and register service
    HelperService service;
    
    if (!service.registerService()) {
        qCritical() << "Failed to register D-Bus service";
        return 1;
    }
    
    qInfo() << "cpupower-gui-helper started successfully";
    qInfo() << "Service: io.github.cpupower_gui.qt.helper";
    qInfo() << "Object:  /io/github/cpupower_gui/qt/helper";
    
    return app.exec();
}
