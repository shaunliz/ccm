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
//  core/SnapshotParser.h
//
//  JSON 에서 한도 사용률을 뽑는다. 두 가지 형태를 모두 받는다.
//
//   1. 원본 형태 : statusline 이 stdin 으로 넘긴 JSON 그대로 (훅이 쓰는 경로)
//   2. 래퍼 형태 : { "schemaVersion": 1, "capturedAt": "...", "payload": { ... } }
//                  (SnapshotStore 가 기록한 파일을 다시 읽는 경로)
//
//  주의: statusline JSON 스키마는 Anthropic 이 버전에 따라 변경할 수 있는
//        내부 형식이다. 누락 필드는 오류가 아니라 present=false 로 처리한다.
// ============================================================================
#ifndef CCM_CORE_SNAPSHOTPARSER_H
#define CCM_CORE_SNAPSHOTPARSER_H

#include "core/UsageTypes.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>

namespace ccm::core {

struct ParseResult {
    bool ok = false;
    UsageSnapshot snapshot;
    QString error;
};

class SnapshotParser
{
public:
    SnapshotParser() = delete;

    /// json 은 위 두 형태 중 하나.
    /// fallbackCapturedAt 은 capturedAt 을 찾지 못했을 때 쓸 UTC 시각이다.
    /// (원본 형태에는 capturedAt 이 없으므로 항상 대체값이 쓰인다.)
    static ParseResult parse(const QByteArray &json,
                             const QDateTime &fallbackCapturedAt = QDateTime());

    /// 스냅샷 파일 래퍼가 사용하는 스키마 버전.
    static int schemaVersion();
};

} // namespace ccm::core

#endif // CCM_CORE_SNAPSHOTPARSER_H
