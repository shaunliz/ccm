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

#include "ui/SettingsDialog.h"

#include "infra/AutoStart.h"
#include "infra/HookRegistrar.h"
#include "infra/Logger.h"
#include "infra/Settings.h"
#include "ui/AlertVisuals.h"
#include "ui/HookSetup.h"
#include "ui/StandardButtons.h"
#include "ui/UiColors.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QTabWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace ccm::ui {
namespace {

constexpr int kMinPollMinutes = 1;
constexpr int kMaxPollMinutes = 720;

/// 확인란 칸에 네모 좌우로 남길 여백(px).
constexpr int kChartColumnPadding = 18;

/// 칸 안에서 네모 좌우에 둘 여백.
constexpr int kChartCellMargin = 6;

enum RuleColumn {
    ColumnAlert = 0,   ///< 이 줄로 알릴지. 개수 제한 없음
    ColumnChart,       ///< 차트에 그릴 줄. 최대 세 개
    ColumnTarget,
    ColumnPercent,
    ColumnLevel,
    RuleColumnCount
};

/// 확인란을 칸 가운데에 놓기 위한 껍데기.
///
/// QTableWidget 의 칸 위젯은 칸을 가득 채우므로, 확인란을 그냥 넣으면 왼쪽에
/// 붙고 글자 없는 네모만 치우쳐 보인다.
QWidget *centered(QWidget *inner, QWidget *parent)
{
    auto *holder = new QWidget(parent);
    auto *layout = new QHBoxLayout(holder);
    // 여백을 0 으로 두면 네모가 칸 가장자리에 닿아 테두리가 잘려 보인다.
    layout->setContentsMargins(kChartCellMargin, 0, kChartCellMargin, 0);
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(inner);
    return holder;
}

/// 확인란 칸의 고정 폭. 앞의 두 열이 함께 쓴다.
///
/// 내용에 맞춰 늘리는 방식으로는 폭이 안정되지 않아 고정으로 둔다.
///
/// 폭을 정하는 값 두 가지
///  1. 네모가 스스로 말하는 권장 폭. PM_IndicatorWidth 는 네모 한 변만 주므로
///     그것만 쓰면 테두리와 초점 표시 자리가 빠져 오른쪽이 잘린다.
///     QCheckBox 의 sizeHint 는 스타일이 그 여백까지 더해 준다.
///  2. 머리글 글자 폭. 글자가 잘리면 무슨 칸인지 알 수 없다.
///
/// 둘 중 넓은 쪽에 여백을 더한다.
int checkColumnWidth(const QWidget *widget, const QString &headerText)
{
    // 재기만 할 것이므로 화면에 붙이지 않는다. sizeHint 는 띄우지 않아도 나온다.
    QCheckBox probe;
    probe.setStyle(widget->style());
    const int indicator = probe.sizeHint().width();

    const int header = widget->fontMetrics().horizontalAdvance(headerText);
    return qMax(indicator, header) + kChartColumnPadding;
}

QLabel *makeHintLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    applyHintTextColor(label);
    return label;
}

/// 조회 전이라도 고를 수 있어야 하는 지표. 이 둘은 계정과 무관하게 항상 있다.
QList<ThresholdTarget> fallbackTargets()
{
    return QList<ThresholdTarget>{
        ThresholdTarget{QStringLiteral("five_hour"),
                        QCoreApplication::translate("ccm", "현재 세션"),
                        ccm::core::UsageWindowKind::Session},
        ThresholdTarget{QStringLiteral("seven_day"),
                        QCoreApplication::translate("ccm", "모든 모델 (주간)"),
                        ccm::core::UsageWindowKind::WeeklyAll},
    };
}

} // namespace

