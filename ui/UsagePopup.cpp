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

#include "ui/UsagePopup.h"

#include "ui/UsageGauge.h"
#include "core/TimeFormat.h"
#include "ui/UiColors.h"
#include "ui/UsageService.h"

#include <QFrame>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

namespace ccm::ui {
namespace {

constexpr int kPopupWidth = 320;
constexpr int kScreenMargin = 8;

} // namespace

UsagePopup::UsagePopup(UsageService *service, QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_service(service)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFixedWidth(kPopupWidth);

    auto *frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setAutoFillBackground(true);

    auto *inner = new QVBoxLayout(frame);
    inner->setContentsMargins(12, 12, 12, 10);
    inner->setSpacing(10);

    auto *title = new QLabel(tr("Claude Code 사용량"), frame);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    inner->addWidget(title);

    m_gaugeLayout = new QVBoxLayout();
    m_gaugeLayout->setSpacing(10);
    inner->addLayout(m_gaugeLayout);

    m_statusLabel = new QLabel(frame);
    m_statusLabel->setWordWrap(true);
    applyHintTextColor(m_statusLabel);
    inner->addWidget(m_statusLabel);

    m_openButton = new QPushButton(tr("창 열기"), frame);
    connect(m_openButton, &QPushButton::clicked, this, [this] {
        hide();
        emit mainWindowRequested();
    });
    inner->addWidget(m_openButton);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(frame);

    connect(m_service, &UsageService::succeeded, this, [this] { rebuild(); });
    connect(m_service, &UsageService::failed, this, [this] { updateStatusLine(); });
    connect(m_service, &UsageService::started, this, [this] { updateStatusLine(); });
}

void UsagePopup::showNear(const QPoint &anchor)
{
    // 먼저 마지막 값으로 채운다. 조회는 1~2초 걸리므로 빈 팝업을 보여 주지 않는다.
    rebuild();
    adjustSize();

    const QScreen *screen = QGuiApplication::screenAt(anchor);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    QPoint target = anchor;
    if (screen) {
        const QRect available = screen->availableGeometry();
        // 트레이는 대개 화면 오른쪽 아래에 있다. 팝업이 잘리지 않도록 안으로 당긴다.
        target.setX(qBound(available.left() + kScreenMargin,
                           anchor.x() - width() / 2,
                           available.right() - width() - kScreenMargin));
        target.setY(anchor.y() - height() - kScreenMargin);
        if (target.y() < available.top() + kScreenMargin) {
            target.setY(anchor.y() + kScreenMargin);
        }
    }

    move(target);
    show();
    raise();

    // 열 때마다 새로 조회한다. 진행 중이면 무시된다.
    m_service->refresh();
}

void UsagePopup::rebuild()
{
    const ccm::core::UsageReport &report = m_service->lastReport();

    // 창의 개수가 조회마다 달라질 수 있으므로 게이지를 필요한 수만큼 맞춘다.
    while (m_gauges.size() < report.windows.size()) {
        auto *gauge = new UsageGauge(this);
        m_gauges.append(gauge);
        m_gaugeLayout->addWidget(gauge);
    }
    while (m_gauges.size() > report.windows.size()) {
        UsageGauge *gauge = m_gauges.takeLast();
        m_gaugeLayout->removeWidget(gauge);
        gauge->deleteLater();
    }

    for (int i = 0; i < report.windows.size(); ++i) {
        const ccm::core::UsageWindow &window = report.windows.at(i);
        m_gauges.at(i)->setWindow(window, m_service->levelFor(window.key));
    }

    updateStatusLine();
    adjustSize();
}

void UsagePopup::updateStatusLine()
{
    if (m_service->busy()) {
        m_statusLabel->setText(tr("조회 중..."));
        return;
    }

    if (!m_service->lastError().isEmpty() && !m_service->lastReport().valid) {
        m_statusLabel->setText(m_service->lastError());
        return;
    }

    const QDateTime updated = m_service->lastUpdatedAt();
    if (!updated.isValid()) {
        m_statusLabel->setText(tr("아직 조회한 값이 없습니다."));
        return;
    }

    const qint64 age = updated.secsTo(QDateTime::currentDateTimeUtc());
    QString text = tr("마지막 업데이트 %1 (%2초 전)")
                       .arg(ccm::core::formatDateTimeWithSeconds(updated))
                       .arg(age);
    if (!m_service->lastError().isEmpty()) {
        text += QChar::LineFeed;
        text += tr("최근 조회 실패: %1").arg(m_service->lastError());
    }
    m_statusLabel->setText(text);
}

} // namespace ccm::ui
