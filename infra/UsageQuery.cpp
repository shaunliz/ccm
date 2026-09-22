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

#include "infra/UsageQuery.h"

#include "core/SnapshotParser.h"
#include "core/TimeFormat.h"
#include "infra/Logger.h"
#include "infra/SnapshotStore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QStringList>

namespace ccm::infra {
namespace {

QString describeWindow(const QString &label, const ccm::core::RateLimitWindow &window)
{
    if (!window.present) {
        return QCoreApplication::translate("ccm", "  %1 : 데이터 없음 (스냅샷에 해당 필드가 없습니다)").arg(label);
    }
    return QStringLiteral("  %1 : %2%").arg(label).arg(window.usedPercent, 0, 'f', 1);
}

} // namespace

UsageQueryResult UsageQuery::run(const QString &filePath)
{
    UsageQueryResult result;

    const SnapshotStore store(filePath);
    result.snapshotPath = store.filePath();

    const SnapshotReadResult raw = store.read();
    if (!raw.exists) {
        result.status = UsageQueryResult::Status::NoSnapshot;
        result.error = QCoreApplication::translate("ccm", "스냅샷이 없습니다. statusline 훅이 아직 실행되지 않았습니다.");
        qCWarning(ccmInfra).noquote() << result.error;
        return result;
    }
    if (!raw.ok) {
        result.status = UsageQueryResult::Status::ReadFailed;
        result.error = QCoreApplication::translate("ccm", "스냅샷 읽기 실패: %1").arg(raw.error);
        return result;
    }

    result.rawContent = raw.content;

    const ccm::core::ParseResult parsed =
        ccm::core::SnapshotParser::parse(raw.content, raw.modifiedAt);
    if (!parsed.ok) {
        result.status = UsageQueryResult::Status::ParseFailed;
        result.error = QCoreApplication::translate("ccm", "스냅샷 파싱 실패: %1").arg(parsed.error);
        return result;
    }

    result.snapshot = parsed.snapshot;
    result.ageSeconds = parsed.snapshot.capturedAt.secsTo(QDateTime::currentDateTimeUtc());
    result.status = UsageQueryResult::Status::Ok;

    qCDebug(ccmInfra) << "사용량 조회 완료. age=" << result.ageSeconds << "초";
    return result;
}

QString UsageQuery::describe(const UsageQueryResult &result)
{
    QStringList lines;

    if (!result.ok()) {
        lines << result.error;
        lines << QString();
        lines << QCoreApplication::translate("ccm", "스냅샷 경로: %1").arg(result.snapshotPath);

        if (result.status == UsageQueryResult::Status::NoSnapshot) {
            lines << QString();
            lines << QCoreApplication::translate("ccm", "Claude Code 가 상태줄을 갱신할 때 만들어집니다.");
            lines << QCoreApplication::translate("ccm", "훅을 방금 등록했다면 Claude Code 를 재시작하십시오.");
        }
        return lines.join(QChar::LineFeed);
    }

    const ccm::core::UsageSnapshot &snapshot = result.snapshot;

    lines << QCoreApplication::translate("ccm", "  기록 시각 : %1 (%2초 전)")
                 .arg(ccm::core::formatDateTimeWithSeconds(snapshot.capturedAt))
                 .arg(result.ageSeconds);
    lines << QString();
    lines << describeWindow(QCoreApplication::translate("ccm", "5시간 창"), snapshot.fiveHour);
    lines << describeWindow(QCoreApplication::translate("ccm", "7일 창  "), snapshot.sevenDay);
    return lines.join(QChar::LineFeed);
}

} // namespace ccm::infra
