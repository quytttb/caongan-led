#pragma once

#include "domain/Types.h"

#include <QAbstractListModel>

namespace caongan {

class DiagnosticsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TimeRole = Qt::UserRole + 1,
        SeverityRole,
        SourceRole,
        ChannelRole,
        CodeRole,
        MessageRole,
    };
    Q_ENUM(Roles)

    explicit DiagnosticsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    void appendBatch(const QVector<DiagnosticEntry> &entries);

private:
    QVector<DiagnosticEntry> m_entries; // mới nhất ở đầu
};

} // namespace caongan
