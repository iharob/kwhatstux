#include "mainwindow.h"

#include <QCloseEvent>
#include <QIcon>
#include <QInputDialog>
#include <QMenuBar>
#include <QMouseEvent>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWebEngineNotification>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineView>

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>
#include <KStandardAction>
#include <KStatusNotifierItem>

static const QString userAgent = QStringLiteral(
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36");

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setStyleSheet(QStringLiteral(
        "QTabWidget::tab-bar { alignment: center; }"
        "QTabBar::tab:!selected { background: palette(window); }"
    ));
    m_tabs->tabBar()->installEventFilter(this);

    auto *addWrapper = new QWidget(this);
    auto *addLayout = new QHBoxLayout(addWrapper);
    addLayout->setContentsMargins(4, 2, 4, 2);
    auto *addButton = new QToolButton(addWrapper);
    addButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addButton->setToolTip(i18n("New Account"));
    addButton->setAutoRaise(true);
    addLayout->addWidget(addButton);
    connect(addButton, &QToolButton::clicked, this, &MainWindow::addAccount);
    m_tabs->setCornerWidget(addWrapper, Qt::TopRightCorner);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tabs);
    setCentralWidget(container);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (m_tabs->count() <= 1)
            return;
        auto *w = m_tabs->widget(index);
        m_tabs->removeTab(index);
        delete w;
        saveAccountCount();
        saveTabOrder();
    });

    connect(m_tabs->tabBar(), &QTabBar::tabMoved, this, [this]() {
        saveTabOrder();
    });

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Accounts"));
    int count = config.readEntry("count", 1);
    for (int i = 0; i < count; ++i)
        addAccount();
    restoreTabOrder();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::saveAccountCount);

    setupActions();
    setupTrayIcon();
    setupGUI(ToolBar | Keys | Save | Create, QStringLiteral("kwhatstuxui.rc"));
    if (!menuBar()->isVisible())
        setProperty("_breeze_no_separator", true);
    resize(1024, 768);
}

MainWindow::~MainWindow() = default;

QWebEngineView *MainWindow::addAccount()
{
    m_accountCounter++;
    const auto profileName = QStringLiteral("kwhatstux-account-%1").arg(m_accountCounter);

    auto *profile = new QWebEngineProfile(profileName, this);
    profile->setHttpUserAgent(userAgent);
    profile->setNotificationPresenter([this](std::unique_ptr<QWebEngineNotification> n) {
        handleNotification(std::move(n));
    });

    auto *webView = new QWebEngineView(profile, this);
    webView->setProperty("accountNumber", m_accountCounter);

    QWebEngineScript script;
    script.setSourceCode(QStringLiteral(
        "(function() {"
        "  var style = document.createElement('style');"
        "  style.textContent = 'html, body { margin: 0 !important; padding: 0 !important; overflow: hidden; }';"
        "  document.documentElement.appendChild(style);"
        "})();"
    ));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    script.setRunsOnSubFrames(false);
    webView->page()->scripts().insert(script);

    connect(webView->page(), &QWebEnginePage::permissionRequested,
            this, &MainWindow::handlePermission);

    connect(webView, &QWebEngineView::titleChanged, this, [this, webView](const QString &title) {
        int idx = m_tabs->indexOf(webView);
        if (idx >= 0)
            updateTabTitle(idx, title);
    });

    int idx = m_tabs->addTab(webView, i18n("Account %1", m_accountCounter));
    m_tabs->setCurrentIndex(idx);

    webView->load(QUrl(QStringLiteral("https://web.whatsapp.com")));
    saveAccountCount();
    return webView;
}

