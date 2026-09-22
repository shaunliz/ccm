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

#include "ui/AlertVisuals.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>

namespace ccm::ui {
namespace {

/// 밝은 배경과 어두운 배경 어디에 놓여도 윤곽이 보이도록 같은 색의 짙은 판을
/// 테두리로 두른다. 색만으로 칠하면 어두운 테마에서 노랑이 번져 보인다.
QPen outlinePen(const QColor &fill, qreal width)
{
    QPen pen(fill.darker(160));
    pen.setWidthF(width);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    return pen;
}

/// 알림: 느낌표. 둥근 막대와 점.
void drawNotice(QPainter *painter, const QRectF &box, const QColor &color)
{
    const qreal barWidth = box.width() * 0.18;
    const qreal barTop = box.top() + box.height() * 0.12;
    const qreal barHeight = box.height() * 0.50;
    const qreal centerX = box.center().x();

    painter->setPen(outlinePen(color, box.width() * 0.055));
    painter->setBrush(color);

    const qreal radius = barWidth / 2.0;
    painter->drawRoundedRect(
        QRectF(centerX - barWidth / 2.0, barTop, barWidth, barHeight), radius, radius);

    const qreal dotDiameter = barWidth * 1.15;
    painter->drawEllipse(QRectF(centerX - dotDiameter / 2.0,
                                box.bottom() - box.height() * 0.10 - dotDiameter,
                                dotDiameter, dotDiameter));
}

/// 경고: 세모. 꼭지가 위를 향한다.
void drawWarning(QPainter *painter, const QRectF &box, const QColor &color)
{
    const qreal inset = box.width() * 0.04;
    const QRectF area = box.adjusted(inset, inset, -inset, -inset);

    QPolygonF triangle;
    triangle << QPointF(area.center().x(), area.top())
             << QPointF(area.right(), area.bottom())
             << QPointF(area.left(), area.bottom());

    QPainterPath path;
    path.addPolygon(triangle);
    path.closeSubpath();

    painter->setPen(outlinePen(color, box.width() * 0.07));
    painter->setBrush(color);
    painter->drawPath(path);
}

/// 위험: 엑스표. 굵은 두 획.
void drawCritical(QPainter *painter, const QRectF &box, const QColor &color)
{
    const qreal inset = box.width() * 0.16;
    const QRectF area = box.adjusted(inset, inset, -inset, -inset);

    QPen pen(color);
    pen.setWidthF(box.width() * 0.22);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    painter->drawLine(area.topLeft(), area.bottomRight());
    painter->drawLine(area.topRight(), area.bottomLeft());
}

} // namespace

QColor levelColor(ccm::core::AlertLevel level)
{
    // 파랑 -> 초록 -> 주황 -> 빨강 순으로 올라간다.
    switch (level) {
    case ccm::core::AlertLevel::Normal:
        return QColor(0x2f, 0x81, 0xf7);   // 파랑. 임계치를 넘지 않은 기본 상태
    case ccm::core::AlertLevel::Notice:
        return QColor(0x1f, 0xa8, 0x55);   // 초록
    case ccm::core::AlertLevel::Warning:
        return QColor(0xe2, 0x7a, 0x12);   // 주황
    case ccm::core::AlertLevel::Critical:
        return QColor(0xd7, 0x3a, 0x49);   // 빨강
    }
    return QColor(0x2f, 0x81, 0xf7);
}

QPixmap levelPixmap(ccm::core::AlertLevel level, int size)
{
    const int side = qMax(8, size);
    QPixmap pixmap(side, side);
    pixmap.fill(Qt::transparent);

    if (level == ccm::core::AlertLevel::Normal) {
        return pixmap;
    }

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box(0.0, 0.0, side, side);
    const QColor color = levelColor(level);

    switch (level) {
    case ccm::core::AlertLevel::Notice:
        drawNotice(&painter, box, color);
        break;
    case ccm::core::AlertLevel::Warning:
        drawWarning(&painter, box, color);
        break;
    case ccm::core::AlertLevel::Critical:
        drawCritical(&painter, box, color);
        break;
    case ccm::core::AlertLevel::Normal:
        break;
    }
    return pixmap;
}

QIcon levelIcon(ccm::core::AlertLevel level, int size)
{
    if (level == ccm::core::AlertLevel::Normal) {
        return QIcon();
    }
    return QIcon(levelPixmap(level, size));
}

} // namespace ccm::ui
