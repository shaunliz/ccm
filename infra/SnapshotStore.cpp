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

#include "infra/SnapshotStore.h"

#include "core/SnapshotParser.h"
#include "infra/ClaudePaths.h"
#include "infra/Logger.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <utility>

namespace ccm::infra {
namespace {

constexpr auto kKeySchemaVersion = "schemaVersion";
constexpr auto kKeyCapturedAt = "capturedAt";
constexpr auto kKeyPayload = "payload";

/// 스냅샷 파일이 이 크기를 넘으면 비정상으로 본다. statusline JSON 은 수 KB 수준이다.
constexpr qint64 kMaxSnapshotBytes = 1 * 1024 * 1024;

} // namespace

SnapshotStore::SnapshotStore(QString filePath)
    : m_filePath(filePath.isEmpty() ? ClaudePaths::snapshotFilePath() : std::move(filePath))
{
    qCDebug(ccmInfra) << "스냅샷 저장소 경로:" << m_filePath;
}

bool SnapshotStore::write(const QByteArray &statuslinePayload, QString *error) const
{
    const auto fail = [&error](const QString &message) {
        qCCritical(ccmInfra).noquote() << message;
        if (error != nullptr) {
            *error = message;
        }
        return false;
    };

    if (statuslinePayload.trimmed().isEmpty()) {
        return fail(QCoreApplication::translate("ccm", "기록할 내용이 비어 있습니다."));
    }

    QJsonParseError parseError{};
    const QJsonDocument payloadDocument = QJsonDocument::fromJson(statuslinePayload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !payloadDocument.isObject()) {
        return fail(QCoreApplication::translate("ccm", "statusline 입력이 올바른 JSON 객체가 아닙니다: %1")
                        .arg(parseError.errorString()));
    }

    // 상위 디렉터리는 실제 대상 경로를 기준으로 만든다.
    // 기본 경로에서는 appDataDir 이 곧 상위 디렉터리이므로 같은 처리로 덮이고,
    // --snapshot 으로 다른 경로를 받은 경우에도 동작한다.
    const QString directory = QFileInfo(m_filePath).absolutePath();
    if (!directory.isEmpty() && !QDir().mkpath(directory)) {
        return fail(QCoreApplication::translate("ccm", "스냅샷 디렉터리를 만들지 못했습니다: %1").arg(directory));
    }

    // payload 는 손대지 않고 그대로 보존한다.
    // 위젯을 붙일 때 statusline 이 준 모든 필드를 여기서 다시 꺼낼 수 있어야 한다.
    QJsonObject wrapper;
    wrapper.insert(QLatin1String(kKeySchemaVersion), ccm::core::SnapshotParser::schemaVersion());
    wrapper.insert(QLatin1String(kKeyCapturedAt),
                   QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    wrapper.insert(QLatin1String(kKeyPayload), payloadDocument.object());

    // QSaveFile 은 임시 파일에 기록한 뒤 commit 시점에 원자적으로 교체한다.
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return fail(QCoreApplication::translate("ccm", "스냅샷 파일을 쓰기 모드로 열지 못했습니다: %1")
                        .arg(file.errorString()));
    }

    const QByteArray encoded = QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
    if (file.write(encoded) != encoded.size()) {
        file.cancelWriting();
        return fail(QCoreApplication::translate("ccm", "스냅샷 기록 중 오류가 발생했습니다: %1").arg(file.errorString()));
    }

    if (!file.commit()) {
        return fail(QCoreApplication::translate("ccm", "스냅샷 교체에 실패했습니다: %1").arg(file.errorString()));
    }

    qCInfo(ccmInfra) << "스냅샷을 기록했습니다." << encoded.size() << "바이트 ->" << m_filePath;
    return true;
}

SnapshotReadResult SnapshotStore::read() const
{
    SnapshotReadResult result;

    const QFileInfo info(m_filePath);
    if (!info.isFile()) {
        qCDebug(ccmInfra) << "스냅샷 파일이 없습니다:" << m_filePath;
        return result;
    }

    result.exists = true;
    result.modifiedAt = info.lastModified().toUTC();

    if (info.size() > kMaxSnapshotBytes) {
        result.error = QCoreApplication::translate("ccm", "스냅샷 파일이 비정상적으로 큽니다: %1 바이트")
                           .arg(info.size());
        qCCritical(ccmInfra).noquote() << result.error;
        return result;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QCoreApplication::translate("ccm", "스냅샷 파일을 열지 못했습니다: %1").arg(file.errorString());
        qCCritical(ccmInfra).noquote() << result.error;
        return result;
    }

    result.content = file.readAll();
    file.close();

    if (result.content.trimmed().isEmpty()) {
        result.error = QCoreApplication::translate("ccm", "스냅샷 파일이 비어 있습니다.");
        qCWarning(ccmInfra).noquote() << result.error;
        return result;
    }

    result.ok = true;
    qCDebug(ccmInfra) << "스냅샷 읽기 완료." << result.content.size() << "바이트"
                      << "modifiedAt=" << result.modifiedAt.toString(Qt::ISODateWithMs);
    return result;
}

} // namespace ccm::infra
