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

#include "infra/UsageHistory.h"

#include "infra/ClaudePaths.h"
#include "infra/Logger.h"
#include "infra/Settings.h"

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
#include <QStringList>

#include <algorithm>

namespace ccm::infra {
namespace {

constexpr int kSchemaVersion = 1;

constexpr auto kKeySchemaVersion = "schemaVersion";
constexpr auto kKeySamples = "samples";
constexpr auto kKeyAt = "at";
constexpr auto kKeyValues = "values";

/// 표본 수 상한. 보관 기간과 별개로 파일이 무한정 커지는 것을 막는다.
/// 1분 간격으로 7일을 쌓아도 이 안에 들어온다.
constexpr int kMaxSamples = 20000;

} // namespace

UsageHistory::UsageHistory(const QString &filePath)
    : m_filePath(filePath.isEmpty() ? ClaudePaths::historyFilePath() : filePath)
{
}

void UsageHistory::load()
{
    m_samples.clear();

    QFile file(m_filePath);
    if (!QFileInfo(m_filePath).isFile()) {
        qCDebug(ccmInfra) << "이력 파일이 없습니다. 빈 상태로 시작합니다:" << m_filePath;
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(ccmInfra) << "이력 파일을 열지 못했습니다:" << file.errorString();
        return;
    }

    const QByteArray content = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(content, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        // 이력은 없어도 프로그램이 도는 데 지장이 없다. 버리고 새로 쌓는다.
        qCWarning(ccmInfra).noquote()
            << QStringLiteral("이력 파일을 해석하지 못해 버립니다: %1")
                   .arg(parseError.errorString());
        return;
    }

    const QJsonObject root = document.object();
    const int version = root.value(QLatin1String(kKeySchemaVersion)).toInt();
    if (version != kSchemaVersion) {
        qCWarning(ccmInfra) << "이력 스키마 버전이 달라 버립니다:" << version;
        return;
    }

    const QJsonArray samples = root.value(QLatin1String(kKeySamples)).toArray();
    for (const QJsonValue &entry : samples) {
        if (!entry.isObject()) {
            continue;
        }
        const QJsonObject object = entry.toObject();

        UsageSample sample;
        sample.at = QDateTime::fromString(object.value(QLatin1String(kKeyAt)).toString(),
                                          Qt::ISODateWithMs);
        if (!sample.at.isValid()) {
            continue;
        }
        sample.at = sample.at.toUTC();

        const QJsonObject values = object.value(QLatin1String(kKeyValues)).toObject();
        for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
            if (it.value().isDouble()) {
                sample.values.insert(it.key(), it.value().toDouble());
            }
        }
        if (!sample.values.isEmpty()) {
            m_samples.append(sample);
        }
    }

    std::sort(m_samples.begin(), m_samples.end(),
              [](const UsageSample &lhs, const UsageSample &rhs) { return lhs.at < rhs.at; });
    prune();

    qCDebug(ccmInfra) << "이력을 읽었습니다. 표본" << m_samples.size() << "개";
}

bool UsageHistory::append(const ccm::core::UsageReport &report, QString *error)
{
    if (report.windows.isEmpty()) {
        // 창이 없는 보고는 쌓을 것이 없다. 실패로 보지 않는다.
        return true;
    }

    UsageSample sample;
    sample.at = report.fetchedAt.isValid() ? report.fetchedAt.toUTC()
                                           : QDateTime::currentDateTimeUtc();
    for (const ccm::core::UsageWindow &window : report.windows) {
        sample.values.insert(window.key, window.usedPercent);
    }

    m_samples.append(sample);
    prune();
    return save(error);
}

QList<UsageSample> UsageHistory::recent(int hours) const
{
    if (hours <= 0) {
        return m_samples;
    }

    const QDateTime since = QDateTime::currentDateTimeUtc().addSecs(-qint64(hours) * 3600);
    QList<UsageSample> result;
    for (const UsageSample &sample : m_samples) {
        if (sample.at >= since) {
            result.append(sample);
        }
    }
    return result;
}

QStringList UsageHistory::keysIn(int hours) const
{
    // 등장 순서를 유지한다. 차트의 선 순서가 매번 달라지면 색이 흔들린다.
    QStringList ordered;
    QSet<QString> seen;
    const QList<UsageSample> window = recent(hours);
    for (const UsageSample &sample : window) {
        for (auto it = sample.values.constBegin(); it != sample.values.constEnd(); ++it) {
            if (!seen.contains(it.key())) {
                seen.insert(it.key());
                ordered.append(it.key());
            }
        }
    }
    ordered.sort();
    return ordered;
}

void UsageHistory::prune()
{
    const int hours = Settings::historyRetentionHours();
    const QDateTime since = QDateTime::currentDateTimeUtc().addSecs(-qint64(hours) * 3600);

    while (!m_samples.isEmpty() && m_samples.first().at < since) {
        m_samples.removeFirst();
    }
    while (m_samples.size() > kMaxSamples) {
        m_samples.removeFirst();
    }
}

bool UsageHistory::save(QString *error)
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
        return fail(QCoreApplication::translate("ccm", "이력 디렉터리를 만들지 못했습니다: %1")
                        .arg(QDir::toNativeSeparators(directory)));
    }

    QJsonArray samples;
    for (const UsageSample &sample : m_samples) {
        QJsonObject values;
        for (auto it = sample.values.constBegin(); it != sample.values.constEnd(); ++it) {
            values.insert(it.key(), it.value());
        }
        QJsonObject object;
        object.insert(QLatin1String(kKeyAt), sample.at.toString(Qt::ISODateWithMs));
        object.insert(QLatin1String(kKeyValues), values);
        samples.append(object);
    }

    QJsonObject root;
    root.insert(QLatin1String(kKeySchemaVersion), kSchemaVersion);
    root.insert(QLatin1String(kKeySamples), samples);

    // 쓰는 중에 프로그램이 죽어도 이전 파일이 남도록 QSaveFile 을 쓴다.
    QSaveFile out(m_filePath);
    if (!out.open(QIODevice::WriteOnly)) {
        return fail(QCoreApplication::translate("ccm", "이력 파일을 쓰기 모드로 열지 못했습니다: %1")
                        .arg(out.errorString()));
    }
    const QByteArray encoded = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (out.write(encoded) != encoded.size()) {
        out.cancelWriting();
        return fail(QCoreApplication::translate("ccm", "이력 기록 중 오류가 발생했습니다: %1").arg(out.errorString()));
    }
    if (!out.commit()) {
        return fail(QCoreApplication::translate("ccm", "이력 파일 교체에 실패했습니다: %1").arg(out.errorString()));
    }
    return true;
}

} // namespace ccm::infra
