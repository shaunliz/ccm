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
//  ui/SettingsDialog.h
//
//  폴링 간격, 임계치, 알림, 시작프로그램 등록, statusline 훅을 다룬다.
//
//  탭으로 가르는 이유
//   항목이 늘어 한 화면에 세로로 늘어놓으면 창이 화면 높이를 넘는다. 성격으로
//   둘로 가른다. 알림 설정("언제 알릴 것인가")과 환경 설정("어떻게 동작할
//   것인가"). 설정 파일 안내와 [저장]/[취소] 는 탭 밖이다. 저장은 어느 탭을
//   보고 있든 두 탭을 함께 저장하므로, 탭 안에 있으면 그 탭만 저장되는 것처럼
//   읽힌다.
//
//  임계치를 표 하나로 두는 이유
//   지표마다 규칙이 여러 개(N개)이고 지표 수도 계정에 따라 달라진다. 지표별로
//   칸을 나누면 화면이 길어지고 지표가 늘 때마다 칸을 만들어야 한다. 지표를
//   한 열로 두고 줄을 추가/삭제하는 표가 개수 변화를 그대로 받아 낸다.
//
//  지표 목록은 마지막 조회 결과에서 받는다
//   "Fable (주간)" 처럼 실제 창 이름이 보여야 하고, 그 이름은 서버가 준다.
//   조회 전이라면 언제나 있는 두 지표(현재 세션, 모든 모델)만 고를 수 있다.
// ============================================================================
#ifndef CCM_UI_SETTINGSDIALOG_H
#define CCM_UI_SETTINGSDIALOG_H

#include "core/AlertTypes.h"
#include "core/UsageTypes.h"

#include <QDialog>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QCheckBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace ccm::ui {

/// 임계치를 붙일 수 있는 지표 하나.
struct ThresholdTarget {
    QString key;
    QString label;
    ccm::core::UsageWindowKind kind = ccm::core::UsageWindowKind::WeeklyModel;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /// \param targets 임계치를 붙일 수 있는 지표 목록. 비우면 기본 두 개를 쓴다.
    explicit SettingsDialog(const QList<ThresholdTarget> &targets,
                            QWidget *parent = nullptr);

signals:
    /// 저장이 끝났을 때. 서비스가 폴링 간격과 임계치를 다시 읽는다.
    void settingsSaved();

private:
    void buildThresholdTable(QWidget *parent, QVBoxLayout *layout);
    void addRuleRow(const QString &targetKey, const ccm::core::ThresholdRule &rule);
    void removeSelectedRules();
    void restoreDefaultRules();

    /// 차트에 그릴 줄을 켜고 끈다.
    ///
    /// 한 줄도 켜지 않는 것도 뜻이 있다. "점선을 긋지 말라". 그래서 비었다고
    /// 대신 켜 주지 않는다. 처음 켠 화면에 기준선을 보여 주는 일만 load() 가
    /// "고른 적이 없을 때" 한 번 한다.
    void setChartRowChecked(int row, bool checked);
    QList<int> chartRows() const;

    /// 최대 개수에 닿으면 나머지 확인란을 잠근다. 잠그지 않으면 네 번째를
    /// 눌렀을 때 아무 일도 일어나지 않는 것처럼 보인다.
    void updateChartLimit();

    void load();
    void save();
    void updateAutoStartInfo();
    void updateHookInfo();
    void updateCloseActionHint();

    QList<ThresholdTarget> m_targets;

    QSpinBox *m_pollInterval = nullptr;
    QCheckBox *m_notifications = nullptr;
    QCheckBox *m_startMinimized = nullptr;
    QCheckBox *m_askOnClose = nullptr;
    QLabel *m_closeActionHint = nullptr;
    QCheckBox *m_autoStart = nullptr;
    QLabel *m_autoStartState = nullptr;
    QLabel *m_autoStartCommand = nullptr;
    QLabel *m_settingsPath = nullptr;

    QLabel *m_hookState = nullptr;
    QPushButton *m_hookRemoveButton = nullptr;

    QTableWidget *m_rules = nullptr;
    QPushButton *m_removeRuleButton = nullptr;
    QLabel *m_chartHint = nullptr;

    /// 표의 확인란을 한데 묶는다. 배타가 아니라, 켜짐이 바뀔 때를 한 곳에서
    /// 받기 위한 묶음이다. 줄을 지우면 그 확인란도 묶음에서 함께 빠진다.
    QButtonGroup *m_chartChoice = nullptr;
};

} // namespace ccm::ui

#endif // CCM_UI_SETTINGSDIALOG_H
