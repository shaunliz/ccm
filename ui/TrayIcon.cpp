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

#include "ui/TrayIcon.h"

#include "core/TimeFormat.h"
#include "infra/AutoStart.h"
#include "infra/ClaudeCli.h"
#include "infra/Logger.h"
#include "infra/Settings.h"
#include "ui/AlertVisuals.h"
#include "ui/UsagePopup.h"
#include "ui/UsageService.h"

#include <QAction>
#include <QCursor>
#include <QFont>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QStringList>

namespace ccm::ui {
namespace {

constexpr int kIconSize = 32;
constexpr int kNotificationIconSize = 48;
constexpr int kNotificationTimeoutMs = 10000;

/// 가장 심각한 창. 아이콘에 그릴 숫자를 여기서 고른다.
/// 수준이 같으면 사용률이 높은 쪽을 고른다.
const ccm::core::UsageWindow *pickHeadline(const UsageService *service)
{
    const ccm::core::UsageReport &report = service->lastReport();
    const ccm::core::UsageWindow *best = nullptr;
    ccm::core::AlertLevel bestLevel = ccm::core::AlertLevel::Normal;

    for (const ccm::core::UsageWindow &window : report.windows) {
        const ccm::core::AlertLevel level = service->levelFor(window.key);
        if (!best || level > bestLevel
            || (level == bestLevel && window.usedPercent > best->usedPercent)) {
            best = &window;
            bestLevel = level;
        }
    }
    return best;
}

/// 사용률을 숫자로 그린 아이콘.
QIcon renderIcon(int percent, ccm::core::AlertLevel level, bool hasValue)
{
    QPixmap pixmap(kIconSize, kIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor color = levelColor(level);

    // 둥근 사각 배경. 작업 표시줄 색과 무관하게 숫자가 읽히도록 채워 둔다.
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRect(0, 0, kIconSize, kIconSize), 7, 7);

    QFont font = painter.font();
    font.setBold(true);
    // 세 자리(100)면 글자를 줄여야 들어간다.
    font.setPixelSize(percent >= 100 ? 13 : 17);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(QRect(0, 0, kIconSize, kIconSize), Qt::AlignCenter,
                     hasValue ? QString::number(percent) : QStringLiteral("-"));
    return QIcon(pixmap);
}

} // namespace

bool TrayIcon::isAvailable()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

TrayIcon::TrayIcon(UsageService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_tray(new QSystemTrayIcon(this))
{
    m_popup = new UsagePopup(service);
    connect(m_popup, &UsagePopup::mainWindowRequested, this, &TrayIcon::mainWindowRequested);

    m_menu = new QMenu();

    QAction *openAction = m_menu->addAction(tr("창 열기"));
    connect(openAction, &QAction::triggered, this, &TrayIcon::mainWindowRequested);

    m_refreshAction = m_menu->addAction(tr("지금 조회"));
    connect(m_refreshAction, &QAction::triggered, this, [this] { m_service->refresh(); });

    m_menu->addSeparator();

    QAction *alertAction = m_menu->addAction(tr("알림 관리"));
    connect(alertAction, &QAction::triggered, this, &TrayIcon::alertLogRequested);

    QAction *settingsAction = m_menu->addAction(tr("설정"));
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::settingsRequested);

    m_autoStartAction = m_menu->addAction(tr("시작 시 자동 실행"));
    m_autoStartAction->setCheckable(true);
    connect(m_autoStartAction, &QAction::toggled, this, [this](bool enabled) {
        QString error;
        if (!ccm::infra::AutoStart::setEnabled(enabled, &error)) {
            m_tray->showMessage(tr("시작프로그램"), error,
                                levelIcon(ccm::core::AlertLevel::Warning,
                                          kNotificationIconSize),
                                kNotificationTimeoutMs);
        }
        updateAutoStartAction();
    });

    m_menu->addSeparator();

    QAction *quitAction = m_menu->addAction(tr("종료"));
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);

    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);

    connect(m_service, &UsageService::succeeded, this, [this] { updateIconAndTooltip(); });
    connect(m_service, &UsageService::failed, this, [this] { updateIconAndTooltip(); });
    connect(m_service, &UsageService::started, this, [this] {
        m_refreshAction->setEnabled(false);
        m_tray->setToolTip(tr("Claude Code 사용량\n조회 중..."));
    });
    connect(m_service, &UsageService::alertsRaised, this, &TrayIcon::onAlertsRaised);
    connect(m_service, &UsageService::alertLogChanged, this,
            [this] { updateIconAndTooltip(); });

    updateIconAndTooltip();
    updateAutoStartAction();
}

