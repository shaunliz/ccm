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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "core/TimeFormat.h"
#include "infra/ClaudeCli.h"
#include "infra/HookRegistrar.h"
#include "infra/Logger.h"
#include "infra/Settings.h"
#include "ui/AboutDialog.h"
#include "ui/AlertLogDialog.h"
#include "ui/CloseChoice.h"
#include "ui/DebugConsole.h"
#include "ui/HealthScanner.h"
#include "ui/HookSetup.h"
#include "ui/SettingsDialog.h"
#include "ui/StandardButtons.h"
#include "ui/UiColors.h"
#include "ui/UsageChart.h"
#include "ui/UsageGauge.h"
#include "ui/UsageService.h"

#include <QAction>
#include <QCloseEvent>
#include <QSignalBlocker>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStringList>
#include <QVBoxLayout>

namespace {

constexpr int kChartRangeHours = 24;

/// 아래쪽 버튼을 키우는 비율.
constexpr double kButtonScale = 1.3;

/// 버튼을 기본 크기보다 키운다.
///
/// 스타일이 정한 크기(sizeHint)를 기준으로 재므로, 글꼴이나 테마가 달라져도
/// 비율이 유지된다. 소수는 반올림한다.
void enlarge(QPushButton *button)
{
    const QSize hint = button->sizeHint();
    button->setMinimumSize(qRound(hint.width() * kButtonScale),
                           qRound(hint.height() * kButtonScale));
}

} // namespace

