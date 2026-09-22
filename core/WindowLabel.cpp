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

#include "core/WindowLabel.h"

#include <QCoreApplication>
#include <QLatin1String>

namespace ccm::core {
namespace {

/// 모델별 주간 창의 키 앞머리. 뒤에 서버가 준 모델 이름이 붙는다.
constexpr auto kModelPrefix = "model:";

} // namespace

QString shortWindowLabel(const QString &key, const QString &fullLabel)
{
    if (key == QLatin1String("five_hour")) {
        return QCoreApplication::translate("ccm", "세션");
    }
    if (key == QLatin1String("seven_day")) {
        return QCoreApplication::translate("ccm", "모델");
    }
    if (key == QLatin1String("seven_day_sonnet")) {
        return QStringLiteral("Sonnet");
    }
    if (key == QLatin1String("seven_day_opus")) {
        return QStringLiteral("Opus");
    }

    // 모델별 창은 키 뒤쪽이 곧 모델 이름이다. ("model:Fable" -> "Fable")
    // 서버가 준 이름이라 이미 짧다.
    if (key.startsWith(QLatin1String(kModelPrefix))) {
        const QString name = key.mid(QLatin1String(kModelPrefix).size());
        if (!name.isEmpty()) {
            return name;
        }
    }

    // 모르는 창이다. 나중에 서버가 늘릴 수 있으므로 빈 칸으로 두지 않는다.
    // 표시 이름에서 괄호로 붙인 꼬리표만 떼어 낸다. ("Haiku (주간)" -> "Haiku")
    QString text = fullLabel.isEmpty() ? key : fullLabel;
    const int paren = text.indexOf(QLatin1Char('('));
    if (paren > 0) {
        text = text.left(paren).trimmed();
    }
    return text;
}

} // namespace ccm::core