SettingsDialog::SettingsDialog(const QList<ThresholdTarget> &targets, QWidget *parent)
    : QDialog(parent)
    , m_targets(targets.isEmpty() ? fallbackTargets() : targets)
{
    setWindowTitle(tr("설정"));
    setMinimumWidth(560);

    auto *layout = new QVBoxLayout(this);

    // 설정 항목이 늘어 한 화면에 세로로 늘어놓으면 창이 화면 높이를 넘는다.
    // 성격으로 갈라 탭에 담는다. "언제 알릴 것인가" 와 "어떻게 동작할 것인가".
    //
    // 설정 파일 안내와 [저장]/[취소] 는 탭 밖에 둔다. 어느 탭을 보고 있든
    // 저장은 두 탭의 내용을 함께 저장하므로, 한쪽 탭에 속한 것처럼 보이면
    // 그 탭만 저장되는 것으로 읽힌다.
    auto *tabs = new QTabWidget(this);

    auto *alertTab = new QWidget(tabs);
    auto *alertLayout = new QVBoxLayout(alertTab);

    auto *environmentTab = new QWidget(tabs);
    auto *environmentLayout = new QVBoxLayout(environmentTab);

    // --- 알림 표시 ---
    auto *alertGroup = new QGroupBox(tr("알림 표시"), alertTab);
    auto *noticeLayout = new QVBoxLayout(alertGroup);

    m_notifications = new QCheckBox(tr("윈도우 알림 띄우기"), alertGroup);
    noticeLayout->addWidget(m_notifications);
    noticeLayout->addWidget(makeHintLabel(
        tr("끄더라도 알림 목록에는 계속 쌓입니다. 같은 임계치는 재설정 전까지 "
           "한 번만 알립니다."),
        alertGroup));
    alertLayout->addWidget(alertGroup);

    // --- 임계치 ---
    auto *thresholdGroup = new QGroupBox(tr("알림 임계치"), alertTab);
    auto *thresholdLayout = new QVBoxLayout(thresholdGroup);
    buildThresholdTable(thresholdGroup, thresholdLayout);
    alertLayout->addWidget(thresholdGroup, 1);

    // --- 조회 간격 ---
    auto *pollGroup = new QGroupBox(tr("조회 간격"), environmentTab);
    auto *pollForm = new QFormLayout(pollGroup);

    m_pollInterval = new QSpinBox(pollGroup);
    m_pollInterval->setRange(0, kMaxPollMinutes);
    m_pollInterval->setSuffix(tr(" 분"));
    m_pollInterval->setSpecialValueText(tr("자동 조회 끔"));
    pollForm->addRow(tr("자동 조회 간격"), m_pollInterval);
    pollForm->addRow(QString(),
                     makeHintLabel(tr("조회 1회에 claude 세션이 하나 뜨고 1~2초가 "
                                       "걸립니다. 토큰 비용은 들지 않습니다. "
                                       "최소 %1분입니다.")
                                        .arg(kMinPollMinutes),
                                    pollGroup));
    environmentLayout->addWidget(pollGroup);

    // --- 시작과 종료 ---
    auto *startupGroup = new QGroupBox(tr("시작과 종료"), environmentTab);
    auto *startupLayout = new QVBoxLayout(startupGroup);

    m_autoStart = new QCheckBox(tr("로그인 시 자동 실행"), startupGroup);
    // 구현이 없는 운영체제에서는 누를 수 없게 한다. 눌렀는데 아무 일도
    // 일어나지 않는 것이 제일 나쁜 결과다. (infra/AutoStart.h)
    m_autoStart->setEnabled(ccm::infra::AutoStart::isSupported());
    startupLayout->addWidget(m_autoStart);

    m_startMinimized = new QCheckBox(tr("시작할 때 창을 띄우지 않고 트레이에서 실행"),
                                     startupGroup);
    startupLayout->addWidget(m_startMinimized);

    m_askOnClose = new QCheckBox(tr("창을 닫을 때 어떻게 할지 물어보기"), startupGroup);
    startupLayout->addWidget(m_askOnClose);

    // 끈 상태에서 무슨 일이 일어나는지 적어 둔다. 끄기만 보여 주면 닫기를 눌러
    // 봐야 알 수 있다.
    m_closeActionHint = makeHintLabel(QString(), startupGroup);
    startupLayout->addWidget(m_closeActionHint);
    connect(m_askOnClose, &QCheckBox::toggled, this, &SettingsDialog::updateCloseActionHint);

    // 이 아래 두 줄은 "무엇이 등록되는가" 를 보여 주는 정보다. 따로 실행할
    // 명령이 아니다. 처음에는 경로만 적어 두었더니 실행해야 하는 명령처럼
    // 읽혔으므로, 상태와 안내를 함께 붙인다.
    m_autoStartState = makeHintLabel(QString(), startupGroup);
    startupLayout->addWidget(m_autoStartState);

    m_autoStartCommand = makeHintLabel(QString(), startupGroup);
    startupLayout->addWidget(m_autoStartCommand);

    startupLayout->addWidget(makeHintLabel(
        tr("위 확인란을 켜고 [저장] 을 누르면 이 명령이 로그인 시 자동으로 실행되도록 "
           "등록됩니다. 직접 실행할 명령이 아닙니다."),
        startupGroup));
    environmentLayout->addWidget(startupGroup);

    // --- statusline 훅 ---
    //
    // 자동 조회와 같은 탭에 둔다. 둘 다 "사용량을 어디서 얻는가" 에 대한
    // 설정이다. (자동 조회는 우리가 claude 를 부르는 것, 훅은 Claude Code 가
    //  우리를 부르는 것이다) 한 번 등록하면 다시 열 일이 없으므로 맨 아래다.
    auto *hookGroup = new QGroupBox(tr("statusline 훅"), environmentTab);
    auto *hookLayout = new QVBoxLayout(hookGroup);

    hookLayout->addWidget(makeHintLabel(
        tr("Claude Code 터미널 아래 상태줄에 사용률을 표시합니다. "
           "이 프로그램의 화면과는 별개이며, 해제해도 조회와 알림은 그대로 "
           "동작합니다."),
        hookGroup));

    auto *hookRow = new QHBoxLayout();
    auto *hookButton = new QPushButton(tr("훅 등록"), hookGroup);
    connect(hookButton, &QPushButton::clicked, this, [this] {
        runHookRegistration(this);
        // 등록 여부가 바뀌었을 수 있다. 화면의 상태 줄을 맞춘다.
        updateHookInfo();
    });
    hookRow->addWidget(hookButton);

    m_hookRemoveButton = new QPushButton(tr("훅 해제"), hookGroup);
    connect(m_hookRemoveButton, &QPushButton::clicked, this, [this] {
        runHookRemoval(this);
        updateHookInfo();
    });
    hookRow->addWidget(m_hookRemoveButton);

    m_hookState = makeHintLabel(QString(), hookGroup);
    hookRow->addWidget(m_hookState, 1);
    hookLayout->addLayout(hookRow);

    environmentLayout->addWidget(hookGroup);

    // 항목이 적을 때 위쪽으로 몰아 둔다. 두지 않으면 묶음들이 세로로 벌어진다.
    environmentLayout->addStretch();

    tabs->addTab(alertTab, tr("알림 설정"));
    tabs->addTab(environmentTab, tr("환경 설정"));
    layout->addWidget(tabs, 1);

    m_settingsPath = makeHintLabel(QString(), this);
    layout->addWidget(m_settingsPath);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        save();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    localizeButtons(buttons);
    layout->addWidget(buttons);

    load();
}