MainWindow::MainWindow(ccm::ui::UsageService *service, bool trayAvailable, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_service(service)
    , m_trayAvailable(trayAvailable)
{
    ui->setupUi(this);
    setWindowTitle(tr("Claude Code 사용량 모니터"));
    resize(640, 760);

    buildUi();
    buildMenus();

    connect(m_service, &ccm::ui::UsageService::succeeded,
            this, &MainWindow::onReportReady);
    connect(m_service, &ccm::ui::UsageService::failed, this, &MainWindow::onFailed);
    connect(m_service, &ccm::ui::UsageService::started, this, &MainWindow::onStarted);
    connect(m_service, &ccm::ui::UsageService::alertLogChanged,
            this, &MainWindow::updateAlertButton);

    // 이미 받아 둔 값이 있으면(트레이로 먼저 떠 있던 경우) 곧바로 그린다.
    if (m_service->lastReport().valid) {
        onReportReady(m_service->lastReport());
    } else {
        updateSummary();
    }
    updateAlertButton();
    refreshHookState();
    updateHealth();
}

void MainWindow::updateAlertButton()
{
    const int unread = m_service->unacknowledgedAlertCount();
    // 미확인 건수를 버튼에 얹는다. 창을 열지 않아도 볼 것이 있는지 알 수 있다.
    m_alertButton->setText(unread > 0 ? tr("알림 관리 (%1)").arg(unread)
                                      : tr("알림 관리"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    // --- 상태 표시등 ---
    // 몇 분에 한 번 조회하는 프로그램이라, 화면이 멈춘 것인지 값이 그대로인
    // 것인지 구별되지 않는다. 움직이는 것을 하나 둔다.
    m_health = new ccm::ui::HealthScanner(central);
    layout->addWidget(m_health);

    // --- 추이 차트 ---
    auto *chartGroup = new QGroupBox(tr("사용률 추이 (최근 %1시간)").arg(kChartRangeHours),
                                     central);
    auto *chartLayout = new QVBoxLayout(chartGroup);
    m_chart = new ccm::ui::UsageChart(chartGroup);
    m_chart->setRangeHours(kChartRangeHours);
    chartLayout->addWidget(m_chart);
    layout->addWidget(chartGroup, 1);

    // --- 현재값 게이지 ---
    auto *gaugeGroup = new QGroupBox(tr("현재 사용량"), central);
    m_gaugeLayout = new QVBoxLayout(gaugeGroup);
    m_gaugeLayout->setSpacing(12);
    layout->addWidget(gaugeGroup);

    // --- 텍스트 요약 ---
    auto *textGroup = new QGroupBox(tr("요약"), central);
    auto *textLayout = new QVBoxLayout(textGroup);

    m_summaryLabel = new QLabel(textGroup);
    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_summaryLabel->setWordWrap(true);
    textLayout->addWidget(m_summaryLabel);

    m_updatedLabel = new QLabel(textGroup);
    m_updatedLabel->setWordWrap(true);
    ccm::ui::applyHintTextColor(m_updatedLabel);
    textLayout->addWidget(m_updatedLabel);

    layout->addWidget(textGroup);

    // --- 버튼 ---
    auto *buttonRow = new QHBoxLayout();
    m_refreshButton = new QPushButton(tr("지금 조회"), central);
    connect(m_refreshButton, &QPushButton::clicked, this, [this] {
        qCDebug(ccmApp) << "사용자가 [지금 조회] 를 눌렀습니다.";
        m_service->refresh();
    });
    buttonRow->addWidget(m_refreshButton);

    m_alertButton = new QPushButton(tr("알림 관리"), central);
    connect(m_alertButton, &QPushButton::clicked, this, &MainWindow::openAlertLog);
    buttonRow->addWidget(m_alertButton);

    auto *settingsButton = new QPushButton(tr("설정"), central);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    buttonRow->addWidget(settingsButton);

    // 기본 크기가 작다. 세 개 모두 같은 비율로 키운다.
    for (QPushButton *button : {m_refreshButton, m_alertButton, settingsButton}) {
        enlarge(button);
    }

    layout->addLayout(buttonRow);

    setCentralWidget(central);
}

void MainWindow::buildMenus()
{
    // 윈도우 11 기본 스타일은 메뉴 제목 사이를 넓게 벌린다. 항목이 둘뿐이라
    // "파일" 과 "도구" 가 멀찍이 떨어져 한 묶음으로 보이지 않는다. 좌우 여백만
    // 줄인다. 나머지 모양은 스타일이 정한 그대로 둔다.
    menuBar()->setStyleSheet(QStringLiteral("QMenuBar::item { padding: 4px 8px; margin: 0; }"));

    QMenu *fileMenu = menuBar()->addMenu(tr("파일(&F)"));
    QAction *refreshAction = fileMenu->addAction(tr("지금 조회"));
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, [this] {
        qCDebug(ccmApp) << "메뉴에서 조회를 요청했습니다.";
        m_service->refresh();
    });

    fileMenu->addSeparator();
    QAction *quitAction = fileMenu->addAction(tr("종료"));
    connect(quitAction, &QAction::triggered, this, [this] {
        m_quitting = true;
        emit quitRequested();
    });

    QMenu *toolsMenu = menuBar()->addMenu(tr("도구(&T)"));
    QAction *alertAction = toolsMenu->addAction(tr("알림 관리"));
    connect(alertAction, &QAction::triggered, this, &MainWindow::openAlertLog);

    QAction *settingsAction = toolsMenu->addAction(tr("설정"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    toolsMenu->addSeparator();
    QAction *hookAction = toolsMenu->addAction(tr("statusline 훅 등록"));
    connect(hookAction, &QAction::triggered, this, &MainWindow::registerStatuslineHook);

    // 콘솔은 윈도우에서만 붙일 수 있다. 쓸 수 없는 곳에서는 항목 자체를 두지
    // 않는다. 눌러도 안 되는 메뉴는 없는 것만 못하다.
    if (ccm::ui::DebugConsole::isAvailable()) {
        toolsMenu->addSeparator();
        m_consoleAction = toolsMenu->addAction(tr("디버그 콘솔"));
        m_consoleAction->setCheckable(true);
        m_consoleAction->setChecked(ccm::ui::DebugConsole::isOpen());
        m_consoleAction->setStatusTip(
            tr("로그를 실시간으로 볼 콘솔 창을 붙입니다. 릴리즈 빌드에서도 동작합니다."));
        connect(m_consoleAction, &QAction::toggled, this, &MainWindow::toggleDebugConsole);
    }

    // 도움말은 맨 오른쪽에 둔다. 윈도우 관례이고, 조작 항목(도구) 사이에
    // 정보 항목이 끼지 않게 된다.
    QMenu *helpMenu = menuBar()->addMenu(tr("도움말(&H)"));
    QAction *aboutAction = helpMenu->addAction(tr("ClaudeCodeMonitor 정보"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::openAbout);
}

void MainWindow::toggleDebugConsole(bool on)
{
    if (!on) {
        ccm::ui::DebugConsole::close();
        statusBar()->showMessage(tr("디버그 콘솔을 닫았습니다."), 5000);
        return;
    }

    QString error;
    if (ccm::ui::DebugConsole::open(&error)) {
        statusBar()->showMessage(tr("디버그 콘솔을 열었습니다."), 5000);
        return;
    }

    // 열지 못했다. 확인란이 켜진 채로 두면 열린 것처럼 보인다.
    ccm::ui::showWarning(this, tr("디버그 콘솔"), error);
    QSignalBlocker blocker(m_consoleAction);
    m_consoleAction->setChecked(false);
}

void MainWindow::openAbout()
{
    ccm::ui::AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::showAndRaise()
{
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized);
    raise();
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 파일 > 종료나 트레이 메뉴에서 온 것은 이미 뜻이 분명하다. 묻지 않는다.
    if (m_quitting) {
        event->accept();
        emit quitRequested();
        return;
    }

    // 트레이를 쓸 수 없으면 숨을 곳이 없다. 닫기가 곧 종료이고, 고를 것도 없다.
    if (!m_trayAvailable) {
        event->accept();
        emit quitRequested();
        return;
    }

    if (!ccm::infra::Settings::askOnClose()) {
        if (ccm::infra::Settings::quitOnClose()) {
            event->accept();
            emit quitRequested();
            return;
        }
        hideToTray(event);
        return;
    }

    bool remember = false;
    const ccm::ui::CloseChoice choice = ccm::ui::askCloseChoice(this, &remember);

    if (choice == ccm::ui::CloseChoice::Cancel) {
        // 취소와 함께 켠 체크는 무시한다. "묻지 말고 앞으로 취소해라" 는 뜻이
        // 성립하지 않는다.
        event->ignore();
        return;
    }

    if (remember) {
        ccm::infra::Settings::setAskOnClose(false);
        ccm::infra::Settings::setQuitOnClose(choice == ccm::ui::CloseChoice::Quit);
        qCInfo(ccmApp) << "종료 시 묻지 않도록 저장했습니다. 완전 종료="
                       << (choice == ccm::ui::CloseChoice::Quit);
    }

    if (choice == ccm::ui::CloseChoice::Quit) {
        event->accept();
        emit quitRequested();
        return;
    }
    hideToTray(event);
}

void MainWindow::hideToTray(QCloseEvent *event)
{
    event->ignore();
    hide();

    if (!m_closeHintShown) {
        m_closeHintShown = true;
        statusBar()->showMessage(tr("트레이에서 계속 동작합니다. 종료는 트레이 메뉴에서."),
                                 5000);
        qCInfo(ccmApp) << "창을 숨기고 트레이에서 계속 동작합니다.";
    }
}

void MainWindow::onStarted()
{
    m_refreshButton->setEnabled(false);
    m_busy = true;
    updateHealth();
    statusBar()->showMessage(tr("claude 에 사용량을 요청하는 중..."));
}

void MainWindow::onReportReady(const ccm::core::UsageReport &report)
{
    m_refreshButton->setEnabled(true);
    m_busy = false;
    m_lastError.clear();
    updateGauges(report);
    refreshChart();
    updateSummary();
    // 훅은 남의 설정 파일이라 프로그램 밖에서도 바뀐다. 조회할 때마다 다시 본다.
    // 작은 JSON 하나를 읽는 일이고 몇 분에 한 번이라 값이 싸다.
    refreshHookState();
    updateHealth();
    statusBar()->showMessage(tr("한도 창 %1개를 조회했습니다.").arg(report.windows.size()),
                             5000);
}

void MainWindow::refreshChart()
{
    const ccm::core::UsageReport &report = m_service->lastReport();

    // 이력에는 창 키만 남으므로 차트가 쓸 이름표를 따로 넘긴다.
    QHash<QString, QString> labels;
    for (const ccm::core::UsageWindow &window : report.windows) {
        labels.insert(window.key, window.label);
    }

    // 임계치는 지표마다 여러 개이므로 전부 그으면 화면이 선으로 덮인다.
    // 무엇을 그릴지는 설정 화면의 확인란이 정한다. (최대 세 개)
    ccm::infra::ChartThresholds thresholds = ccm::infra::Settings::chartThresholds();
    if (!ccm::infra::Settings::chartThresholdsConfigured()) {
        // 아직 고른 적이 없다. 가장 먼저 닿는 것 하나를 그려 둔다. 처음 켠
        // 화면에 기준선이 하나도 없으면 높낮이만 보인다.
        //
        // 골라 놓고 모두 끈 경우는 여기 오지 않는다. 그것은 "그리지 말라" 는
        // 뜻이므로 그대로 따른다.
        thresholds = firstThresholdToDraw();
    }
    m_chart->refresh(m_service->history(), labels, thresholds);
}

ccm::infra::ChartThresholds MainWindow::firstThresholdToDraw() const
{
    const ccm::core::UsageReport &report = m_service->lastReport();
    const ccm::core::ThresholdConfig &config = m_service->thresholds();

    ccm::infra::ChartThreshold lowest;
    for (const ccm::core::UsageWindow &window : report.windows) {
        const ccm::core::ThresholdRules rules = config.rulesFor(window);
        for (const ccm::core::ThresholdRule &rule : rules) {
            // 꺼 둔 규칙은 대신 그려 주지 않는다. 고른 적이 없을 때 보여 주는
            // 기본값이므로, 쓰지 않기로 한 줄을 골라 주는 것은 뜻에 어긋난다.
            // (사용자가 직접 고른 것은 켜짐과 무관하게 그린다)
            if (!rule.enabled) {
                continue;
            }
            if (lowest.percent < 0.0 || rule.percent < lowest.percent) {
                lowest.windowKey = window.key;
                lowest.percent = rule.percent;
            }
        }
    }

    if (lowest.windowKey.isEmpty()) {
        return ccm::infra::ChartThresholds{};
    }
    return ccm::infra::ChartThresholds{lowest};
}

void MainWindow::onFailed(const QString &error)
{
    m_refreshButton->setEnabled(true);
    m_busy = false;
    m_lastError = error;
    updateSummary();
    updateHealth();
    statusBar()->showMessage(error, 10000);
}

void MainWindow::refreshHookState()
{
    QString registered;
    const ccm::infra::HookRegistrar::State state =
        ccm::infra::HookRegistrar::state(&registered);
    const bool ok = state == ccm::infra::HookRegistrar::State::ThisApp;

    if (ok != m_hookOk) {
        qCInfo(ccmApp).noquote()
            << QStringLiteral("statusline 훅 상태가 바뀌었습니다: %1")
                   .arg(ok ? QStringLiteral("등록됨")
                           : (registered.isEmpty()
                                  ? QStringLiteral("등록되지 않음")
                                  : QStringLiteral("다른 명령이 등록됨 - %1")
                                        .arg(registered)));
    }
    m_hookOk = ok;
}

void MainWindow::updateHealth()
{
    if (!m_health) {
        return;
    }

    // 표시등이 무엇을 보이고 있는지 콘솔에서도 따라갈 수 있게 한다. 바뀔 때만
    // 남긴다. 매번 남기면 5분마다 같은 줄이 쌓인다.
    const auto announce = [this](const char *what) {
        if (m_healthNote != QLatin1String(what)) {
            m_healthNote = QLatin1String(what);
            qCInfo(ccmApp) << "상태 표시등:" << what;
        }
    };

    const ccm::core::UsageReport &report = m_service->lastReport();
    const ccm::core::AlertLevel level = m_service->highestLevel();

    // 색은 사용량 수준이 정한다. 트레이 아이콘도 같은 값을 쓰므로 둘이 언제나
    // 같은 색을 보인다.
    m_health->setUsageLevel(level);

    // 도구 설명에는 두 통로를 모두 적는다. 색과 움직임이 각각 무엇을 말하는지
    // 글로도 확인할 수 있어야 한다.
    QStringList detail;
    detail << tr("사용량 수준: %1").arg(ccm::core::alertLevelName(level));
    if (report.valid) {
        detail << tr("마지막 조회: %1")
                      .arg(ccm::core::formatDateTimeWithSeconds(
                          report.fetchedAt.toLocalTime()));
    }

    // 아래 차례가 곧 우선순위다. 조회 중이 맨 앞인 이유는, 실패한 뒤 다시 부르는
    // 동안에도 "지금 고치는 중" 이 보여야 하기 때문이다.
    if (m_busy) {
        detail << tr("claude 에 사용량을 요청하는 중입니다.");
        announce("조회 중");
        m_health->setStatus(ccm::ui::HealthState::Busy, tr("조회 중"),
                            detail.join(QChar::LineFeed));
        return;
    }

    if (!m_lastError.isEmpty()) {
        detail << m_lastError;
        announce("오류 - 조회 실패");
        m_health->setStatus(ccm::ui::HealthState::Error, tr("조회 실패"),
                            detail.join(QChar::LineFeed));
        return;
    }

    if (!m_hookOk) {
        detail << tr("statusline 훅이 등록되어 있지 않습니다. Claude Code 터미널 "
                     "아래에 사용률이 표시되지 않을 뿐, 조회와 알림은 그대로 "
                     "동작합니다.");
        detail << tr("[설정 > 환경 설정 > statusline 훅] 에서 등록할 수 있습니다.");
        announce("주의 - statusline 훅 미등록");
        m_health->setStatus(ccm::ui::HealthState::Warning, tr("훅 미등록"),
                            detail.join(QChar::LineFeed));
        return;
    }

    if (!report.valid) {
        detail << tr("아직 조회하지 않았습니다.");
        announce("대기 중 - 아직 조회하지 않음");
        m_health->setStatus(ccm::ui::HealthState::Ok, tr("대기 중"),
                            detail.join(QChar::LineFeed));
        return;
    }

    // 걸리는 것이 없다. 글자도 사용량 수준을 말하게 둔다. 빨간 램프 옆에
    // "정상" 이라고 적혀 있으면 어느 쪽을 믿어야 할지 알 수 없다.
    announce("정상");
    m_health->setStatus(ccm::ui::HealthState::Ok, ccm::core::alertLevelName(level),
                        detail.join(QChar::LineFeed));
}

void MainWindow::updateGauges(const ccm::core::UsageReport &report)
{
    // 창의 개수가 조회마다 달라질 수 있으므로 게이지를 필요한 수만큼 맞춘다.
    while (m_gauges.size() < report.windows.size()) {
        auto *gauge = new ccm::ui::UsageGauge(this);
        m_gauges.append(gauge);
        m_gaugeLayout->addWidget(gauge);
    }
    while (m_gauges.size() > report.windows.size()) {
        ccm::ui::UsageGauge *gauge = m_gauges.takeLast();
        m_gaugeLayout->removeWidget(gauge);
        gauge->deleteLater();
    }

    for (int i = 0; i < report.windows.size(); ++i) {
        const ccm::core::UsageWindow &window = report.windows.at(i);
        m_gauges.at(i)->setWindow(window, m_service->levelFor(window.key));
    }
}

void MainWindow::updateSummary()
{
    const ccm::core::UsageReport &report = m_service->lastReport();

    if (!report.valid) {
        m_summaryLabel->setText(tr("아직 조회한 값이 없습니다. [지금 조회] 를 누르십시오."));
    } else {
        QStringList lines;
        for (const ccm::core::UsageWindow &window : report.windows) {
            QString line = tr("%1 : %2%")
                               .arg(window.label)
                               .arg(window.usedPercent, 0, 'f', 1);
            if (window.resetsAt.isValid()) {
                line += tr("   재설정 %1").arg(ccm::core::formatDateTime(window.resetsAt));
            }
            lines << line;
        }
        if (!report.subscriptionType.isEmpty()) {
            lines << tr("플랜 : %1").arg(report.subscriptionType);
        }
        m_summaryLabel->setText(lines.join(QChar::LineFeed));
    }

    QStringList status;
    const QDateTime updated = m_service->lastUpdatedAt();
    if (updated.isValid()) {
        const qint64 age = updated.secsTo(QDateTime::currentDateTimeUtc());
        status << tr("마지막 업데이트 : %1 (%2초 전)")
                      .arg(ccm::core::formatDateTimeWithSeconds(updated))
                      .arg(age);
    }
    const int interval = m_service->pollIntervalMinutes();
    status << (interval > 0 ? tr("자동 조회 : %1분 간격").arg(interval)
                            : tr("자동 조회 : 끔"));
    if (!m_service->lastError().isEmpty()) {
        status << tr("최근 조회 실패 : %1").arg(m_service->lastError());
    }
    m_updatedLabel->setText(status.join(QChar::LineFeed));
}

void MainWindow::openSettings()
{
    qCDebug(ccmApp) << "설정 창을 엽니다.";
    // 임계치를 붙일 지표는 실제로 조회된 창에서 가져온다. "Fable (주간)" 처럼
    // 서버가 준 이름이 화면에 보여야 하기 때문이다.
    QList<ccm::ui::ThresholdTarget> targets;
    for (const ccm::core::UsageWindow &window : m_service->lastReport().windows) {
        targets.append(ccm::ui::ThresholdTarget{window.key, window.label, window.kind});
    }

    ccm::ui::SettingsDialog dialog(targets, this);
    connect(&dialog, &ccm::ui::SettingsDialog::settingsSaved, this, [this] {
        m_service->setPollIntervalMinutes(ccm::infra::Settings::pollIntervalMinutes());
        m_service->reloadThresholds();
        updateGauges(m_service->lastReport());
        refreshChart();
        // 설정 창 안에서 훅을 등록했을 수 있다.
        refreshHookState();
        updateHealth();
        updateSummary();
    });
    dialog.exec();

    // 저장하지 않고 훅만 등록하거나 해제했을 수 있다. 창을 닫을 때 한 번 더 본다.
    refreshHookState();
    updateHealth();
}

void MainWindow::openAlertLog()
{
    if (!m_alertLog) {
        // 알림이 추가되거나 확인될 때 목록이 갱신되어야 하므로 창을 살려 둔다.
        m_alertLog = new ccm::ui::AlertLogDialog(m_service, this);
    }
    m_alertLog->reload();
    m_alertLog->show();
    m_alertLog->raise();
    m_alertLog->activateWindow();
}

void MainWindow::registerStatuslineHook()
{
    // 주고받는 절차는 ui/HookSetup 에 있다. 설정 창도 같은 것을 부른다.
    QString summary;
    ccm::ui::runHookRegistration(this, &summary);
    refreshHookState();
    updateHealth();
    statusBar()->showMessage(summary, 8000);
}
