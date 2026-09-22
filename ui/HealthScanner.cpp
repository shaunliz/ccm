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

#include "ui/HealthScanner.h"

#include "ui/AlertVisuals.h"

#include <QFontMetrics>
#include <QPixmap>
#include <QLinearGradient>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

#include <cmath>

namespace ccm::ui {
namespace {

/// 한 바퀴. M_PI 는 표준이 아니라 컴파일러마다 있고 없어서 직접 둔다.
constexpr double kTurn = 6.283185307179586;

/// 다시 그리는 간격. 30ms 면 초당 33장이라 눈에 매끄럽다.
constexpr int kFrameIntervalMs = 30;

/// 한 번 왕복하는 데 걸리는 시간. 상태마다 다르다.
constexpr int kSweepMsOk = 2600;
constexpr int kSweepMsBusy = 1100;   ///< 조회 중에는 빨라진다. 일하는 중이라는 뜻.
constexpr int kSweepMsError = 1800;  ///< 오가지 않고 깜박이는 주기.

constexpr int kBarHeight = 12;
constexpr int kTextGap = 10;

/// 상태 아이콘과 띠 사이의 간격.
constexpr int kIconGap = 6;

/// 빛의 폭. 띠 전체 폭에 대한 비율이다.
constexpr double kLightWidthRatio = 0.22;

/// 꺼져 있는 자리도 아주 희미하게 남긴다. 띠가 어디까지인지 보이게 한다.
constexpr int kTrackAlpha = 38;

/// 빛 한가운데의 진하기.
constexpr int kLightAlpha = 235;

/// 깜박일 때 가장 어두운 지점의 진하기. 0 까지 떨어뜨리면 꺼진 것처럼 보인다.
constexpr int kPulseFloorAlpha = 60;

} // namespace

HealthScanner::HealthScanner(QWidget *parent)
    : QWidget(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(kFrameIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &HealthScanner::step);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStatus(HealthState::Ok, tr("시작하는 중"));
}

QSize HealthScanner::sizeHint() const
{
    const int height = qMax(kBarHeight, fontMetrics().height()) + 4;
    return QSize(240, height);
}

void HealthScanner::setStatus(HealthState state, const QString &message, const QString &detail)
{
    m_state = state;
    m_message = message;
    setToolTip(detail.isEmpty() ? message : detail);

    applyPace();
    update();
}

void HealthScanner::setUsageLevel(ccm::core::AlertLevel level)
{
    if (m_usageLevel == level) {
        return;
    }
    m_usageLevel = level;
    update();
}

void HealthScanner::applyPace()
{
    int sweepMs = kSweepMsOk;
    switch (m_state) {
    case HealthState::Busy:
        sweepMs = kSweepMsBusy;
        break;
    case HealthState::Error:
        sweepMs = kSweepMsError;
        break;
    case HealthState::Ok:
    case HealthState::Warning:
        sweepMs = kSweepMsOk;
        break;
    }

    // 한 주기(2π)를 sweepMs 에 도는 걸음 크기.
    m_stepSize = kTurn * kFrameIntervalMs / sweepMs;
}

void HealthScanner::step()
{
    m_phase += m_stepSize;
    if (m_phase >= kTurn) {
        m_phase -= kTurn;
    }
    update();
}

void HealthScanner::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_timer->start();
}

void HealthScanner::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    // 트레이로 내려간 동안에는 아무도 보지 않는다.
    m_timer->stop();
}

QColor HealthScanner::messageColor() const
{
    switch (m_state) {
    case HealthState::Warning:
        return levelColor(ccm::core::AlertLevel::Warning);
    case HealthState::Error:
        return levelColor(ccm::core::AlertLevel::Critical);
    case HealthState::Ok:
    case HealthState::Busy:
        break;
    }
    // 걸리는 것이 없을 때는 글자도 사용량 수준을 말한다. 띠와 같은 색이 맞다.
    return levelColor(m_usageLevel);
}

QPixmap HealthScanner::stateIcon(int size) const
{
    // 프로그램에 걸리는 것이 있을 때만 그린다. 늘 떠 있으면 눈이 무시한다.
    switch (m_state) {
    case HealthState::Warning:
        return levelPixmap(ccm::core::AlertLevel::Warning, size);
    case HealthState::Error:
        return levelPixmap(ccm::core::AlertLevel::Critical, size);
    case HealthState::Ok:
    case HealthState::Busy:
        break;
    }
    return QPixmap();
}

