// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-FileCopyrightText: 2026 Innogrid Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-only

#include "infra/AlertStore.h"

#include "infra/ClaudePaths.h"
#include "infra/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

namespace ccm::infra {
namespace {

constexpr int kSchemaVersion = 3;
constexpr int kCapacity = 500;

/// 알림 수준이 셋으로 늘기 전 형식. 그때는 경고=1, 위험=2 였다.
constexpr int kSchemaVersionBeforeNotice = 2;

constexpr auto kKeySchemaVersion = "schemaVersion";
constexpr auto kKeyAlerts = "alerts";

constexpr auto kKeyId = "id";
constexpr auto kKeyAt = "at";
constexpr auto kKeyWindowKey = "windowKey";
constexpr auto kKeyWindowLabel = "windowLabel";
constexpr auto kKeyKind = "kind";
constexpr auto kKeyLevel = "level";
constexpr auto kKeyUsedPercent = "usedPercent";
constexpr auto kKeyThresholdPercent = "thresholdPercent";
constexpr auto kKeyAcknowledged = "acknowledged";

ccm::core::AlertLevel intToLevel(int value)
{
    if (value < static_cast<int>(ccm::core::AlertLevel::Normal)
        || value > static_cast<int>(ccm::core::AlertLevel::Critical)) {
        return ccm::core::AlertLevel::Warning;
    }
    return static_cast<ccm::core::AlertLevel>(value);
}

/// 버전 2 파일의 수준 값을 지금 값으로 옮긴다. (경고 1 -> 2, 위험 2 -> 3)
ccm::core::AlertLevel migrateLevelFromV2(int value)
{
    switch (value) {
    case 1:
        return ccm::core::AlertLevel::Warning;
    case 2:
        return ccm::core::AlertLevel::Critical;
    default:
        return ccm::core::AlertLevel::Warning;
    }
}

ccm::core::UsageWindowKind intToKind(int value)
{
    if (value < static_cast<int>(ccm::core::UsageWindowKind::Session)
        || value > static_cast<int>(ccm::core::UsageWindowKind::WeeklyModel)) {
        return ccm::core::UsageWindowKind::WeeklyModel;
    }
    return static_cast<ccm::core::UsageWindowKind>(value);
}

QJsonObject toJson(const ccm::core::AlertEvent &event)
{
    QJsonObject object;
    object.insert(QLatin1String(kKeyId), event.id);
    object.insert(QLatin1String(kKeyAt), event.at.toUTC().toString(Qt::ISODateWithMs));
    object.insert(QLatin1String(kKeyWindowKey), event.windowKey);
    object.insert(QLatin1String(kKeyWindowLabel), event.windowLabel);
    object.insert(QLatin1String(kKeyKind), static_cast<int>(event.kind));
    object.insert(QLatin1String(kKeyLevel), static_cast<int>(event.level));
    object.insert(QLatin1String(kKeyUsedPercent), event.usedPercent);
    object.insert(QLatin1String(kKeyThresholdPercent), event.thresholdPercent);
    object.insert(QLatin1String(kKeyAcknowledged), event.acknowledged);
    return object;
}

bool fromJson(const QJsonObject &object, ccm::core::AlertEvent *out, int schemaVersion)
{
    const QDateTime at = QDateTime::fromString(object.value(QLatin1String(kKeyAt)).toString(),
                                               Qt::ISODateWithMs);
    if (!at.isValid()) {
        return false;
    }

    out->at = at.toUTC();
    out->id = object.value(QLatin1String(kKeyId)).toString();
    if (out->id.isEmpty()) {
        // 예전 형식에는 식별자가 없다. 읽을 때 붙여 준다. 없으면 하나씩 지울 수
        // 없기 때문이다.
        out->id = ccm::core::AlertEvent::makeId();
    }
    out->windowKey = object.value(QLatin1String(kKeyWindowKey)).toString();
    out->windowLabel = object.value(QLatin1String(kKeyWindowLabel)).toString();
    out->kind = intToKind(object.value(QLatin1String(kKeyKind)).toInt());
    const int storedLevel = object.value(QLatin1String(kKeyLevel)).toInt();
    out->level = schemaVersion <= kSchemaVersionBeforeNotice
                     ? migrateLevelFromV2(storedLevel)
                     : intToLevel(storedLevel);
    out->usedPercent = object.value(QLatin1String(kKeyUsedPercent)).toDouble();
    out->thresholdPercent = object.value(QLatin1String(kKeyThresholdPercent)).toDouble();
    out->acknowledged = object.value(QLatin1String(kKeyAcknowledged)).toBool();
    return true;
}

} // namespace

AlertStore::AlertStore(const QString &filePath)
    : m_filePath(filePath.isEmpty() ? ClaudePaths::alertFilePath() : filePath)
{
}

int AlertStore::capacity()
{
    return kCapacity;
}

void AlertStore::load()
{
    m_alerts.clear();

    if (!QFileInfo(m_filePath).isFile()) {
        qCDebug(ccmInfra) << "알림 목록이 없습니다. 빈 상태로 시작합니다:" << m_filePath;
        return;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(ccmInfra) << "알림 목록을 열지 못했습니다:" << file.errorString();
        return;
    }
    const QByteArray content = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(content, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qCWarning(ccmInfra).noquote()
            << QStringLiteral("알림 목록을 해석하지 못해 버립니다: %1")
                   .arg(parseError.errorString());
        return;
    }

    const QJsonObject root = document.object();
    const int version = root.value(QLatin1String(kKeySchemaVersion)).toInt();
    if (version > kSchemaVersion) {
        // 앞선 버전이 쓴 파일이면 손대지 않는다. 덮어써서 잃는 편이 더 나쁘다.
        qCWarning(ccmInfra) << "알림 목록이 더 새로운 형식입니다. 읽지 않습니다:" << version;
        return;
    }

    const QJsonArray alerts = root.value(QLatin1String(kKeyAlerts)).toArray();
    for (const QJsonValue &entry : alerts) {
        if (!entry.isObject()) {
            continue;
        }
        ccm::core::AlertEvent event;
        if (fromJson(entry.toObject(), &event, version)) {
            m_alerts.append(event);
        }
    }

    if (version < kSchemaVersion && !m_alerts.isEmpty()) {
        // 옮긴 결과를 바로 기록해 둔다. 그러지 않으면 매번 옮기게 된다.
        QString error;
        if (!save(&error)) {
            qCWarning(ccmInfra).noquote() << error;
        } else {
            qCInfo(ccmInfra) << "알림 목록을 새 형식으로 옮겼습니다. 이전 버전:" << version;
        }
    }

    sortNewestFirst();

    qCDebug(ccmInfra) << "알림 목록을 읽었습니다." << m_alerts.size() << "건, 미확인"
                      << unacknowledgedCount() << "건";
}

int AlertStore::unacknowledgedCount() const
{
    int count = 0;
    for (const ccm::core::AlertEvent &event : m_alerts) {
        if (!event.acknowledged) {
            ++count;
        }
    }
    return count;
}

bool AlertStore::append(const QList<ccm::core::AlertEvent> &events, QString *error)
{
    if (events.isEmpty()) {
        return true;
    }

    m_alerts.append(events);
    // 한 묶음 안의 순서와 파일에 적힌 순서를 믿지 않고 매번 다시 정렬한다.
    // 앞에 끼우는 방식으로 두었더니 한 묶음 안에서 순서가 뒤집혀, 화면에
    // 오래된 것이 위로 올라왔다.
    sortNewestFirst();
    trim();
    return save(error);
}

bool AlertStore::acknowledge(const QString &id, QString *error)
{
    for (ccm::core::AlertEvent &event : m_alerts) {
        if (event.id != id) {
            continue;
        }
        if (event.acknowledged) {
            return true;   // 이미 확인됨. 파일을 다시 쓸 이유가 없다.
        }
        event.acknowledged = true;
        return save(error);
    }
    return false;
}

bool AlertStore::acknowledgeAll(QString *error)
{
    bool changed = false;
    for (ccm::core::AlertEvent &event : m_alerts) {
        if (!event.acknowledged) {
            event.acknowledged = true;
            changed = true;
        }
    }
    if (!changed) {
        return true;
    }
    return save(error);
}

bool AlertStore::remove(const QString &id, QString *error)
{
    for (int i = 0; i < m_alerts.size(); ++i) {
        if (m_alerts.at(i).id == id) {
            m_alerts.removeAt(i);
            return save(error);
        }
    }
    return false;
}

int AlertStore::removeMany(const QList<QString> &ids, QString *error)
{
    if (ids.isEmpty()) {
        return 0;
    }

    const QSet<QString> targets(ids.constBegin(), ids.constEnd());
    const int before = m_alerts.size();
    m_alerts.removeIf([&targets](const ccm::core::AlertEvent &event) {
        return targets.contains(event.id);
    });

    const int removed = before - m_alerts.size();
    if (removed > 0 && !save(error)) {
        return removed;   // 지우기는 했으나 기록에 실패. 호출자가 error 를 본다.
    }
    return removed;
}

bool AlertStore::clear(QString *error)
{
    if (m_alerts.isEmpty()) {
        return true;
    }
    m_alerts.clear();
    return save(error);
}

void AlertStore::sortNewestFirst()
{
    // 시각이 같으면 넣은 순서를 지킨다. 한 번의 조회에서 나온 알림들은 시각이
    // 같으므로, 안정 정렬이어야 지표 순서(세션 -> 주간 -> 모델별)가 유지된다.
    std::stable_sort(m_alerts.begin(), m_alerts.end(),
                     [](const ccm::core::AlertEvent &lhs,
                        const ccm::core::AlertEvent &rhs) { return lhs.at > rhs.at; });
}

void AlertStore::trim()
{
    if (m_alerts.size() <= kCapacity) {
        return;
    }

    // 오래된 것부터 버리되 미확인 항목은 남긴다. 보지 못한 알림을 조용히
    // 버리면 알림의 뜻이 없다.
    for (int i = m_alerts.size() - 1; i >= 0 && m_alerts.size() > kCapacity; --i) {
        if (m_alerts.at(i).acknowledged) {
            m_alerts.removeAt(i);
        }
    }

    // 미확인만으로 상한을 넘겼다면 그때는 오래된 것부터 버린다. 파일이 무한정
    // 커지게 둘 수는 없다.
    while (m_alerts.size() > kCapacity) {
        qCWarning(ccmInfra) << "미확인 알림이 상한을 넘어 가장 오래된 것을 버립니다.";
        m_alerts.removeLast();
    }
}

bool AlertStore::save(QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error) {
            *error = message;
        }
        qCWarning(ccmInfra).noquote() << message;
        return false;
    };

    const QString directory = QFileInfo(m_filePath).absolutePath();
    if (!directory.isEmpty() && !QDir().mkpath(directory)) {
        return fail(QCoreApplication::translate("ccm", "알림 목록 디렉터리를 만들지 못했습니다: %1")
                        .arg(QDir::toNativeSeparators(directory)));
    }

    QJsonArray alerts;
    for (const ccm::core::AlertEvent &event : m_alerts) {
        alerts.append(toJson(event));
    }

    QJsonObject root;
    root.insert(QLatin1String(kKeySchemaVersion), kSchemaVersion);
    root.insert(QLatin1String(kKeyAlerts), alerts);

    QSaveFile out(m_filePath);
    if (!out.open(QIODevice::WriteOnly)) {
        return fail(QCoreApplication::translate("ccm", "알림 목록을 쓰기 모드로 열지 못했습니다: %1")
                        .arg(out.errorString()));
    }
    const QByteArray encoded = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (out.write(encoded) != encoded.size()) {
        out.cancelWriting();
        return fail(QCoreApplication::translate("ccm", "알림 목록 기록 중 오류가 발생했습니다: %1")
                        .arg(out.errorString()));
    }
    if (!out.commit()) {
        return fail(QCoreApplication::translate("ccm", "알림 목록 교체에 실패했습니다: %1").arg(out.errorString()));
    }
    return true;
}

} // namespace ccm::infra
