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

#include "ui/UsageGauge.h"

#include "core/TimeFormat.h"
#include "ui/UiColors.h"

#include <QCoreApplication>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>

namespace ccm::ui {
namespace {

constexpr int kBarHeight = 14;
constexpr int kBarRadius = 7;
constexpr int kLabelGap = 4;
constexpr int kResetGap = 3;
constexpr int kIconGap = 4;

/// 재설정 시각 표기. 오늘이면 시각만, 다른 날이면 날짜까지 붙인다.
QString formatReset(const QDateTime &resetsAtUtc)
{
    if (!resetsAtUtc.isValid()) {
        return QString();
    }

    const QDateTime local = resetsAtUtc.toLocalTime();
    const qint64 seconds = QDateTime::currentDateTime().secsTo(local);

    const QString when = ccm::core::formatDateTime(local);

    if (seconds <= 0) {
        return QCoreApplication::translate("ccm", "%1 재설정").arg(when);
    }

    const qint64 hours = seconds / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    if (hours > 0) {
        return QCoreApplication::translate("ccm", "%1 재설정 (%2시간 %3분 후)")
            .arg(when)
            .arg(hours)
            .arg(minutes);
    }
    return QCoreApplication::translate("ccm", "%1 재설정 (%2분 후)").arg(when).arg(minutes);
}

} // namespace

UsageGauge::UsageGauge(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void UsageGauge::setWindow(const ccm::core::UsageWindow &window, ccm::core::AlertLevel level)
{
    m_label = window.label;
    m_usedPercent = window.usedPercent;
    m_resetText = formatReset(window.resetsAt);
    m_level = level;
    m_hasValue = true;
    update();
}

void UsageGauge::clearValue(const QString &label)
{
    m_label = label;
    m_usedPercent = 0.0;
    m_resetText.clear();
    m_level = ccm::core::AlertLevel::Normal;
    m_hasValue = false;
    update();
}

QSize UsageGauge::minimumSizeHint() const
{
    const QFontMetrics metrics(font());
    // 이름 줄 + 막대 + 재설정 줄.
    const int height = metrics.height() + kLabelGap + kBarHeight + kResetGap + metrics.height();
    return QSize(180, height);
}

QSize UsageGauge::sizeHint() const
{
    return minimumSizeHint();
}

void UsageGauge::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QFontMetrics metrics(font());
    const QColor textColor = palette().color(QPalette::WindowText);
    const QColor hintColor = hintTextColor(palette());

    // --- 1. 이름과 사용률 ---
    const int labelTop = 0;
    const QString valueText = m_hasValue
                                  ? QStringLiteral("%1%").arg(m_usedPercent, 0, 'f', 1)
                                  : QStringLiteral("-");

    QFont valueFont = font();
    valueFont.setBold(true);

    painter.setPen(textColor);
    painter.drawText(QRect(0, labelTop, width(), metrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, m_label);

    painter.setFont(valueFont);
    painter.setPen(m_hasValue ? levelColor(m_level) : hintColor);
    const QRect valueRect(0, labelTop, width(), metrics.height());
    painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter, valueText);

    // 수준 아이콘을 사용률 왼쪽에 붙인다. 색만으로는 색약인 사용자와 흑백
    // 화면에서 세 수준이 같아진다.
    if (m_hasValue && m_level != ccm::core::AlertLevel::Normal) {
        const int iconSide = metrics.height();
        const int valueWidth = QFontMetrics(valueFont).horizontalAdvance(valueText);
        const QRect iconRect(width() - valueWidth - kIconGap - iconSide, labelTop,
                             iconSide, iconSide);
        painter.drawPixmap(iconRect, levelPixmap(m_level, iconSide));
    }
    painter.setFont(font());

    // --- 2. 막대 ---
    const int barTop = labelTop + metrics.height() + kLabelGap;
    const QRect barRect(0, barTop, width(), kBarHeight);

    QColor trackColor = palette().color(QPalette::Window);
    trackColor = trackColor.lightness() > 128 ? trackColor.darker(115) : trackColor.lighter(160);

    painter.setPen(Qt::NoPen);
    painter.setBrush(trackColor);
    painter.drawRoundedRect(barRect, kBarRadius, kBarRadius);

    if (m_hasValue && m_usedPercent > 0.0) {
        const double ratio = qBound(0.0, m_usedPercent / 100.0, 1.0);
        // 0 에 가까운 값도 보이도록 최소 폭을 준다. 보이지 않으면 값이 없는
        // 것과 구별되지 않는다.
        const int filled = qMax(kBarRadius * 2, static_cast<int>(width() * ratio));
        painter.setBrush(levelColor(m_level));
        painter.drawRoundedRect(QRect(barRect.left(), barRect.top(), filled, barRect.height()),
                                kBarRadius, kBarRadius);
    }

    // --- 3. 재설정 시각 ---
    if (!m_resetText.isEmpty()) {
        const int resetTop = barRect.bottom() + kResetGap;
        painter.setPen(hintColor);
        painter.drawText(QRect(0, resetTop, width(), metrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, m_resetText);
    }
}

} // namespace ccm::ui
