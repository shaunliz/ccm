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
//  ui/UsageService.h
//
//  사용량 조회를 비동기로 돌리고, 결과를 이력/알림까지 흘려보낸다.
//  프로그램에서 "지금 값" 을 아는 유일한 지점이다.
//
//  왜 비동기인가
//   조회 1회에 claude 세션이 하나 뜨고 1~2초가 걸린다. 트레이 상주 프로그램이
//   몇 분마다 그만큼 멈추면 쓸 수 없다. QProcess 시그널로 받으므로 스레드는
//   쓰지 않는다.
//
//  한 번에 하나만
//   조회가 진행 중인데 또 요청이 오면(사용자가 버튼을 연타하거나, 폴링과
//   수동 조회가 겹치거나) 무시한다. claude 세션을 여러 개 띄울 이유가 없다.
// ============================================================================
#ifndef CCM_UI_USAGESERVICE_H
#define CCM_UI_USAGESERVICE_H

#include "core/AlertEvaluator.h"
#include "core/AlertTypes.h"
#include "core/UsageTypes.h"
#include "infra/AlertStore.h"
#include "infra/UsageHistory.h"

#include <QByteArray>
#include <QDateTime>
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QString>

class QProcess;
class QTimer;

namespace ccm::ui {

class UsageService : public QObject
{
    Q_OBJECT

public:
    explicit UsageService(QObject *parent = nullptr);
    ~UsageService() override;

    /// 이력과 알림 목록을 읽어 올린다. 생성 직후 1회 호출한다.
    void start();

    /// 즉시 1회 조회. 진행 중이면 아무것도 하지 않고 false 를 돌려준다.
    bool refresh();

    bool busy() const { return m_busy; }

    /// 자동 조회 간격(분). 0 이면 자동 조회를 멈춘다.
    void setPollIntervalMinutes(int minutes);
    int pollIntervalMinutes() const { return m_pollMinutes; }

    const ccm::core::UsageReport &lastReport() const { return m_lastReport; }
    QDateTime lastUpdatedAt() const { return m_lastReport.fetchedAt; }
    QString lastError() const { return m_lastError; }
    QByteArray lastRawResponse() const { return m_lastRaw; }

    /// 현재 넘고 있는 임계치 중 가장 심각한 수준. 트레이 아이콘 색이 쓴다.
    ccm::core::AlertLevel highestLevel() const { return m_highestLevel; }

    /// 창 하나가 현재 넘고 있는 임계치 중 가장 심각한 수준. 게이지 색이 쓴다.
    ccm::core::AlertLevel levelFor(const QString &windowKey) const;

    /// 임계치가 바뀌었을 때 호출한다. 다시 조회하지 않고 현재 보고를 재평가한다.
    void reloadThresholds();

    const ccm::core::ThresholdConfig &thresholds() const { return m_thresholds; }
    const ccm::infra::UsageHistory &history() const { return m_history; }
    const ccm::infra::AlertStore &alerts() const { return m_alerts; }

    int unacknowledgedAlertCount() const { return m_alerts.unacknowledgedCount(); }

    // --- 알림 목록 조작 ---
    // 어느 화면에서 눌러도 같은 저장소를 고쳐야 하므로 서비스를 거친다.
    void acknowledgeAlert(const QString &id);
    void acknowledgeAllAlerts();
    void removeAlerts(const QList<QString> &ids);
    void clearAlerts();

signals:
    void started();
    void succeeded(const ccm::core::UsageReport &report);
    void failed(const QString &error);

    /// 새로 알릴 것이 생겼을 때. 트레이가 받아 윈도우 알림을 띄운다.
    void alertsRaised(const QList<ccm::core::AlertEvent> &events);

    /// 알림 목록이 바뀌었을 때. (추가, 확인, 삭제 모두)
    void alertLogChanged();

private:
    void onProcessFinished();
    void onReadyRead();
    void onTimeout();
    void onProcessError();

    void finishWithFailure(const QString &error);
    void handleResponse(const QByteArray &line);
    void evaluateAlerts(const ccm::core::UsageReport &report);
    void teardownProcess();

    QProcess *m_process = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QTimer *m_pollTimer = nullptr;

    QByteArray m_pending;      ///< 아직 줄이 완성되지 않은 표준 출력.
    bool m_busy = false;
    int m_pollMinutes = 0;

    /// 조회 한 번에 걸린 시간. 로그에만 쓴다. 느려졌을 때 어디서 느려졌는지
    /// 알려면 숫자가 있어야 한다.
    QElapsedTimer m_elapsed;

    /// 이번 조회에서 응답 전에 지나간 줄 수. claude 는 control_response 앞에
    /// system 이벤트를 여러 줄 낸다.
    int m_skippedLines = 0;

    ccm::core::UsageReport m_lastReport;
    QByteArray m_lastRaw;
    QString m_lastError;

    ccm::core::ThresholdConfig m_thresholds;
    ccm::core::FiredMarkMap m_marks;
    ccm::core::AlertLevel m_highestLevel = ccm::core::AlertLevel::Normal;

    ccm::infra::UsageHistory m_history;
    ccm::infra::AlertStore m_alerts;
};

} // namespace ccm::ui

#endif // CCM_UI_USAGESERVICE_H
