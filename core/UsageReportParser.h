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
//  core/UsageReportParser.h
//
//  claude CLI 의 control_response 한 줄에서 한도 창 목록을 뽑는다.
//
//  응답 봉투
//   { "type": "control_response",
//     "response": { "subtype": "success", "request_id": "...",
//                   "response": { "rate_limits": { ... }, ... } } }
//
//  창의 출처는 rate_limits 아래 세 갈래다.
//
//   five_hour        현재 세션 (5시간)
//   seven_day        주간 전체 (모든 모델)
//   model_scoped[]   모델별 주간. 서버가 display_name 을 함께 준다. ("Fable")
//
//  seven_day_sonnet / seven_day_opus 는 플랜에 따라 채워지는 고정 이름 창이며,
//  null 이 아닐 때만 목록에 넣는다.
//
//  주의: 이 스키마는 Anthropic 이 버전에 따라 바꿀 수 있는 내부 형식이다.
//        누락과 null 은 오류가 아니라 "그 창이 없음" 으로 처리한다.
// ============================================================================
#ifndef CCM_CORE_USAGEREPORTPARSER_H
#define CCM_CORE_USAGEREPORTPARSER_H

#include "core/UsageTypes.h"

#include <QByteArray>
#include <QString>

namespace ccm::core {

struct UsageReportParseResult {
    bool ok = false;
    UsageReport report;
    QString error;
};

class UsageReportParser
{
public:
    UsageReportParser() = delete;

    /// controlResponseLine 은 control_response 한 줄 전체.
    static UsageReportParseResult parse(const QByteArray &controlResponseLine);
};

} // namespace ccm::core

#endif // CCM_CORE_USAGEREPORTPARSER_H
