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

#include "ui/AppIcon.h"

#include <QColor>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>

namespace ccm::ui {
namespace {

// 색은 프로그램의 다른 곳과 맞춘다. 파랑은 정상 상태의 색이고, 초록은 알림
// 수준의 색이다. 아이콘이 낯선 색을 쓰면 같은 프로그램으로 보이지 않는다.
const QColor kBackTop(0x2b, 0x3a, 0x4d);      // 어두운 청회색
const QColor kBackBottom(0x1b, 0x25, 0x33);
const QColor kScreen(0x12, 0x1a, 0x24);
const QColor kLine(0x2f, 0x81, 0xf7);         // 파랑. 꺾은선
const QColor kBody(0xe8, 0xed, 0xf3);         // 로봇 몸통
const QColor kBodyEdge(0x9a, 0xa7, 0xb8);
const QColor kEye(0x1f, 0xa8, 0x55);          // 초록. 눈
const QColor kLeg(0xc3, 0xcd, 0xda);

/// 이 크기 미만에서는 로봇을 그리지 않는다.
///
/// 32px 까지는 몸통과 다리가 서로 붙어 형체를 알 수 없는 덩어리가 된다.
/// 눈으로 여러 크기를 늘어놓고 견주어 정한 값이다. 작은 자리(제목줄, 작업
/// 표시줄)에서는 꺾은선만 남기고, 48px 이상에서 로봇을 함께 그린다.
constexpr int kRobotMinSize = 40;

/// 둥근 사각 배경.
void drawBackground(QPainter *painter, qreal side)
{
    QLinearGradient gradient(0, 0, 0, side);
    gradient.setColorAt(0.0, kBackTop);
    gradient.setColorAt(1.0, kBackBottom);

    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawRoundedRect(QRectF(0, 0, side, side), side * 0.22, side * 0.22);
}

/// 모니터 화면과 그 안의 꺾은선.
///
/// \param compact 작은 크기에서는 모니터 테두리와 받침을 생략하고 꺾은선만
///                굵게 그린다. 선 하나가 남는 편이 형체 없는 덩어리보다 낫다.
void drawMonitor(QPainter *painter, qreal side, bool compact)
{
    const QRectF screen = compact
                              ? QRectF(side * 0.14, side * 0.24, side * 0.72, side * 0.52)
                              : QRectF(side * 0.14, side * 0.20, side * 0.72, side * 0.44);

    if (!compact) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(kScreen);
        painter->drawRoundedRect(screen, side * 0.05, side * 0.05);

        QPen edge(kBodyEdge);
        edge.setWidthF(side * 0.035);
        painter->setPen(edge);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(screen, side * 0.05, side * 0.05);

        // 받침대. 화면 아래 짧은 목과 발.
        painter->setPen(Qt::NoPen);
        painter->setBrush(kBodyEdge);
        const qreal neckWidth = side * 0.08;
        painter->drawRect(QRectF(side * 0.5 - neckWidth / 2, screen.bottom(),
                                 neckWidth, side * 0.06));
        painter->drawRoundedRect(QRectF(side * 0.34, screen.bottom() + side * 0.06,
                                        side * 0.32, side * 0.045),
                                 side * 0.02, side * 0.02);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(kScreen);
        painter->drawRoundedRect(screen, side * 0.08, side * 0.08);
    }

    // 꺾은선. 왼쪽 아래에서 오른쪽 위로 오르는 네 점.
    const QRectF plot = screen.adjusted(screen.width() * 0.14, screen.height() * 0.18,
                                        -screen.width() * 0.12, -screen.height() * 0.18);
    QPolygonF points;
    points << QPointF(plot.left(), plot.bottom())
           << QPointF(plot.left() + plot.width() * 0.32, plot.bottom() - plot.height() * 0.34)
           << QPointF(plot.left() + plot.width() * 0.62, plot.bottom() - plot.height() * 0.26)
           << QPointF(plot.right(), plot.top());

