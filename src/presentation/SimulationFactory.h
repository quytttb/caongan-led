#pragma once

#include "domain/Types.h"

#include <QString>
#include <QVector>

namespace caongan {

QStringList simulationScenarioNames();
QVector<ChannelState> buildSimulationStates(const AppConfig &config,
                                            const QString &scenario,
                                            const QDateTime &nowUtc);

} // namespace caongan
