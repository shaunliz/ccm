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

#include "infra/Settings.h"

#include "infra/ClaudePaths.h"
#include "infra/Logger.h"

#include <QSettings>
#include <QStringList>

namespace ccm::infra {
namespace {

constexpr auto kKeyPollInterval = "poll/intervalMinutes";
constexpr auto kKeyNotifications = "alert/notifications";
constexpr auto kKeyStartMinimized = "ui/startMinimized";
constexpr auto kKeyAskOnClose = "ui/askOnClose";
constexpr auto kKeyQuitOnClose = "ui/quitOnClose";
constexpr auto kKeyChartThresholds = "ui/chartThresholds";

// 한 줄만 고를 수 있던 시절의 키. 읽기만 한다. (아래 chartThresholds 참조)
constexpr auto kKeyChartWindowLegacy = "ui/chartThresholdWindow";
constexpr auto kKeyChartPercentLegacy = "ui/chartThresholdPercent";
constexpr auto kKeyFirstRunDone = "app/firstRunDone";

constexpr auto kGroupThreshold = "threshold";
constexpr auto kGroupFired = "alert/fired";

constexpr int kDefaultPollMinutes = 5;
constexpr int kMinPollMinutes = 1;
constexpr int kMaxPollMinutes = 720;
constexpr int kHistoryRetentionHours = 24 * 7;

/// 창 키는 서버가 준 모델 이름을 담는다. INI 에서 '/' 는 그룹 구분자이므로
/// 그대로 쓰면 키가 하위 그룹으로 쪼개진다. 저장할 때만 바꿔 둔다.
QString encodeKey(const QString &key)
{
    return QString(key).replace(QLatin1Char('/'), QLatin1String("%2F"));
}

QString decodeKey(const QString &key)
{
    return QString(key).replace(QLatin1String("%2F"), QLatin1String("/"));
}

/// 차트에 그릴 임계치 하나를 "70.0:five_hour" 한 조각으로.
///
/// 사용률을 앞에 두는 이유가 있다. 창 키에는 ':' 가 들어간다("model:Fable").
/// 키를 앞에 두면 어디서 잘라야 할지 알 수 없지만, 숫자를 앞에 두면 언제나
/// 첫 ':' 에서 자르면 된다.
QString chartThresholdToToken(const ChartThreshold &threshold)
{
    return QStringLiteral("%1:%2").arg(threshold.percent, 0, 'f', 1).arg(threshold.windowKey);
}

/// 위의 역. 알아볼 수 없으면 windowKey 가 빈 값으로 돌아온다.
ChartThreshold chartThresholdFromToken(const QString &token)
{
    const int separator = token.indexOf(QLatin1Char(':'));
    if (separator <= 0) {
        qCWarning(ccmInfra) << "차트 임계치 조각을 알아볼 수 없어 건너뜁니다:" << token;
        return ChartThreshold{};
    }

    bool numberOk = false;
    const double percent = token.left(separator).trimmed().toDouble(&numberOk);
    const QString key = token.mid(separator + 1).trimmed();
    if (!numberOk || percent < 0.0 || percent > 100.0 || key.isEmpty()) {
        qCWarning(ccmInfra) << "차트 임계치 조각을 알아볼 수 없어 건너뜁니다:" << token;
        return ChartThreshold{};
    }
    return ChartThreshold{key, percent};
}

/// 설정 파일 하나를 여러 곳에서 열고 닫는다. QSettings 는 값싸고 스스로 동기화한다.
QSettings open()
{
    return QSettings(ClaudePaths::settingsFilePath(), QSettings::IniFormat);
}

/// 꺼 둔 규칙에만 붙는 꼬리표. 켠 것에는 붙이지 않는다.
///
/// 대부분의 줄이 켜져 있으므로, 켠 쪽을 짧게 두면 설정 파일이 읽기 쉽다.
/// 꼬리표가 없는 줄은 켜진 것으로 읽는다. 예전 파일도 그래서 그대로 열린다.
constexpr auto kRuleDisabledToken = "off";

/// 규칙 목록을 {"70.0:warn", "85.0:warn:off", ...} 로.
/// QSettings 가 쉼표로 잇고 따옴표까지 붙여 준다.
QStringList rulesToList(const ccm::core::ThresholdRules &rules)
{
    QStringList parts;
    for (const ccm::core::ThresholdRule &rule : rules) {
        QString part = QStringLiteral("%1:%2")
                           .arg(rule.percent, 0, 'f', 1)
                           .arg(ccm::core::alertLevelToken(rule.level));
        if (!rule.enabled) {
            part += QLatin1Char(':') + QLatin1String(kRuleDisabledToken);
        }
        parts << part;
    }
    return parts;
}

/// 위의 역. 알아볼 수 없는 조각은 건너뛴다. 손으로 고친 파일이 프로그램을
/// 멈추게 하지 않도록 하기 위함이다.
ccm::core::ThresholdRules rulesFromList(const QStringList &parts)
{
    ccm::core::ThresholdRules rules;

    for (const QString &part : parts) {
        // 세 번째 칸은 나중에 생겼다. 없으면 켜진 것으로 읽는다.
        const QStringList fields = part.split(QLatin1Char(':'));
        if (fields.size() != 2 && fields.size() != 3) {
            qCWarning(ccmInfra) << "임계치 조각을 알아볼 수 없어 건너뜁니다:" << part;
            continue;
        }

        bool numberOk = false;
        const double percent = fields.at(0).trimmed().toDouble(&numberOk);
        ccm::core::AlertLevel level = ccm::core::AlertLevel::Warning;
        if (!numberOk || percent < 0.0 || percent > 100.0
            || !ccm::core::alertLevelFromToken(fields.at(1), &level)) {
            qCWarning(ccmInfra) << "임계치 조각을 알아볼 수 없어 건너뜁니다:" << part;
            continue;
        }

        const bool enabled = fields.size() < 3
                             || fields.at(2).trimmed().compare(
                                    QLatin1String(kRuleDisabledToken), Qt::CaseInsensitive) != 0;
        rules.append(ccm::core::ThresholdRule{percent, level, enabled});
    }

    ccm::core::sortThresholdRules(&rules);
    return rules;
}

} // namespace

int Settings::defaultPollIntervalMinutes()
{
    return kDefaultPollMinutes;
}

int Settings::pollIntervalMinutes()
{
    QSettings settings = open();
    const int stored =
        settings.value(QLatin1String(kKeyPollInterval), kDefaultPollMinutes).toInt();
    if (stored == 0) {
        return 0;   // 자동 조회 끔.
    }
    return qBound(kMinPollMinutes, stored, kMaxPollMinutes);
}

void Settings::setPollIntervalMinutes(int minutes)
{
    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyPollInterval),
                      minutes <= 0 ? 0 : qBound(kMinPollMinutes, minutes, kMaxPollMinutes));
}

