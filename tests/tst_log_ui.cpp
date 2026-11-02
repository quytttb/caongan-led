#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "utils/AppPaths.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QTemporaryDir>
#include <QTest>

Q_IMPORT_QML_PLUGIN(LoggerKit_ThemePlugin)

using namespace caongan;

class TestLogUi : public QObject
{
    Q_OBJECT
private slots:
    void displaysAndCopiesLog();
};

void TestLogUi::displaysAndCopiesLog()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    AppPaths::ensureDirectories();
    QFile source(QDir(AppPaths::logDir()).filePath(QStringLiteral("caongan-led-20260101.log")));
    QVERIFY(source.open(QIODevice::WriteOnly));
    const QByteArray expected = QByteArrayLiteral("[INFO] Đã nhập dữ liệu\n");
    QCOMPARE(source.write(expected), expected.size());
    source.close();

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    const QUrl qml = QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/LogPage.qml"));
    QQmlComponent component(&engine, qml);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> page(component.create());
    QVERIFY2(page, qPrintable(component.errorString()));

    QObject *viewer = page->findChild<QObject *>(QStringLiteral("logViewerText"));
    QObject *copy = page->findChild<QObject *>(QStringLiteral("copyLogButton"));
    QObject *exportButton = page->findChild<QObject *>(QStringLiteral("exportLogButton"));
    QObject *exportDialog = page->findChild<QObject *>(QStringLiteral("exportLogDialog"));
    QVERIFY(viewer);
    QVERIFY(copy);
    QVERIFY(exportButton);
    QVERIFY(exportDialog);
    QCOMPARE(viewer->property("text").toString(), QString::fromUtf8(expected));
    QVERIFY(copy->property("enabled").toBool());
    QVERIFY(exportButton->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(copy, "clicked"));
    QCOMPARE(QGuiApplication::clipboard()->text(), QString::fromUtf8(expected));

    QVERIFY(QMetaObject::invokeMethod(exportButton, "clicked"));
    const QString exportPath = temp.filePath(QStringLiteral("export.log"));
    QVERIFY(exportDialog->setProperty("selectedFile", QUrl::fromLocalFile(exportPath)));
    QVERIFY(QMetaObject::invokeMethod(exportDialog, "accepted"));
    QFile exported(exportPath);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    QCOMPARE(exported.readAll(), expected);
    QVERIFY(QMetaObject::invokeMethod(exportDialog, "close"));
}

QTEST_MAIN(TestLogUi)
#include "tst_log_ui.moc"
