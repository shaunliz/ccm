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

#include "ui/UiColors.h"

#include <QPalette>
#include <QWidget>

namespace ccm::ui {
namespace {

/// 본문색을 얼마나 섞을지. 0 이면 비활성색 그대로, 1 이면 본문색 그대로.
///
/// 비활성색(0)은 읽히기를 포기한 밝기라 너무 여리고, 0.55 는 본문과 잘 구분되지
/// 않을 만큼 진했다. 0.3 이 "회색으로 보이지만 읽히는" 자리다.
constexpr double kTowardTextRatio = 0.3;

int mixChannel(int from, int to, double ratio)
{
    return qBound(0, static_cast<int>(qRound(from + (to - from) * ratio)), 255);
}

} // namespace

QColor hintTextColor(const QPalette &palette)
{
    const QColor text = palette.color(QPalette::Active, QPalette::WindowText);
    const QColor disabled = palette.color(QPalette::Disabled, QPalette::WindowText);

    return QColor(mixChannel(disabled.red(), text.red(), kTowardTextRatio),
                  mixChannel(disabled.green(), text.green(), kTowardTextRatio),
                  mixChannel(disabled.blue(), text.blue(), kTowardTextRatio));
}

void applyHintTextColor(QWidget *widget)
{
    if (!widget) {
        return;
    }
    QPalette palette = widget->palette();
    palette.setColor(QPalette::WindowText, hintTextColor(palette));
    widget->setPalette(palette);
}

} // namespace ccm::ui