    QPen line(kLine);
    line.setWidthF(compact ? side * 0.11 : side * 0.075);
    line.setCapStyle(Qt::RoundCap);
    line.setJoinStyle(Qt::RoundJoin);
    painter->setPen(line);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(points);
}

/// 네 다리로 선 로봇. 모니터 앞 오른쪽 아래에 겹쳐 앉는다.
void drawRobot(QPainter *painter, qreal side)
{
    const QRectF body(side * 0.46, side * 0.56, side * 0.40, side * 0.26);

    // 다리 넷. 몸통 아래에서 벌어져 내려온다. 두 개는 앞, 두 개는 뒤라
    // 길이를 달리해 네 개로 보이게 한다.
    QPen leg(kLeg);
    leg.setWidthF(side * 0.045);
    leg.setCapStyle(Qt::RoundCap);
    painter->setPen(leg);
    painter->setBrush(Qt::NoBrush);

    const qreal legTop = body.bottom() - side * 0.02;
    const qreal legBottom = side * 0.93;
    const QList<qreal> offsets{0.10, 0.34, 0.62, 0.88};
    for (int index = 0; index < offsets.size(); ++index) {
        const qreal x = body.left() + body.width() * offsets.at(index);
        // 가운데 두 다리를 조금 짧게 그려 앞뒤가 있는 것처럼 보이게 한다.
        const bool inner = index == 1 || index == 2;
        const qreal bottom = inner ? legBottom - side * 0.05 : legBottom;
        const qreal spread = (x - body.center().x()) * 0.35;
        painter->drawLine(QPointF(x, legTop), QPointF(x + spread, bottom));
    }

    // 몸통.
    painter->setPen(Qt::NoPen);
    painter->setBrush(kBody);
    painter->drawRoundedRect(body, side * 0.09, side * 0.09);

    QPen edge(kBodyEdge);
    edge.setWidthF(side * 0.025);
    painter->setPen(edge);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(body, side * 0.09, side * 0.09);

    // 눈 둘. 초록으로 켜 둔다. "보고 있다" 를 나타내는 부분이다.
    const qreal eyeDiameter = side * 0.075;
    const qreal eyeY = body.top() + body.height() * 0.34;
    painter->setPen(Qt::NoPen);
    painter->setBrush(kEye);
    painter->drawEllipse(
        QRectF(body.left() + body.width() * 0.24 - eyeDiameter / 2, eyeY,
               eyeDiameter, eyeDiameter));
    painter->drawEllipse(
        QRectF(body.left() + body.width() * 0.62 - eyeDiameter / 2, eyeY,
               eyeDiameter, eyeDiameter));

    // 더듬이 하나. 로봇으로 읽히게 하는 값싼 단서다.
    QPen antenna(kBodyEdge);
    antenna.setWidthF(side * 0.028);
    antenna.setCapStyle(Qt::RoundCap);
    painter->setPen(antenna);
    const QPointF antennaBase(body.center().x(), body.top());
    const QPointF antennaTip(body.center().x() + side * 0.03, body.top() - side * 0.09);
    painter->drawLine(antennaBase, antennaTip);
    painter->setPen(Qt::NoPen);
    painter->setBrush(kEye);
    painter->drawEllipse(QRectF(antennaTip.x() - side * 0.028,
                                antennaTip.y() - side * 0.028,
                                side * 0.056, side * 0.056));
}

} // namespace

QPixmap appIconPixmap(int size)
{
    const int side = qMax(8, size);
    QPixmap pixmap(side, side);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal length = side;
    drawBackground(&painter, length);

    // 작은 크기에서는 로봇까지 넣으면 둘 다 알아볼 수 없다. 꺾은선만 남긴다.
    const bool compact = side < kRobotMinSize;
    drawMonitor(&painter, length, compact);
    if (!compact) {
        drawRobot(&painter, length);
    }
    return pixmap;
}

QIcon appIcon()
{
    QIcon icon;
    for (int size : {16, 20, 24, 32, 48, 64, 128, 256}) {
        icon.addPixmap(appIconPixmap(size));
    }
    return icon;
}

} // namespace ccm::ui
