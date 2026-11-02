#pragma once

#include <QAbstractNativeEventFilter>
#include <QPointer>
#include <QtGui/qwindowdefs.h>

class QWindow;

// Chỉ áp dụng cho cửa sổ vận hành; LED và các hộp thoại native không bị đổi.
class WindowsFramelessHelper : public QAbstractNativeEventFilter
{
public:
    void setWindow(QWindow *window);
    bool hasNoTitleBar() const;
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    WId m_windowId = 0;
    QPointer<QWindow> m_window;
};
