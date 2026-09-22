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

#include "core/SnapshotParser.h"

#include "infra/Logger.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace ccm::core {
namespace {

// ---------------------------------------------------------------------------
// 스키마 상수. 문자열 하드코딩을 한곳에 모아 스키마 변경 대응 비용을 낮춘다.
// ---------------------------------------------------------------------------
constexpr int kSchemaVersion = 1;

constexpr auto kKeySchemaVersion = "schemaVersion";
constexpr auto kKeyCapturedAt    = "capturedAt";
constexpr auto kKeyPayload       = "payload";

constexpr auto kKeyRateLimits    = "rate_limits";
constexpr auto kKeyFiveHour      = "five_hour";
constexpr auto kKeySevenDay      = "seven_day";
constexpr auto kKeyUsedPercent   = "used_percentage";

constexpr double kPercentMin = 0.0;
constexpr double kPercentMax = 100.0;

double clampPercent(double value)
{
    if (value < kPercentMin) {
        return kPercentMin;
    }
    if (value > kPercentMax) {
        return kPercentMax;
    }
    return value;
}

/// capturedAt 은 SnapshotStore 가 Qt::ISODateWithMs 로 기록한 값이다.
/// 그 외 형식은 받지 않는다.
QDateTime parseCapturedAt(const QJsonValue &value)
{
    if (!value.isString()) {
        return {};
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return {};
    }

    QDateTime parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(text, Qt::ISODate);
    }
    return parsed.isValid() ? parsed.toUTC() : QDateTime();
}

RateLimitWindow parseWindow(const QJsonObject &parent, const char *key)
{
    RateLimitWindow window;

    const QJsonValue node = parent.value(QLatin1String(key));
    if (!node.isObject()) {
        return window;
    }

    const QJsonValue percent = node.toObject().value(QLatin1String(kKeyUsedPercent));
    if (!percent.isDouble()) {
        qCDebug(ccmCore) << "한도 창에 사용률 필드가 없습니다:" << key;
        return window;
    }

    window.present = true;
    window.usedPercent = clampPercent(percent.toDouble());
    return window;
}

} // namespace

int SnapshotParser::schemaVersion()
{
    return kSchemaVersion;
}

ParseResult SnapshotParser::parse(const QByteArray &json, const QDateTime &fallbackCapturedAt)
{
    ParseResult result;

    if (json.trimmed().isEmpty()) {
        result.error = QCoreApplication::translate("ccm", "입력이 비어 있습니다.");
        return result;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.error = QCoreApplication::translate("ccm", "JSON 파싱 실패: %1 (offset %2)")
                           .arg(parseError.errorString())
                           .arg(parseError.offset);
        return result;
    }
    if (!document.isObject()) {
        result.error = QCoreApplication::translate("ccm", "최상위가 JSON 객체가 아닙니다.");
        return result;
    }

    const QJsonObject root = document.object();

    QJsonObject payload = root;
    UsageSnapshot snapshot;

    const QJsonValue payloadNode = root.value(QLatin1String(kKeyPayload));
    if (payloadNode.isObject()) {
        // 래퍼 형태
        payload = payloadNode.toObject();
        snapshot.capturedAt = parseCapturedAt(root.value(QLatin1String(kKeyCapturedAt)));

        const int version =
            static_cast<int>(root.value(QLatin1String(kKeySchemaVersion)).toDouble());
        if (version != 0 && version != kSchemaVersion) {
            qCWarning(ccmCore) << "스냅샷 스키마 버전이 다릅니다. expected"
                               << kSchemaVersion << "actual" << version;
        }
    }

    if (!snapshot.capturedAt.isValid()) {
        snapshot.capturedAt = fallbackCapturedAt.isValid()
                                  ? fallbackCapturedAt.toUTC()
                                  : QDateTime::currentDateTimeUtc();
        qCDebug(ccmCore) << "capturedAt 을 찾지 못해 대체값을 사용합니다:"
                         << snapshot.capturedAt.toString(Qt::ISODateWithMs);
    }

    const QJsonValue limitsNode = payload.value(QLatin1String(kKeyRateLimits));
    if (limitsNode.isObject()) {
        const QJsonObject limits = limitsNode.toObject();
        snapshot.fiveHour = parseWindow(limits, kKeyFiveHour);
        snapshot.sevenDay = parseWindow(limits, kKeySevenDay);
    } else {
        // Premium 이 아닌 시트나 특정 인증 조합에서 필드 자체가 없을 수 있다.
        qCWarning(ccmCore) << "스냅샷에 rate_limits 필드가 없습니다. 한도 표시를 비활성화합니다.";
    }

    snapshot.valid = true;
    result.ok = true;
    result.snapshot = snapshot;

    qCDebug(ccmCore) << "사용률 파싱 완료."
                     << "fiveHour.present=" << snapshot.fiveHour.present
                     << "sevenDay.present=" << snapshot.sevenDay.present;
    return result;
}

} // namespace ccm::core
