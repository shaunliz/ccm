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

#include "core/UsageReportParser.h"

#include "infra/Logger.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>

namespace ccm::core {
namespace {

// ---------------------------------------------------------------------------
// 스키마 상수. 문자열 하드코딩을 한곳에 모아 스키마 변경 대응 비용을 낮춘다.
// ---------------------------------------------------------------------------
constexpr auto kKeyType        = "type";
constexpr auto kKeyResponse    = "response";
constexpr auto kKeySubtype     = "subtype";
constexpr auto kKeyError       = "error";

constexpr auto kTypeControlResponse = "control_response";
constexpr auto kSubtypeSuccess      = "success";

constexpr auto kKeyRateLimits        = "rate_limits";
constexpr auto kKeyRateLimitsPresent = "rate_limits_available";
constexpr auto kKeySubscriptionType  = "subscription_type";

constexpr auto kKeyUtilization  = "utilization";
constexpr auto kKeyResetsAt     = "resets_at";
constexpr auto kKeyDisplayName  = "display_name";

constexpr auto kKeyFiveHour     = "five_hour";
constexpr auto kKeySevenDay     = "seven_day";
constexpr auto kKeySevenDaySonnet = "seven_day_sonnet";
constexpr auto kKeySevenDayOpus   = "seven_day_opus";
constexpr auto kKeyModelScoped    = "model_scoped";

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

/// resets_at 은 ISO 8601 이지만 초 아래 자릿수가 6자리다.
/// ("2026-09-15T13:59:59.815513+00:00")
/// Qt 의 ISO 파서는 밀리초까지만 확실하게 받으므로 소수부를 떼고 넘긴다.
///
/// 다만 소수부를 그냥 버리면 안 된다. 서버는 같은 재설정 경계를
/// 13:59:59.815513 로도 14:00:00.041 로도 준다. 버리면 앞의 것만 13:59:59 가
/// 되어 두 값이 1초 어긋나고, 그 어긋남이 알림 중복 판정을 흔들어 사용률이
/// 그대로인데도 같은 알림을 반복시킨다. 그래서 버리지 않고 반올림한다.
/// (표식 쪽은 resetToken() 이 분 단위로 한 번 더 묶어 마지막 방어선을 만든다.)
QDateTime parseResetsAt(const QJsonValue &value)
{
    if (!value.isString()) {
        return {};
    }

    const QString raw = value.toString().trimmed();
    if (raw.isEmpty()) {
        return {};
    }

    QString text = raw;
    static const QRegularExpression fraction(QStringLiteral("\\.(\\d+)"));
    const QRegularExpressionMatch match = fraction.match(text);

    bool roundUp = false;
    if (match.hasMatch()) {
        // "0.815513" 꼴로 만들어 견준다. 자릿수가 몇 개든 상관없다.
        roundUp = QStringLiteral("0.%1").arg(match.captured(1)).toDouble() >= 0.5;
        text.remove(match.capturedStart(), match.capturedLength());
    }

    QDateTime parsed = QDateTime::fromString(text, Qt::ISODate);
    if (!parsed.isValid()) {
        return {};
    }
    if (roundUp) {
        parsed = parsed.addSecs(1);
    }

    const QDateTime utc = parsed.toUTC();

    // 서버 값이 실제로 어디까지 흔들리는지는 원본이 없으면 확인할 길이 없다.
    // 같은 의심이 다시 들 때 로그 한 줄로 끝내려고 남긴다.
    qCDebug(ccmCore).noquote() << QStringLiteral("resets_at 원본=%1 -> %2")
                                      .arg(raw, utc.toString(Qt::ISODate));

    return utc;
}

/// 이름이 정해진 창 하나를 목록에 넣는다.
/// 필드가 없거나 null 이거나 utilization 이 null 이면 그 창은 존재하지 않는다.
void appendNamedWindow(QList<UsageWindow> *windows,
                       const QJsonObject &rateLimits,
                       const char *key,
                       const QString &label,
                       UsageWindowKind kind)
{
    const QJsonValue node = rateLimits.value(QLatin1String(key));
    if (!node.isObject()) {
        return;
    }

    const QJsonObject window = node.toObject();
    const QJsonValue utilization = window.value(QLatin1String(kKeyUtilization));
    if (!utilization.isDouble()) {
        qCDebug(ccmCore) << "utilization 이 없어 창을 건너뜁니다:" << key;
        return;
    }

    UsageWindow parsed;
    parsed.key = QString::fromLatin1(key);
    parsed.label = label;
    parsed.kind = kind;
    parsed.usedPercent = clampPercent(utilization.toDouble());
    parsed.resetsAt = parseResetsAt(window.value(QLatin1String(kKeyResetsAt)));
    windows->append(parsed);
}

/// 모델별 주간 창. 개수와 이름 모두 서버가 정한다.
void appendModelScopedWindows(QList<UsageWindow> *windows, const QJsonObject &rateLimits)
{
    const QJsonValue node = rateLimits.value(QLatin1String(kKeyModelScoped));
    if (!node.isArray()) {
        return;
    }

    const QJsonArray entries = node.toArray();
    for (const QJsonValue &entry : entries) {
        if (!entry.isObject()) {
            continue;
        }
        const QJsonObject window = entry.toObject();

        const QJsonValue utilization = window.value(QLatin1String(kKeyUtilization));
        const QString name = window.value(QLatin1String(kKeyDisplayName)).toString().trimmed();
        if (!utilization.isDouble() || name.isEmpty()) {
            qCDebug(ccmCore) << "모델별 창의 필드가 비어 건너뜁니다:" << name;
            continue;
        }

        UsageWindow parsed;
        // 모델 이름은 서버 값이므로 키에 그대로 쓴다. 접두어로 고정 이름 창과 구분한다.
        parsed.key = QStringLiteral("model:%1").arg(name);
        // 서버는 모델 이름만 준다. model_scoped 는 모두 주간 창이므로 여기서 붙인다.
        parsed.label = QCoreApplication::translate("ccm", "%1 (주간)").arg(name);
        parsed.kind = UsageWindowKind::WeeklyModel;
        parsed.usedPercent = clampPercent(utilization.toDouble());
        parsed.resetsAt = parseResetsAt(window.value(QLatin1String(kKeyResetsAt)));
        windows->append(parsed);
    }
}

} // namespace

UsageReportParseResult UsageReportParser::parse(const QByteArray &controlResponseLine)
{
    UsageReportParseResult result;

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(controlResponseLine, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.error = QCoreApplication::translate("ccm", "응답 JSON 을 해석하지 못했습니다: %1")
                           .arg(parseError.errorString());
        qCCritical(ccmCore).noquote() << result.error;
        return result;
    }

    const QJsonObject envelope = document.object();
    if (envelope.value(QLatin1String(kKeyType)).toString() != QLatin1String(kTypeControlResponse)) {
        result.error = QCoreApplication::translate("ccm", "control_response 가 아닙니다: %1")
                           .arg(envelope.value(QLatin1String(kKeyType)).toString());
        qCCritical(ccmCore).noquote() << result.error;
        return result;
    }

    const QJsonObject outer = envelope.value(QLatin1String(kKeyResponse)).toObject();
    const QString subtype = outer.value(QLatin1String(kKeySubtype)).toString();
    if (subtype != QLatin1String(kSubtypeSuccess)) {
        const QString detail = outer.value(QLatin1String(kKeyError)).toString();
        result.error = detail.isEmpty()
                           ? QCoreApplication::translate("ccm", "claude 가 요청을 거부했습니다. (subtype=%1)").arg(subtype)
                           : QCoreApplication::translate("ccm", "claude 가 요청을 거부했습니다: %1").arg(detail);
        qCCritical(ccmCore).noquote() << result.error;
        return result;
    }

    const QJsonObject usage = outer.value(QLatin1String(kKeyResponse)).toObject();

    UsageReport report;
    report.fetchedAt = QDateTime::currentDateTimeUtc();
    report.rateLimitsAvailable = usage.value(QLatin1String(kKeyRateLimitsPresent)).toBool();
    report.subscriptionType = usage.value(QLatin1String(kKeySubscriptionType)).toString();

    const QJsonValue rateLimitsNode = usage.value(QLatin1String(kKeyRateLimits));
    if (!rateLimitsNode.isObject()) {
        // 한도가 적용되지 않는 계정이면 rate_limits 가 null 로 온다. 오류가 아니다.
        qCWarning(ccmCore) << "rate_limits 가 없습니다. rate_limits_available="
                           << report.rateLimitsAvailable;
        report.valid = true;
        result.report = report;
        result.ok = true;
        return result;
    }

    const QJsonObject rateLimits = rateLimitsNode.toObject();

    // 순서는 Claude 앱의 사용량 화면과 같게 둔다.
    appendNamedWindow(&report.windows, rateLimits, kKeyFiveHour,
                      QCoreApplication::translate("ccm", "현재 세션"), UsageWindowKind::Session);
    appendNamedWindow(&report.windows, rateLimits, kKeySevenDay,
                      QCoreApplication::translate("ccm", "모든 모델 (주간)"), UsageWindowKind::WeeklyAll);
    appendNamedWindow(&report.windows, rateLimits, kKeySevenDaySonnet,
                      QCoreApplication::translate("ccm", "Sonnet 전용 (주간)"), UsageWindowKind::WeeklyModel);
    appendNamedWindow(&report.windows, rateLimits, kKeySevenDayOpus,
                      QCoreApplication::translate("ccm", "Opus 전용 (주간)"), UsageWindowKind::WeeklyModel);
    appendModelScopedWindows(&report.windows, rateLimits);

    report.valid = true;
    result.report = report;
    result.ok = true;

    qCDebug(ccmCore) << "사용량 응답 파싱 완료. 창" << report.windows.size() << "개, 플랜"
                     << report.subscriptionType;
    return result;
}

} // namespace ccm::core