void SettingsDialog::buildThresholdTable(QWidget *parent, QVBoxLayout *layout)
{
    // 확인란을 한데 묶는다. 배타가 아니다. 여러 줄을 켤 수 있어야 한다.
    // 묶는 이유는 켜짐이 바뀔 때를 한 곳에서 받기 위해서다. 표의 줄이 사라지면
    // 그 확인란도 함께 사라지고, 묶음에서 빼는 일은 QButtonGroup 이 알아서 한다.
    m_chartChoice = new QButtonGroup(this);
    m_chartChoice->setExclusive(false);
    connect(m_chartChoice, &QButtonGroup::buttonToggled,
            this, [this](QAbstractButton *, bool) { updateChartLimit(); });

    m_rules = new QTableWidget(parent);
    m_rules->setColumnCount(RuleColumnCount);
    const QString alertHeaderText = tr("알림 활성");
    const QString chartHeaderText = tr("차트 표시");
    m_rules->setHorizontalHeaderLabels(
        {alertHeaderText, chartHeaderText, tr("지표"), tr("사용률"), tr("수준")});
    if (QTableWidgetItem *alertHeader = m_rules->horizontalHeaderItem(ColumnAlert)) {
        alertHeader->setToolTip(
            tr("이 줄로 알릴지 정합니다. 개수 제한은 없습니다. "
               "꺼도 차트에는 그대로 그릴 수 있습니다."));
    }
    if (QTableWidgetItem *chartHeader = m_rules->horizontalHeaderItem(ColumnChart)) {
        chartHeader->setToolTip(
            tr("사용률 추이 차트에 점선으로 표시할 임계치를 고릅니다. 최대 %1개. "
               "알림을 켜고 끄는 것과는 상관이 없습니다.")
                .arg(ccm::infra::Settings::maxChartThresholds()));
    }
    m_rules->verticalHeader()->setVisible(false);
    m_rules->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rules->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 지표 칸에 들어갈 글자가 길지 않다. 필요한 만큼만 주고 남는 폭은 마지막
    // 칸이 가져가게 한다. 지표를 늘려 두면 빈 자리만 넓어진다.
    QHeaderView *header = m_rules->horizontalHeader();
    header->setSectionResizeMode(ColumnAlert, QHeaderView::Fixed);
    header->resizeSection(ColumnAlert, checkColumnWidth(m_rules, alertHeaderText));
    header->setSectionResizeMode(ColumnChart, QHeaderView::Fixed);
    header->resizeSection(ColumnChart, checkColumnWidth(m_rules, chartHeaderText));
    header->setSectionResizeMode(ColumnTarget, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ColumnPercent, QHeaderView::ResizeToContents);
    header->setStretchLastSection(true);

    m_rules->setMinimumHeight(160);
    layout->addWidget(m_rules);

    auto *buttonRow = new QHBoxLayout();

    auto *addButton = new QPushButton(tr("알림 추가"), parent);
    connect(addButton, &QPushButton::clicked, this, [this] {
        // 첫 지표에 기본 경고 한 줄을 얹어 준다. 빈 줄을 주고 값을 채우게
        // 하는 것보다 고치기 쉽다.
        addRuleRow(m_targets.first().key,
                   ccm::core::ThresholdRule{70.0, ccm::core::AlertLevel::Notice});
        m_rules->selectRow(m_rules->rowCount() - 1);
        updateChartLimit();
    });
    buttonRow->addWidget(addButton);

    m_removeRuleButton = new QPushButton(tr("선택 알림 삭제"), parent);
    connect(m_removeRuleButton, &QPushButton::clicked,
            this, &SettingsDialog::removeSelectedRules);
    buttonRow->addWidget(m_removeRuleButton);

    auto *defaultButton = new QPushButton(tr("기본값으로"), parent);
    connect(defaultButton, &QPushButton::clicked, this, [this] {
        if (askYesNo(this, tr("알림 임계치"),
                     tr("지금 표를 버리고 지표별 기본값으로 되돌립니다. 계속할까요?"))) {
            restoreDefaultRules();
        }
    });
    buttonRow->addWidget(defaultButton);

    buttonRow->addStretch();
    layout->addLayout(buttonRow);

    layout->addWidget(makeHintLabel(
        tr("지표 하나에 여러 줄을 둘 수 있습니다. 예: 현재 세션 70% 알림, 85% 경고, "
           "95% 위험.\n"
           "한 지표의 줄을 모두 지우면 그 지표는 알리지 않습니다. 표에 없는 지표"
           "(나중에 생긴 모델 등)는 종류별 기본값으로 동작합니다.\n"
           "[알림 활성] 과 [차트 표시] 는 서로 상관이 없습니다. 알림을 꺼도 "
           "차트에는 그릴 수 있고, 그 반대도 됩니다."),
        parent));

    // 몇 개까지 고를 수 있고 지금 몇 개인지는 확인란을 누를 때마다 바뀐다.
    // 잠긴 확인란을 보고 고장인가 싶지 않도록 숫자를 함께 보여 준다.
    m_chartHint = makeHintLabel(QString(), parent);
    layout->addWidget(m_chartHint);
}

