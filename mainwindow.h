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
//  mainwindow.h
//
//  메인 화면. 위에서부터 추이 차트 / 현재값 게이지 / 텍스트 요약이다.
//
//  창을 닫는 것과 종료하는 것이 다르다
//   트레이 상주 프로그램이므로 닫기(X)는 트레이로 숨기는 동작이다. 종료는
//   트레이 메뉴나 파일 메뉴에서만 한다. 트레이를 쓸 수 없는 환경에서는
//   닫기가 곧 종료여야 하므로 그 경우를 따로 다룬다.
// ============================================================================
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/UsageTypes.h"
#include "infra/Settings.h"

#include <QList>
#include <QString>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QAction;
class QLabel;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ccm::ui {
class AlertLogDialog;
class HealthScanner;
class UsageChart;
class UsageGauge;
class UsageService;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ccm::ui::UsageService *service, bool trayAvailable,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

    /// 트레이에서 불러 창을 다시 띄운다. 최소화되어 있어도 올라온다.
    void showAndRaise();

public slots:
    void openSettings();
    void openAlertLog();
    void openAbout();
    void registerStatuslineHook();

signals:
    /// 진짜 종료. 트레이가 아니라 여기서 요청해도 되도록 신호로 뺀다.
    void quitRequested();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void buildMenus();
    void hideToTray(QCloseEvent *event);
    void onReportReady(const ccm::core::UsageReport &report);
    void onFailed(const QString &error);
    void onStarted();
    void refreshChart();

    /// 고른 임계치가 하나도 없을 때 대신 그릴 것. 가장 먼저 닿는 임계치 하나다.
    ccm::infra::ChartThresholds firstThresholdToDraw() const;
    void updateSummary();
    void updateGauges(const ccm::core::UsageReport &report);
    void updateAlertButton();

    /// 도구 > 디버그 콘솔. 로그를 실시간으로 볼 콘솔 창을 붙이고 뗀다.
    void toggleDebugConsole(bool on);

    /// 상태 표시등을 지금 사정에 맞춘다.
    void updateHealth();

    /// statusline 훅이 우리 것으로 등록되어 있는지 다시 본다. 파일을 읽으므로
    /// 매 그리기마다 하지 않고, 바뀔 만한 자리에서만 부른다.
    void refreshHookState();

    Ui::MainWindow *ui;
    ccm::ui::UsageService *m_service = nullptr;
    bool m_trayAvailable = false;
    bool m_quitting = false;
    bool m_closeHintShown = false;

    /// 상태 표시등이 읽는 값들.
    bool m_busy = false;
    bool m_hookOk = true;
    QString m_lastError;

    /// 마지막으로 로그에 남긴 표시등 상태. 같은 줄을 되풀이하지 않으려고 든다.
    QString m_healthNote;

    ccm::ui::UsageChart *m_chart = nullptr;
    QVBoxLayout *m_gaugeLayout = nullptr;
    QList<ccm::ui::UsageGauge *> m_gauges = {};
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_updatedLabel = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_alertButton = nullptr;
    ccm::ui::HealthScanner *m_health = nullptr;
    QAction *m_consoleAction = nullptr;
    ccm::ui::AlertLogDialog *m_alertLog = nullptr;
};

#endif // MAINWINDOW_H