TrayIcon::~TrayIcon()
{
    // 메뉴와 팝업은 최상위 위젯이라 부모가 없다. 직접 치운다.
    delete m_menu;
    delete m_popup;
}

void TrayIcon::show()
{
    m_tray->show();
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::MiddleClick:
        // 한 번 누르면 팝업. 열려 있으면 닫는다.
        if (m_popup->isVisible()) {
            m_popup->hide();
        } else {
            m_popup->showNear(QCursor::pos());
        }
        break;
    case QSystemTrayIcon::DoubleClick:
        m_popup->hide();
        emit mainWindowRequested();
        break;
    default:
        break;
    }
}

void TrayIcon::onAlertsRaised(const QList<ccm::core::AlertEvent> &events)
{
    if (!ccm::infra::Settings::notificationsEnabled()) {
        qCDebug(ccmApp) << "알림 팝업이 꺼져 있어 기록만 남깁니다.";
        return;
    }

    // 한 번의 조회에서 여러 임계치를 동시에 넘을 수 있다. 알림을 그만큼 쪼개
    // 띄우면 알림 센터가 도배되므로 한 건으로 묶는다.
    ccm::core::AlertLevel highest = ccm::core::AlertLevel::Normal;
    QStringList lines;
    for (const ccm::core::AlertEvent &event : events) {
        lines << event.message();
        if (event.level > highest) {
            highest = event.level;
        }
    }
    if (lines.isEmpty()) {
        return;
    }

    const QString title = events.size() == 1
                              ? tr("Claude Code 사용량 %1")
                                    .arg(ccm::core::alertLevelName(highest))
                              : tr("Claude Code 사용량 %1 (%2건)")
                                    .arg(ccm::core::alertLevelName(highest))
                                    .arg(events.size());

    // 윈도우 알림 센터(시계를 누르면 나오는 쪽)에도 이 알림이 쌓이고, 사용자가
    // 지우면 사라진다. 그 보관은 OS 가 맡는다. 프로그램 쪽 목록은 따로 남는다.
    //
    // QSystemTrayIcon::MessageIcon 대신 직접 그린 아이콘을 넘긴다. 기본 아이콘은
    // 정보/경고/위험 셋뿐이어서 우리 세 수준의 모양과 맞지 않는다.
    m_tray->showMessage(title, lines.join(QChar::LineFeed),
                        levelIcon(highest, kNotificationIconSize),
                        kNotificationTimeoutMs);
}

void TrayIcon::updateIconAndTooltip()
{
    m_refreshAction->setEnabled(true);

    const ccm::core::UsageWindow *headline = pickHeadline(m_service);
    const bool hasValue = headline != nullptr;
    const int percent = hasValue ? qRound(headline->usedPercent) : 0;
    const ccm::core::AlertLevel level =
        hasValue ? m_service->levelFor(headline->key) : ccm::core::AlertLevel::Normal;

    m_tray->setIcon(renderIcon(percent, level, hasValue));

    QStringList tooltip;
    tooltip << tr("Claude Code 사용량");
    if (hasValue) {
        tooltip << ccm::infra::ClaudeCli::describe(m_service->lastReport());
        const QDateTime updated = m_service->lastUpdatedAt();
        if (updated.isValid()) {
            tooltip << tr("마지막 업데이트 %1")
                           .arg(ccm::core::formatDateTimeWithSeconds(updated));
        }
    } else {
        tooltip << tr("아직 조회한 값이 없습니다.");
    }
    const int unread = m_service->unacknowledgedAlertCount();
    if (unread > 0) {
        tooltip << tr("미확인 알림 %1건").arg(unread);
    }
    if (!m_service->lastError().isEmpty()) {
        tooltip << tr("최근 조회 실패: %1").arg(m_service->lastError());
    }

    // 도구 설명은 마지막 조회값을 즉시 보여 준다. 올릴 때마다 조회하지 않는다.
    m_tray->setToolTip(tooltip.join(QChar::LineFeed));
}

void TrayIcon::updateAutoStartAction()
{
    const bool enabled = ccm::infra::AutoStart::isEnabled();
    // 신호를 막지 않으면 상태를 맞추는 것만으로 toggled 가 다시 돌아온다.
    const QSignalBlocker blocker(m_autoStartAction);
    m_autoStartAction->setChecked(enabled);
}

} // namespace ccm::ui