void SettingsDialog::addRuleRow(const QString &targetKey, const ccm::core::ThresholdRule &rule)
{
    const int row = m_rules->rowCount();
    m_rules->insertRow(row);

    // 알림 확인란은 묶음에 넣지 않는다. 개수 제한이 없으므로 켜짐이 바뀔 때
    // 함께 살펴볼 일이 없다.
    auto *alert = new QCheckBox(m_rules);
    alert->setChecked(rule.enabled);
    alert->setToolTip(tr("이 임계치를 넘으면 알립니다."));
    m_rules->setCellWidget(row, ColumnAlert, centered(alert, m_rules));

    auto *chart = new QCheckBox(m_rules);
    chart->setToolTip(tr("이 임계치를 사용률 추이 차트에 점선으로 표시합니다."));
    // 번호를 주지 않는다. 줄을 지우면 뒤 줄의 번호가 밀려 맞지 않게 된다.
    // 어느 줄인지는 칸 위젯을 훑어 알아낸다. (chartRows)
    m_chartChoice->addButton(chart);
    m_rules->setCellWidget(row, ColumnChart, centered(chart, m_rules));

    auto *target = new QComboBox(m_rules);
    for (const ThresholdTarget &item : m_targets) {
        target->addItem(item.label, item.key);
    }
    const int index = target->findData(targetKey);
    target->setCurrentIndex(index >= 0 ? index : 0);
    m_rules->setCellWidget(row, ColumnTarget, target);

    auto *percent = new QDoubleSpinBox(m_rules);
    percent->setRange(0.0, 100.0);
    percent->setDecimals(0);
    percent->setSuffix(QStringLiteral(" %"));
    percent->setValue(rule.percent);
    m_rules->setCellWidget(row, ColumnPercent, percent);

    auto *level = new QComboBox(m_rules);
    const QList<ccm::core::AlertLevel> levels = ccm::core::selectableAlertLevels();
    for (ccm::core::AlertLevel item : levels) {
        level->addItem(levelIcon(item), ccm::core::alertLevelName(item),
                       static_cast<int>(item));
    }
    const int levelIndex = level->findData(static_cast<int>(rule.level));
    level->setCurrentIndex(levelIndex >= 0 ? levelIndex : 0);
    m_rules->setCellWidget(row, ColumnLevel, level);

}

