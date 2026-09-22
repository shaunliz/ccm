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

#include "infra/StatusLineCache.h"

#include "core/WindowLabel.h"
#include "infra/ClaudePaths.h"
#include "infra/Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace ccm::infra {
namespace {

constexpr int kSchemaVersion = 1;

constexpr auto kKeySchemaVersion = "schemaVersion";
constexpr auto kKeyAt = "at";
constexpr auto kKeyWindows = "windows";
constexpr auto kKeyLabel = "label";
constexpr auto kKeyUsedPercent = "usedPercent";

/// 이 크기를 넘으면 우리가 쓴 파일이 아니라고 본다. 정상이면 수백 바이트다.
constexpr qint64 kMaxBytes = 64 * 1024;

} // namespace

StatusLineCache::StatusLineCache(const QString &filePath)
    : m_filePath(filePath.isEmpty() ? ClaudePaths::statusLineCacheFilePath() : filePath)
{
}

bool StatusLineCache::write(const ccm::core::UsageReport &report, QString *error) const
{
    const auto fail = [&error](const QString &message) {
        qCWarning(ccmInfra).noquote() << message;
        if (error != nullptr) {
            *error = message;
        }
        return false;
    };

    QJsonArray windows;
    for (const ccm::core::UsageWindow &window : report.windows) {
        // 훅이 직접 받는 두 창은 담지 않는다. (헤더 주석 참조)
        if (window.kind != ccm::core::UsageWindowKind::WeeklyModel) {
            continue;
        }

        QJsonObject entry;
        entry.insert(QLatin1String(kKeyLabel),
                     ccm::core::shortWindowLabel(window.key, window.label));
        entry.insert(QLatin1String(kKeyUsedPercent), window.usedPercent);
        windows.append(entry);
    }

    QJsonObject root;
    root.insert(QLatin1String(kKeySchemaVersion), kSchemaVersion);
    root.insert(QLatin1String(kKeyAt),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    root.insert(QLatin1String(kKeyWindows), windows);

    const QString directory = QFileInfo(m_filePath).absolutePath();
    if (!directory.isEmpty() && !QDir().mkpath(directory)) {
        return fail(QCoreApplication::translate("ccm", "디렉터리를 만들지 못했습니다: %1")
                        .arg(QDir::toNativeSeparators(directory)));
    }

    // 훅이 5초마다 읽어 가는 파일이다. 반쯤 쓰인 것을 읽는 일이 없도록
    // QSaveFile 로 통째로 바꿔 끼운다.
    QSaveFile out(m_filePath);
    if (!out.open(QIODevice::WriteOnly)) {
        return fail(QCoreApplication::translate("ccm", "상태줄 캐시를 열지 못했습니다: %1")
                        .arg(out.errorString()));
    }
    const QByteArray encoded = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (out.write(encoded) != encoded.size() || !out.commit()) {
        return fail(QCoreApplication::translate("ccm", "상태줄 캐시를 쓰지 못했습니다: %1")
                        .arg(out.errorString()));
    }

    qCDebug(ccmInfra) << "상태줄 캐시를 갱신했습니다. 모델별 창" << windows.size() << "개";
    return true;
}

QList<ccm::core::StatusLineExtra> StatusLineCache::read(int maxAgeMinutes) const
{
    const QFileInfo info(m_filePath);
    if (!info.isFile() || info.size() > kMaxBytes) {
        return {};
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!document.isObject()) {
        return {};
    }

    const QJsonObject root = document.object();
    if (root.value(QLatin1String(kKeySchemaVersion)).toInt() != kSchemaVersion) {
        return {};
    }

    // 기록된 시각으로 낡음을 본다. 파일 수정 시각을 쓰지 않는 이유는, 내용이
    // 같아도 다시 쓰면 시각이 갱신되어 낡은 값이 새것처럼 보이기 때문이다.
    const QDateTime at =
        QDateTime::fromString(root.value(QLatin1String(kKeyAt)).toString(), Qt::ISODateWithMs);
    if (!at.isValid()) {
        return {};
    }
    if (maxAgeMinutes > 0 && at.secsTo(QDateTime::currentDateTimeUtc()) > qint64(maxAgeMinutes) * 60) {
        return {};
    }

    QList<ccm::core::StatusLineExtra> extras;
    const QJsonArray windows = root.value(QLatin1String(kKeyWindows)).toArray();
    for (const QJsonValue &value : windows) {
        const QJsonObject entry = value.toObject();
        const QString label = entry.value(QLatin1String(kKeyLabel)).toString();
        if (label.isEmpty()) {
            continue;
        }
        extras.append(ccm::core::StatusLineExtra{
            label, entry.value(QLatin1String(kKeyUsedPercent)).toDouble()});
    }
    return extras;
}

} // namespace ccm::infra
