add_executable(kwhatstux
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(kwhatstux PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    KF6::CoreAddons
    KF6::I18n
    KF6::XmlGui
    KF6::Crash
    KF6::DBusAddons
    KF6::Notifications
    KF6::StatusNotifierItem
)

install(TARGETS kwhatstux DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES kwhatstuxui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kwhatstux)