namespace {

/// 줄의 확인란 하나. 칸 위젯을 아직 만들지 않았으면 nullptr.
QCheckBox *boxAt(const QTableWidget *table, int row, RuleColumn column)
{
    QWidget *holder = table->cellWidget(row, column);
    return holder ? holder->findChild<QCheckBox *>() : nullptr;
}

QCheckBox *chartBoxAt(const QTableWidget *table, int row)
{
    return boxAt(table, row, ColumnChart);
}

QCheckBox *alertBoxAt(const QTableWidget *table, int row)
{
    return boxAt(table, row, ColumnAlert);
}

} // namespace

QList<int> SettingsDialog::chartRows() const
{
    QList<int> rows;
    for (int row = 0; row < m_rules->rowCount(); ++row) {
        if (QCheckBox *box = chartBoxAt(m_rules, row)) {
            if (box->isChecked()) {
                rows.append(row);
            }
        }
    }
    return rows;
}

void SettingsDialog::setChartRowChecked(int row, bool checked)
{
    if (row < 0 || row >= m_rules->rowCount()) {
        return;
    }
    if (QCheckBox *box = chartBoxAt(m_rules, row)) {
        // 잠겨 있어도 프로그램이 켜는 것은 막지 않는다. 잠금은 사람이 네 번째를
        // 켜지 못하게 하려는 것이지, 불러온 값을 막으려는 것이 아니다.
        box->setChecked(checked);
    }
}

void SettingsDialog::updateChartLimit()
{
    const int maximum = ccm::infra::Settings::maxChartThresholds();
    const int chosen = chartRows().size();
    const bool full = chosen >= maximum;

    // 다 찼으면 꺼져 있는 것들을 잠근다. 잠그지 않으면 네 번째를 눌렀을 때
    // 아무 일도 일어나지 않는 것처럼 보인다.
    for (int row = 0; row < m_rules->rowCount(); ++row) {
        if (QCheckBox *box = chartBoxAt(m_rules, row)) {
            box->setEnabled(box->isChecked() || !full);
        }
    }

    if (m_chartHint) {
        m_chartHint->setText(
            tr("[차트 표시] 로 고른 임계치가 사용률 추이 차트에 점선으로 표시됩니다. "
               "최대 %1개까지, 지금 %2개를 골랐습니다.")
                .arg(maximum)
                .arg(chosen));
    }
}

