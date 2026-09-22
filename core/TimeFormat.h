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
//  core/TimeFormat.h
//
//  화면에 보일 날짜와 시각 문자열. 형식을 한곳에 모은다.
//
//  왜 한곳에 모으는가
//   같은 시각이 화면 일곱 군데에 나온다. (게이지의 재설정 시각, 요약, 트레이
//   도구 설명, 팝업, 알림 목록, 차트 축) 자리마다 서식 문자열을 적으면 곳마다
//   표기가 달라지고, 바꿀 때 한 곳을 빠뜨린다.
//
//  왜 QDateTime::toString 의 요일을 쓰지 않는가
//   Qt 6 의 `QDateTime::toString(format)` 은 요일과 달 이름을 영어로 채운다.
//   (`ddd` -> "Tue") 화면 글자가 모두 한글인데 요일만 영어로 섞이므로 직접
//   붙인다. QLocale 을 끼우는 방법도 있지만, Qt 판이나 CLDR 판에 따라 약칭이
//   달라질 수 있어 표를 고정해 둔다. 이 프로그램은 한국어 화면만 갖는다.
//
//  오늘이어도 날짜를 적는다
//   처음에는 오늘이면 시각만 보이게 했다. 그러면 같은 값이 자리에 따라 다르게
//   보여(게이지는 "18:40", 요약은 "9월 11일(금) 18:40") 같은 것인지 헷갈린다.
//   길어지더라도 모든 자리에서 같은 형태를 쓴다.
//
//  들어오는 시각
//   저장된 시각은 모두 UTC 다. 아래 함수들은 받은 값을 지역 시각으로 바꿔
//   표시한다. 이미 지역 시각이면 바꾸는 일이 없으므로 어느 쪽을 넘겨도 된다.
// ============================================================================
#ifndef CCM_CORE_TIMEFORMAT_H
#define CCM_CORE_TIMEFORMAT_H

#include <QDateTime>
#include <QString>

namespace ccm::core {

/// "9월 15일(화)"
QString formatDate(const QDateTime &when);

/// "9월 15일(화) 23:00"
QString formatDateTime(const QDateTime &when);

/// "9월 15일(화) 23:00:05"
QString formatDateTimeWithSeconds(const QDateTime &when);

/// 서식 문자열만 받는 자리(QDateTimeAxis 등)에 넘길 값.
///
/// 요일은 넣지 않는다. 그 자리를 Qt 가 영어 이름으로 채우기 때문이다.
/// 숫자와 한글 리터럴만 쓰므로 어느 로케일에서도 같게 나온다.
QString axisTimeFormat();
QString axisDateTimeFormat();

} // namespace ccm::core

#endif // CCM_CORE_TIMEFORMAT_H
