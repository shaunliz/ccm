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

#include "core/TimeFormat.h"

#include <QCoreApplication>
#include <QDate>
#include <QTime>

namespace ccm::core {
namespace {

/// QDate::dayOfWeek() 는 월요일이 1, 일요일이 7 이다.
///
/// 목록을 static 으로 담아 두지 않는다. 함수 지역 static 은 첫 호출에 한 번만
/// 초기화되므로, 번역기가 설치되기 전에 한 번이라도 불리면 그때의 글자가 굳어
/// 그 뒤로 언어를 따라가지 않는다.
QString weekdayName(int dayOfWeek)
{
    switch (dayOfWeek) {
    case 1:
        return QCoreApplication::translate("ccm", "월");
    case 2:
        return QCoreApplication::translate("ccm", "화");
    case 3:
        return QCoreApplication::translate("ccm", "수");
    case 4:
        return QCoreApplication::translate("ccm", "목");
    case 5:
        return QCoreApplication::translate("ccm", "금");
    case 6:
        return QCoreApplication::translate("ccm", "토");
    case 7:
        return QCoreApplication::translate("ccm", "일");
    default:
        return QString();
    }
}

QString datePart(const QDate &date)
{
    return QCoreApplication::translate("ccm", "%1월 %2일(%3)")
        .arg(date.month())
        .arg(date.day())
        .arg(weekdayName(date.dayOfWeek()));
}

QString timePart(const QTime &time, bool withSeconds)
{
    return time.toString(withSeconds ? QStringLiteral("HH:mm:ss")
                                     : QStringLiteral("HH:mm"));
}

} // namespace

QString formatDate(const QDateTime &when)
{
    if (!when.isValid()) {
        return QString();
    }
    return datePart(when.toLocalTime().date());
}

QString formatDateTime(const QDateTime &when)
{
    if (!when.isValid()) {
        return QString();
    }
    const QDateTime local = when.toLocalTime();
    return QStringLiteral("%1 %2").arg(datePart(local.date()), timePart(local.time(), false));
}

QString formatDateTimeWithSeconds(const QDateTime &when)
{
    if (!when.isValid()) {
        return QString();
    }
    const QDateTime local = when.toLocalTime();
    return QStringLiteral("%1 %2").arg(datePart(local.date()), timePart(local.time(), true));
}

QString axisTimeFormat()
{
    return QStringLiteral("HH:mm");
}

QString axisDateTimeFormat()
{
    // 작은따옴표 안은 서식 지정자가 아니라 글자 그대로 나간다.
    return QCoreApplication::translate("ccm", "M'월' d'일' HH:mm");
}

} // namespace ccm::core