void SettingsDialog::removeSelectedRules()
{
    // 뒤에서부터 지운다. 앞에서 지우면 남은 줄 번호가 밀린다.
    QList<int> rows;
    const QList<QModelIndex> selected = m_rules->selectionModel()->selectedRows();
    for (const QModelIndex &index : selected) {
        rows.append(index.row());
    }
    if (rows.isEmpty()) {
        return;
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) {
        m_rules->removeRow(row);
    }
    updateChartLimit();
}

void SettingsDialog::restoreDefaultRules()
{
    m_rules->setRowCount(0);
    for (const ThresholdTarget &target : m_targets) {
        const ccm::core::ThresholdRules rules =
            ccm::core::defaultThresholdRules(target.kind);
        for (const ccm::core::ThresholdRule &rule : rules) {
            addRuleRow(target.key, rule);
        }
    }
    setChartRowChecked(0, true);
    updateChartLimit();
}

void SettingsDialog::load()
{
    m_pollInterval->setValue(ccm::infra::Settings::pollIntervalMinutes());
    m_notifications->setChecked(ccm::infra::Settings::notificationsEnabled());
    m_startMinimized->setChecked(ccm::infra::Settings::startMinimized());
    m_askOnClose->setChecked(ccm::infra::Settings::askOnClose());
    m_autoStart->setChecked(ccm::infra::AutoStart::isEnabled());
    updateCloseActionHint();

    const ccm::core::ThresholdConfig config = ccm::infra::Settings::thresholds();

    m_rules->setRowCount(0);
    for (const ThresholdTarget &target : m_targets) {
        // 설정된 것이 있으면 그것을, 없으면 종류 기본값을 보여 준다. 화면이
        // 실제로 적용되는 값을 보여야 한다.
        ccm::core::ThresholdRules rules = config.configuredRules(target.key);
        const bool configured = config.configuredKeys().contains(target.key);
        if (!configured) {
            rules = ccm::core::defaultThresholdRules(target.kind);
        }
        for (const ccm::core::ThresholdRule &rule : rules) {
            addRuleRow(target.key, rule);
        }
    }

    // 차트에 그릴 줄들을 켠다.
    const ccm::infra::ChartThresholds chartThresholds =
        ccm::infra::Settings::chartThresholds();
    for (int row = 0; row < m_rules->rowCount(); ++row) {
        auto *target = qobject_cast<QComboBox *>(m_rules->cellWidget(row, ColumnTarget));
        auto *percent =
            qobject_cast<QDoubleSpinBox *>(m_rules->cellWidget(row, ColumnPercent));
        if (!target || !percent) {
            continue;
        }
        const ccm::infra::ChartThreshold here{target->currentData().toString(),
                                              percent->value()};
        if (chartThresholds.contains(here)) {
            setChartRowChecked(row, true);
        }
    }

    // 아직 한 번도 고른 적이 없으면 맨 위 줄을 켜 둔다. 저장된 것이 "하나도
    // 그리지 말라" 인 경우와 구별해야 하므로 목록이 비었는지가 아니라
    // 저장된 적이 있는지를 본다.
    if (!ccm::infra::Settings::chartThresholdsConfigured()) {
        setChartRowChecked(0, true);
    }
    updateChartLimit();

    m_settingsPath->setText(tr("설정 파일: %1")
                                .arg(QDir::toNativeSeparators(
                                    ccm::infra::Settings::filePath())));
    updateAutoStartInfo();
    updateHookInfo();
}

void SettingsDialog::updateCloseActionHint()
{
    if (m_askOnClose->isChecked()) {
        m_closeActionHint->setText(
            tr("닫기(X)를 누르면 백그라운드 유지 / 완전 종료 / 취소를 고르는 창이 뜹니다."));
        return;
    }

    // 끈 상태다. 마지막에 고른 동작을 그대로 쓴다.
    m_closeActionHint->setText(
        ccm::infra::Settings::quitOnClose()
            ? tr("묻지 않습니다. 닫기(X)를 누르면 바로 완전 종료합니다.")
            : tr("묻지 않습니다. 닫기(X)를 누르면 트레이로 내려가 계속 동작합니다."));
}

