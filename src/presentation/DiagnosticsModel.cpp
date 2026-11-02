#include "DiagnosticsModel.h"

#include "domain/Tt10Time.h"

namespace caongan {

DiagnosticsModel::DiagnosticsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DiagnosticsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DiagnosticsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const DiagnosticEntry &entry = m_entries.at(index.row());
    switch (role) {
    case TimeRole:
        return tt10::formatLocalDisplay(entry.atUtc, QStringLiteral("HH:mm:ss dd/MM"));
    case SeverityRole: return entry.severity;
    case SourceRole:   return entry.sourceId;
    case ChannelRole:  return entry.channelId;
    case CodeRole:     return entry.code;
    case MessageRole:  return entry.message;
    default:           return {};
    }
}

QHash<int, QByteArray> DiagnosticsModel::roleNames() const
{
    return {
        {TimeRole, "timeText"},
        {SeverityRole, "severity"},
        {SourceRole, "sourceId"},
        {ChannelRole, "channelId"},
        {CodeRole, "code"},
        {MessageRole, "message"},
    };
}

void DiagnosticsModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void DiagnosticsModel::appendBatch(const QVector<DiagnosticEntry> &entries)
{
    if (entries.isEmpty())
        return;

    // Mới nhất lên đầu; giới hạn 300 dòng để bộ nhớ không tăng vô hạn.
    beginResetModel();
    for (const DiagnosticEntry &entry : entries)
        m_entries.prepend(entry);
    if (m_entries.size() > 300)
        m_entries = m_entries.mid(0, 300);
    endResetModel();
}

} // namespace caongan
