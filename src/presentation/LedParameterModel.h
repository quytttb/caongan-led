#pragma once

#include "domain/Types.h"

#include <QAbstractListModel>

namespace caongan {

class LedParameterModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ChannelIdRole = Qt::UserRole + 1,
        SymbolRole,
        DisplayNameRole,
        UnitRole,
        GroupRole,
        GroupTitleRole,
        ValueTextRole,
        StatusTextRole,
        StateKeyRole,
        SeverityRole,
        DetailTextRole,
        TimeTextRole,
        ThresholdTextRole,
        SourceNameRole,
        HasReadingRole,
        AgeTextRole,
        RangeTextRole,
    };
    Q_ENUM(Roles)

    explicit LedParameterModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setConfig(const AppConfig &config);
    void setStates(const QVector<ChannelState> &states);
    QVector<ChannelState> states() const { return m_states; }

signals:
    void statesChanged();

private:
    void rebuildPlaceholders();

    AppConfig m_config;
    QVector<ChannelState> m_states;
};

} // namespace caongan