void SettingsDialog::updateAutoStartInfo()
{
    const QString expected = ccm::infra::AutoStart::commandForCurrentExecutable();
    const QString registered = ccm::infra::AutoStart::registeredCommand().trimmed();

    if (registered.isEmpty()) {
        m_autoStartState->setText(tr("현재 상태: 등록되어 있지 않습니다."));
    } else if (registered == expected) {
        m_autoStartState->setText(tr("현재 상태: 등록되어 있습니다."));
    } else {
        // 설치 위치를 옮기거나 다른 빌드로 등록해 둔 경우다. 그대로 두면 없는
        // 파일을 가리키는 항목이 시작 앱 목록에 남으므로 사실을 알린다.
        m_autoStartState->setText(
            tr("현재 상태: 다른 실행 파일이 등록되어 있습니다."
               "\n등록된 명령: %1")
                .arg(registered));
    }

    if (!ccm::infra::AutoStart::isSupported()) {
        m_autoStartState->setText(
            tr("현재 상태: 이 운영체제에서는 아직 지원하지 않습니다."));
    }

    m_autoStartCommand->setText(tr("등록되는 명령: %1").arg(expected));
}

void SettingsDialog::updateHookInfo()
{
    using State = ccm::infra::HookRegistrar::State;

    QString registered;
    const State state = ccm::infra::HookRegistrar::state(&registered);

    switch (state) {
    case State::ThisApp:
        m_hookState->setText(tr("등록되어 있습니다."));
        break;
    case State::OtherCopy:
        // 우리 프로그램이긴 한데 다른 자리의 것이다. 해제할 때 한 번 더 묻는다.
        m_hookState->setText(
            tr("이 프로그램의 다른 복사본이 등록되어 있습니다: %1").arg(registered));
        break;
    case State::Other:
        // 다른 프로그램이 상태줄을 쓰고 있다. 등록을 누르면 백업한 뒤 교체할지
        // 물어본다. 누르기 전에 무엇이 걸려 있는지 보여 준다.
        m_hookState->setText(tr("다른 명령이 등록되어 있습니다: %1").arg(registered));
        break;
    case State::NotRegistered:
        m_hookState->setText(tr("등록되어 있지 않습니다."));
        break;
    }

    // 지울 것이 없으면 해제를 눌러 봐야 "없습니다" 만 나온다. 미리 잠근다.
    // 남의 것일 때는 잠그지 않는다. 눌러 보고 "우리 것만 지운다" 는 사실을
    // 알 수 있어야 하기 때문이다.
    m_hookRemoveButton->setEnabled(state != State::NotRegistered);
}

