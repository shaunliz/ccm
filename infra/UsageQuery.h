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

// ============================================================================
//  infra/UsageQuery.h
//
//  스냅샷 파일을 1회 읽어 사용량을 만든다.
//
//  statusline 훅은 푸시 방식이므로 위젯이 원할 때 값을 요청할 수는 없다.
//  여기서 하는 일은 훅이 남겨 둔 파일을 읽는 것뿐이다.
// ============================================================================
#ifndef CCM_INFRA_USAGEQUERY_H
#define CCM_INFRA_USAGEQUERY_H

#include "core/UsageTypes.h"

#include <QByteArray>
#include <QString>

namespace ccm::infra {

struct UsageQueryResult {
    enum class Status {
        Ok,
        NoSnapshot,   ///< 파일이 없다. 훅이 아직 실행되지 않았다.
        ReadFailed,
        ParseFailed
    };

    Status status = Status::NoSnapshot;
    ccm::core::UsageSnapshot snapshot;
    QString snapshotPath;
    QString error;
    QByteArray rawContent;    ///< 읽은 원본 그대로. 디버그 표시에 쓴다.
    qint64 ageSeconds = 0;    ///< capturedAt 으로부터 경과한 초.

    bool ok() const { return status == Status::Ok; }
};

class UsageQuery
{
public:
    UsageQuery() = delete;

    /// filePath 를 비우면 ClaudePaths::snapshotFilePath() 를 쓴다.
    static UsageQueryResult run(const QString &filePath = QString());

    /// 사람이 읽을 여러 줄 텍스트. 실패 사유와 다음 조치까지 담는다.
    static QString describe(const UsageQueryResult &result);
};

} // namespace ccm::infra

#endif // CCM_INFRA_USAGEQUERY_H