ccm::core::ThresholdConfig Settings::thresholds()
{
    QSettings settings = open();
    settings.beginGroup(QLatin1String(kGroupThreshold));

    ccm::core::ThresholdConfig config;
    const QStringList keys = settings.childKeys();
    for (const QString &key : keys) {
        // 값이 비어 있으면 "이 지표는 알리지 않음" 이다. 기본값으로 되돌아가지
        // 않도록 빈 목록도 그대로 설정해 둔다.
        config.setRules(decodeKey(key), rulesFromList(settings.value(key).toStringList()));
    }
    settings.endGroup();
    return config;
}

void Settings::setThresholds(const ccm::core::ThresholdConfig &config)
{
    QSettings settings = open();
    // 사라진 창의 규칙이 남지 않도록 그룹을 비우고 다시 쓴다.
    settings.remove(QLatin1String(kGroupThreshold));

    settings.beginGroup(QLatin1String(kGroupThreshold));
    const QList<QString> keys = config.configuredKeys();
    for (const QString &key : keys) {
        settings.setValue(encodeKey(key), rulesToList(config.configuredRules(key)));
    }
    settings.endGroup();
}

ChartThresholds Settings::chartThresholds()
{
    QSettings settings = open();

    if (settings.contains(QLatin1String(kKeyChartThresholds))) {
        // 값이 있으면 그것이 전부다. 빈 목록도 뜻이 있다. "하나도 그리지 않음".
        // 그래서 비어 있다고 아래의 옛 키로 내려가지 않는다.
        ChartThresholds result;
        const QStringList parts =
            settings.value(QLatin1String(kKeyChartThresholds)).toStringList();
        for (const QString &part : parts) {
            const ChartThreshold threshold = chartThresholdFromToken(part);
            if (threshold.windowKey.isEmpty()) {
                continue;
            }
            result.append(threshold);
            if (result.size() >= Settings::maxChartThresholds()) {
                break;
            }
        }
        return result;
    }

    // 예전 설정 파일이다. 그때는 라디오 버튼으로 한 줄만 골랐다. 옮겨 온다.
    // 쓰기는 새 키로만 하므로 이 자리는 한 번만 지난다.
    const QString legacyKey = settings.value(QLatin1String(kKeyChartWindowLegacy)).toString();
    const double legacyPercent =
        settings.value(QLatin1String(kKeyChartPercentLegacy), -1.0).toDouble();
    if (legacyKey.isEmpty() || legacyPercent < 0.0) {
        return ChartThresholds{};
    }
    return ChartThresholds{ChartThreshold{legacyKey, legacyPercent}};
}