void MainWindow::setupActions()
{
    KStandardAction::quit(qApp, &QCoreApplication::quit, actionCollection());
    KStandardAction::showMenubar(this, [this](bool visible) {
        menuBar()->setVisible(visible);
        menuBar()->setMaximumHeight(visible ? QWIDGETSIZE_MAX : 0);
        setProperty("_breeze_no_separator", !visible);
    }, actionCollection());

    auto *reload = new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")),
                               i18n("&Reload"), this);
    actionCollection()->addAction(QStringLiteral("reload"), reload);
    actionCollection()->setDefaultShortcut(reload, Qt::Key_F5);
    connect(reload, &QAction::triggered, this, [this]() {
        if (auto *view = qobject_cast<QWebEngineView *>(m_tabs->currentWidget()))
            view->reload();
    });

    auto *newAccount = new QAction(QIcon::fromTheme(QStringLiteral("list-add")),
                                   i18n("&New Account"), this);
    actionCollection()->addAction(QStringLiteral("new_account"), newAccount);
    actionCollection()->setDefaultShortcut(newAccount, QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(newAccount, &QAction::triggered, this, &MainWindow::addAccount);
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new KStatusNotifierItem(this);
    m_trayIcon->setCategory(KStatusNotifierItem::Communications);
    if (QIcon::hasThemeIcon(QStringLiteral("kwhatstux-tray")))
        m_trayIcon->setIconByName(QStringLiteral("kwhatstux-tray"));
    else
        m_trayIcon->setIconByName(QStringLiteral("kwhatstux"));
    m_trayIcon->setStandardActionsEnabled(true);
    m_trayIcon->setToolTipTitle(i18n("KWhatsTux"));
    m_trayIcon->setToolTipSubTitle(i18n("WhatsApp Web Client"));

    connect(m_trayIcon, &KStatusNotifierItem::activateRequested, this, [this]() {
        if (isVisible() && !isMinimized()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    });
}

void MainWindow::handlePermission(QWebEnginePermission permission)
{
    const auto origin = permission.origin();
    if (origin.host() == QStringLiteral("web.whatsapp.com")) {
        permission.grant();
        return;
    }
    permission.deny();
}

void MainWindow::handleNotification(std::unique_ptr<QWebEngineNotification> webNotification)
{
    auto *knotify = new KNotification(QStringLiteral("webNotification"),
                                       KNotification::CloseOnTimeout, this);
    knotify->setTitle(webNotification->title());
    knotify->setText(webNotification->message());

    const auto icon = webNotification->icon();
    if (!icon.isNull())
        knotify->setPixmap(QPixmap::fromImage(icon));

    auto *raw = webNotification.release();

    auto *defaultAction = knotify->addDefaultAction(i18n("Open"));
    connect(defaultAction, &KNotificationAction::activated, this, [this, raw]() {
        raw->click();
        show();
        raise();
        activateWindow();
    });
    connect(knotify, &KNotification::closed, raw, &QWebEngineNotification::close);
    connect(raw, &QWebEngineNotification::closed, knotify, &KNotification::close);

    knotify->sendEvent();
}

void MainWindow::updateTabTitle(int index, const QString &title)
{
    auto *widget = m_tabs->widget(index);
    int num = widget->property("accountNumber").toInt();

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Accounts"));
    QString customName = config.readEntry(QStringLiteral("name-%1").arg(num), QString());
    QString label = customName.isEmpty() ? i18n("Account %1", num) : customName;

    if (title.startsWith(QLatin1Char('('))) {
        int close = title.indexOf(QLatin1Char(')'));
        if (close > 0)
            label = title.left(close + 1) + QLatin1Char(' ') + label;
    }

    m_tabs->setTabText(index, label);
}

void MainWindow::renameTab(int index)
{
    auto *widget = m_tabs->widget(index);
    int num = widget->property("accountNumber").toInt();

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Accounts"));
    QString current = config.readEntry(QStringLiteral("name-%1").arg(num), QString());

    bool ok;
    QString name = QInputDialog::getText(this, i18n("Rename Account"),
                                         i18n("Account name:"),
                                         QLineEdit::Normal, current, &ok);
    if (!ok)
        return;

    config.writeEntry(QStringLiteral("name-%1").arg(num), name);
    config.sync();

    auto *webView = qobject_cast<QWebEngineView *>(widget);
    updateTabTitle(index, webView ? webView->title() : QString());
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_tabs->tabBar() && event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent *>(event);
        int index = m_tabs->tabBar()->tabAt(me->pos());
        if (index >= 0) {
            renameTab(index);
            return true;
        }
    }
    return KXmlGuiWindow::eventFilter(obj, event);
}

void MainWindow::saveAccountCount()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Accounts"));
    config.writeEntry("count", m_tabs->count());
    config.sync();
}

void MainWindow::saveTabOrder()
{
    QList<int> order;
    for (int i = 0; i < m_tabs->count(); ++i)
        order.append(m_tabs->widget(i)->property("accountNumber").toInt());
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Accounts"));
    config.writeEntry("tabOrder", order);
    config.sync();
}

void MainWindow::restoreTabOrder()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Accounts"));
    QList<int> order = config.readEntry("tabOrder", QList<int>());
    if (order.isEmpty())
        return;

    for (int target = 0; target < order.size() && target < m_tabs->count(); ++target) {
        for (int j = target; j < m_tabs->count(); ++j) {
            if (m_tabs->widget(j)->property("accountNumber").toInt() == order[target]) {
                if (j != target)
                    m_tabs->tabBar()->moveTab(j, target);
                break;
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon) {
        hide();
        event->ignore();
        return;
    }
    KXmlGuiWindow::closeEvent(event);
}