void SettingsDialog::save()
{
    // 무엇을 어떻게 바꿨는지 한 줄씩 남긴다. 설정을 건드린 뒤에 동작이 달라졌을
    // 때, 무엇을 바꿨는지 기억에 기대지 않고 콘솔에서 확인할 수 있어야 한다.
    qCDebug(ccmApp) << "설정을 저장합니다.";
    qCDebug(ccmApp) << "  자동 조회 간격:" << m_pollInterval->value() << "분 (이전"
                    << ccm::infra::Settings::pollIntervalMinutes() << "분)";
    qCDebug(ccmApp) << "  윈도우 알림:" << m_notifications->isChecked();
    qCDebug(ccmApp) << "  트레이에서 시작:" << m_startMinimized->isChecked();
    qCDebug(ccmApp) << "  닫을 때 묻기:" << m_askOnClose->isChecked();
    qCDebug(ccmApp) << "  로그인 시 자동 실행:" << m_autoStart->isChecked() << "(이전"
                    << ccm::infra::AutoStart::isEnabled() << ")";

    ccm::infra::Settings::setPollIntervalMinutes(m_pollInterval->value());
    ccm::infra::Settings::setNotificationsEnabled(m_notifications->isChecked());
    ccm::infra::Settings::setStartMinimized(m_startMinimized->isChecked());
    ccm::infra::Settings::setAskOnClose(m_askOnClose->isChecked());

    // 표에 보이는 모든 지표를 명시적으로 저장한다. 줄이 하나도 없는 지표는
    // 빈 목록으로 저장해야 "알리지 않음" 이 되고, 기본값으로 되돌아가지 않는다.
    ccm::core::ThresholdConfig config;
    for (const ThresholdTarget &target : m_targets) {
        config.setRules(target.key, ccm::core::ThresholdRules{});
    }

    for (int row = 0; row < m_rules->rowCount(); ++row) {
        auto *target = qobject_cast<QComboBox *>(m_rules->cellWidget(row, ColumnTarget));
        auto *percent = qobject_cast<QDoubleSpinBox *>(
            m_rules->cellWidget(row, ColumnPercent));
        auto *level = qobject_cast<QComboBox *>(m_rules->cellWidget(row, ColumnLevel));
        if (!target || !percent || !level) {
            continue;
        }

        const QString key = target->currentData().toString();
        ccm::core::ThresholdRules rules = config.configuredRules(key);

        ccm::core::ThresholdRule rule;
        rule.percent = percent->value();
        rule.level = static_cast<ccm::core::AlertLevel>(level->currentData().toInt());
        if (QCheckBox *alert = alertBoxAt(m_rules, row)) {
            rule.enabled = alert->isChecked();
        }

        // 같은 지표에 같은 사용률을 두 번 넣어도 알림은 한 번만 날 수 있다.
        // 표식이 (창, 임계치) 로 잡히기 때문이다. 저장 단계에서 걸러 둔다.
        if (!rules.contains(rule)) {
            rules.append(rule);
        }
        config.setRules(key, rules);

        qCDebug(ccmApp).noquote()
            << QStringLiteral("  임계치: %1 %2% %3 (알림 %4)")
                   .arg(key)
                   .arg(rule.percent, 0, 'f', 0)
                   .arg(ccm::core::alertLevelName(rule.level),
                        rule.enabled ? QStringLiteral("켬") : QStringLiteral("끔"));
    }
    ccm::infra::Settings::setThresholds(config);

    // 차트에 그릴 줄들. 표가 비어 있으면 빈 목록이 저장된다.
    ccm::infra::ChartThresholds chartThresholds;
    const QList<int> chosenRows = chartRows();
    for (int row : chosenRows) {
        auto *target = qobject_cast<QComboBox *>(m_rules->cellWidget(row, ColumnTarget));
        auto *percent =
            qobject_cast<QDoubleSpinBox *>(m_rules->cellWidget(row, ColumnPercent));
        if (!target || !percent) {
            continue;
        }
        chartThresholds.append(
            ccm::infra::ChartThreshold{target->currentData().toString(), percent->value()});
    }
    ccm::infra::Settings::setChartThresholds(chartThresholds);
    for (const ccm::infra::ChartThreshold &threshold : chartThresholds) {
        qCDebug(ccmApp).noquote() << QStringLiteral("  차트 표시: %1 %2%")
                                         .arg(threshold.windowKey)
                                         .arg(threshold.percent, 0, 'f', 0);
    }
    if (chartThresholds.isEmpty()) {
        qCDebug(ccmApp) << "  차트 표시: 없음 (점선을 그리지 않습니다)";
    }

    // 시작프로그램만은 레지스트리라 실패할 수 있다. 실패를 삼키지 않는다.
    QString error;
    if (ccm::infra::AutoStart::isEnabled() != m_autoStart->isChecked()) {
        if (ccm::infra::AutoStart::setEnabled(m_autoStart->isChecked(), &error)) {
            qCInfo(ccmApp) << "시작프로그램 등록을 바꿨습니다:"
                           << m_autoStart->isChecked();
        } else {
            qCWarning(ccmApp).noquote() << QStringLiteral("시작프로그램 등록 실패: %1")
                                               .arg(error);
            showWarning(this, tr("설정"), error);
        }
    }

    qCInfo(ccmApp) << "설정을 저장했습니다. 임계치 줄" << m_rules->rowCount() << "개";
    emit settingsSaved();
}

} // namespace ccm::ui