void HealthScanner::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 색은 사용량 수준만 말한다. 프로그램 상태는 움직임과 아이콘이 맡는다.
    // (헤더 주석 참조)
    const QColor color = levelColor(m_usageLevel);

    // --- 글자 자리를 먼저 떼어 낸다 ---
    const int textWidth = m_message.isEmpty()
                              ? 0
                              : fontMetrics().horizontalAdvance(m_message) + kTextGap;

    QRect barRect = rect();
    barRect.setWidth(qMax(0, barRect.width() - textWidth));
    barRect.setHeight(kBarHeight);
    barRect.moveTop((height() - kBarHeight) / 2);

    if (textWidth > 0) {
        // 글자는 자기가 말하는 통로의 색을 쓴다. "훅 미등록" 은 프로그램 이야기
        // 이므로 왼쪽 아이콘과 같은 색이어야 한 묶음으로 읽힌다. 띠 색(사용량)을
        // 쓰면 초록 바탕에 "훅 미등록" 이 적혀 어느 쪽 말인지 흐려진다.
        QRect textRect = rect();
        textRect.setLeft(barRect.right() + kTextGap);
        painter.setPen(messageColor());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, m_message);
    }

    // --- 상태 아이콘은 띠 왼쪽에 ---
    const int iconSize = qMax(kBarHeight, fontMetrics().height() - 2);
    const QPixmap icon = stateIcon(iconSize);
    if (!icon.isNull()) {
        const int top = (height() - iconSize) / 2;
        painter.drawPixmap(barRect.left(), top, icon);
        barRect.setLeft(barRect.left() + iconSize + kIconGap);
    }

    if (barRect.width() <= 0) {
        return;
    }

    const double radius = kBarHeight / 2.0;

    // --- 바탕 띠 ---
    QColor track = color;
    track.setAlpha(kTrackAlpha);
    painter.setPen(Qt::NoPen);
    painter.setBrush(track);
    painter.drawRoundedRect(barRect, radius, radius);

    // --- 빛 ---
    if (m_state == HealthState::Error) {
        // 오가지 않는다. 띠 전체가 숨을 쉰다. 멈춰 있다는 것을 움직임으로 말한다.
        const double breath = (1.0 - std::cos(m_phase)) / 2.0;
        QColor glow = color;
        glow.setAlpha(kPulseFloorAlpha
                      + static_cast<int>((kLightAlpha - kPulseFloorAlpha) * breath));
        painter.setBrush(glow);
        painter.drawRoundedRect(barRect, radius, radius);
        return;
    }

    // 좌우 끝에서 부드럽게 되돌아오도록 코사인을 쓴다. 일정한 속도로 튕기면
    // 끝에서 딱 꺾여 기계적으로 보인다.
    const double center = (1.0 - std::cos(m_phase)) / 2.0;

    const double lightWidth = barRect.width() * kLightWidthRatio;
    const double centerX = barRect.left() + center * barRect.width();

    // 그러데이션은 띠 전체에 걸고, 빛 주변에만 색을 둔다. 좌표를 0~1 비율로
    // 주므로 띠 폭이 바뀌어도 모양이 유지된다.
    QLinearGradient gradient(barRect.left(), 0, barRect.right(), 0);
    const double span = barRect.width() > 0 ? barRect.width() : 1.0;
    const double half = lightWidth / span / 2.0;
    const double at = (centerX - barRect.left()) / span;

    QColor edge = color;
    edge.setAlpha(0);
    QColor core = color;
    core.setAlpha(kLightAlpha);
    QColor mid = color;
    mid.setAlpha(kLightAlpha / 3);

    const auto put = [&gradient](double position, const QColor &value) {
        gradient.setColorAt(qBound(0.0, position, 1.0), value);
    };

    put(0.0, edge);
    put(at - half, edge);
    put(at - half / 2.0, mid);
    put(at, core);
    put(at + half / 2.0, mid);
    put(at + half, edge);
    put(1.0, edge);

    painter.setBrush(gradient);
    painter.drawRoundedRect(barRect, radius, radius);
}

} // namespace ccm::ui
