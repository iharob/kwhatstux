#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KXmlGuiWindow>

#include <memory>

class QTabWidget;
class QWebEngineNotification;
class QWebEngineView;
class QWebEngineProfile;
class QWebEnginePermission;
class KStatusNotifierItem;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QWebEngineView *addAccount();
    void setupActions();
    void setupTrayIcon();
    bool isAutostartEnabled() const;
    void setAutostartEnabled(bool enabled);
    void handlePermission(QWebEnginePermission permission);
    void handleNotification(std::unique_ptr<QWebEngineNotification> notification);
    void updateTabTitle(int index, const QString &title);
    void updateTrayBadge();
    void renameTab(int index);
    void saveAccountCount();
    void saveTabOrder();
    void restoreTabOrder();

    QTabWidget *m_tabs = nullptr;
    KStatusNotifierItem *m_trayIcon = nullptr;
    int m_accountCounter = 0;
};

#endif
