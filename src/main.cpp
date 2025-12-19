// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>

#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "application.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application domain for translations
    KLocalizedString::setApplicationDomain("cpupower-gui");
    
    // Set up KDE about data
    KAboutData aboutData(
        QStringLiteral("cpupower-gui"),
        i18n("CPU Power GUI"),
        QStringLiteral(CPUPOWER_GUI_VERSION_STRING),
        i18n("GUI utility to change the CPU frequency and governor"),
        KAboutLicense::GPL_V3,
        i18n("Copyright (C) 2017-2024 [RnD]²"),
        QString(),
        QStringLiteral("https://github.com/vagnum08/cpupower-gui")
    );
    
    aboutData.addAuthor(
        i18n("Evangelos Rigas"),
        i18n("Original Author"),
        QStringLiteral("erigas@rnd2.org")
    );
    
    aboutData.setOrganizationDomain("github.io");
    aboutData.setDesktopFileName(QStringLiteral("io.github.cpupower_gui.qt"));
    
    KAboutData::setApplicationData(aboutData);
    
    // Set application icon
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("io.github.cpupower_gui.qt")));
    
    // Use org.kde.desktop style on KDE, fall back to default otherwise
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
    
    // Create application controller
    Application appController;
    
    // Set up QML engine
    QQmlApplicationEngine engine;
    
    // Add KDE i18n context
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    
    // Register application controller to QML
    appController.setupQmlEngine(&engine);
    
    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    
    engine.load(url);
    
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    
    return app.exec();
}