bool Settings::chartThresholdsConfigured()
{
    QSettings settings = open();
    return settings.contains(QLatin1String(kKeyChartThresholds))
           || !settings.value(QLatin1String(kKeyChartWindowLegacy)).toString().isEmpty();
}

void Settings::setChartThresholds(const ChartThresholds &thresholds)
{
    QStringList parts;
    for (const ChartThreshold &threshold : thresholds) {
        if (threshold.windowKey.isEmpty() || threshold.percent < 0.0) {
            continue;
        }
        parts << chartThresholdToToken(threshold);
        if (parts.size() >= Settings::maxChartThresholds()) {
            break;
        }
    }

    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyChartThresholds), parts);

    // 옮겨 왔으니 옛 키는 지운다. 남겨 두면 다음에 열었을 때 무엇이 참인지
    // 파일만 보고는 알 수 없다.
    settings.remove(QLatin1String(kKeyChartWindowLegacy));
    settings.remove(QLatin1String(kKeyChartPercentLegacy));
}

bool Settings::notificationsEnabled()
{
    QSettings settings = open();
    return settings.value(QLatin1String(kKeyNotifications), true).toBool();
}

void Settings::setNotificationsEnabled(bool enabled)
{
    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyNotifications), enabled);
}

bool Settings::startMinimized()
{
    QSettings settings = open();
    return settings.value(QLatin1String(kKeyStartMinimized), false).toBool();
}

void Settings::setStartMinimized(bool enabled)
{
    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyStartMinimized), enabled);
}

bool Settings::askOnClose()
{
    QSettings settings = open();
    return settings.value(QLatin1String(kKeyAskOnClose), true).toBool();
}

void Settings::setAskOnClose(bool enabled)
{
    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyAskOnClose), enabled);
}

bool Settings::quitOnClose()
{
    QSettings settings = open();
    // 기본값은 트레이로 내려가기다. 트레이 상주가 이 프로그램의 평소 모습이다.
    return settings.value(QLatin1String(kKeyQuitOnClose), false).toBool();
}

void Settings::setQuitOnClose(bool enabled)
{
    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyQuitOnClose), enabled);
}

bool Settings::isFirstRun()
{
    QSettings settings = open();
    return !settings.value(QLatin1String(kKeyFirstRunDone), false).toBool();
}

void Settings::markFirstRunDone()
{
    QSettings settings = open();
    settings.setValue(QLatin1String(kKeyFirstRunDone), true);
}

ccm::core::FiredMarkMap Settings::firedMarks()
{
    QSettings settings = open();
    settings.beginGroup(QLatin1String(kGroupFired));

    ccm::core::FiredMarkMap marks;
    const QStringList keys = settings.childKeys();
    for (const QString &key : keys) {
        marks.insert(decodeKey(key), settings.value(key).toString());
    }
    settings.endGroup();
    return marks;
}

void Settings::setFiredMarks(const ccm::core::FiredMarkMap &marks)
{
    QSettings settings = open();
    // 사라진 창이나 지워진 규칙의 표식이 남지 않도록 그룹을 비우고 다시 쓴다.
    settings.remove(QLatin1String(kGroupFired));

    settings.beginGroup(QLatin1String(kGroupFired));
    for (auto it = marks.constBegin(); it != marks.constEnd(); ++it) {
        settings.setValue(encodeKey(it.key()), it.value());
    }
    settings.endGroup();
}

int Settings::historyRetentionHours()
{
    return kHistoryRetentionHours;
}

QString Settings::filePath()
{
    return ClaudePaths::settingsFilePath();
}

} // namespace ccm::infra
