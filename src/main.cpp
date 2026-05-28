#include <QApplication>
#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KAboutData aboutData(
        QStringLiteral("kwhatstux"),
        i18n("KWhatsTux"),
        QStringLiteral("1.0.0"),
        i18n("WhatsApp Web client for KDE"),
        KAboutLicense::GPL_V3,
        i18n("© 2024–2026 KWhatsTux contributors"),
        QString(),
        QStringLiteral("https://gitlab.com/nexxontech/whatstux")
    );
    aboutData.addAuthor(i18n("Riccardo Sacchetto"), i18n("Original WhatsTux author"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kwhatstux"));

    KAboutData::setApplicationData(aboutData);
    auto icon = QIcon::fromTheme(QStringLiteral("kwhatstux"));
    if (icon.isNull())
        icon = QIcon(QStringLiteral(":/icons/kwhatstux.svg"));
    QApplication::setWindowIcon(icon);

    KCrash::initialize();

    KDBusService service(KDBusService::Unique);

    auto *window = new MainWindow;
    window->show();

    QObject::connect(&service, &KDBusService::activateRequested, window, [window]() {
        window->show();
        window->raise();
        window->activateWindow();
    });

    return app.exec();
}
