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
//  infra/Settings.h
//
//  사용자 설정. %LOCALAPPDATA%/ClaudeCodeMonitor/settings.ini 에 담는다.
//
//  레지스트리가 아니라 INI 인 이유
//   설치 관리자가 지우기 쉽고, 문제가 생겼을 때 사용자가 직접 열어 볼 수 있다.
//   시작프로그램 등록만은 OS 가 정한 자리가 있으므로 레지스트리를 쓴다.
//   (infra/AutoStart)
//
//  임계치 저장 형태
//   지표마다 규칙이 여러 개이므로, 값 하나에 목록으로 적는다.
//
//     [threshold]
//     five_hour=70:warn, 85:warn, 95:critical
//
//   중첩 그룹으로 쪼개면 규칙 수가 바뀔 때 낡은 키가 남는다. 목록 하나면
//   덮어쓰는 것으로 끝나고, 사람이 열어 고치기도 쉽다.
//
//   구분자는 QSettings 의 목록 표기(쉼표)를 그대로 쓴다. 직접 문자열을 만들어
//   세미콜론으로 잇는 방법을 먼저 썼다가 규칙이 첫 줄만 읽히는 것을 확인했다.
//   INI 에서 세미콜론은 주석의 시작이라 그 뒤가 잘린다. 목록은 QSettings 가
//   따옴표까지 알아서 붙이므로 이런 함정이 없다.
//
//  알린 것을 기억하는 표식
//   같은 알림을 반복하지 않기 위해 (창, 임계치, 재설정 시각) 을 저장한다.
//   프로그램을 다시 켜도 유지된다. 상세는 core/AlertTypes.h 참조.
// ============================================================================
#ifndef CCM_INFRA_SETTINGS_H
#define CCM_INFRA_SETTINGS_H

#include "core/AlertTypes.h"

#include <QList>
#include <QString>

namespace ccm::infra {

/// 사용률 추이 차트에 점선으로 그릴 임계치 하나를 가리킨다.
///
/// 설정 화면의 임계치 표에서 (지표, 사용률) 두 값으로 한 줄을 짚는다. 줄 번호로
/// 짚지 않는 이유는 줄을 지우거나 더하면 번호가 밀리기 때문이다.
struct ChartThreshold {
    QString windowKey;
    double percent = -1.0;

    bool operator==(const ChartThreshold &other) const
    {
        // 사용률은 화면에서 정수로만 고르므로 미세한 오차만 감안하면 된다.
        return windowKey == other.windowKey && qAbs(percent - other.percent) < 0.05;
    }
};

using ChartThresholds = QList<ChartThreshold>;

class Settings
{
public:
    Settings() = delete;

    /// 폴링 간격(분). 0 이면 자동 조회를 하지 않는다.
    static int pollIntervalMinutes();
    static void setPollIntervalMinutes(int minutes);

    /// 기본값. 조회 1회에 claude 세션이 하나 뜨므로 너무 짧게 두지 않는다.
    static int defaultPollIntervalMinutes();

    static ccm::core::ThresholdConfig thresholds();
    static void setThresholds(const ccm::core::ThresholdConfig &config);

    /// 사용률 추이 차트에 점선으로 그릴 임계치들.
    ///
    /// 임계치가 여러 개라 전부 그으면 화면이 선으로 덮인다. 어느 것을 그릴지
    /// 사용자가 설정 화면의 확인란으로 고른다.
    ///
    /// 아직 고른 적이 없으면 빈 목록이다. 그때는 화면 쪽이 알아서 고른다.
    static ChartThresholds chartThresholds();
    static void setChartThresholds(const ChartThresholds &thresholds);

    /// 사용자가 한 번이라도 골라 저장한 적이 있는가.
    ///
    /// 빈 목록에는 뜻이 둘 있다. "아직 고른 적이 없다" 와 "하나도 그리지 말라".
    /// 앞은 기본값을 보여 주어야 하고 뒤는 그대로 두어야 하므로 구별한다.
    static bool chartThresholdsConfigured();

    /// 한 번에 그릴 수 있는 최대 개수.
    ///
    /// 선이 많아지면 정작 봐야 할 추이선을 덮는다. 지표가 셋(세션 / 모든 모델 /
    /// 모델별)이므로 지표마다 하나씩 고를 수 있는 만큼으로 둔다.
    static constexpr int maxChartThresholds() { return 3; }

    /// 윈도우 알림 팝업을 띄울지. 끄면 알림 목록에만 쌓인다.
    static bool notificationsEnabled();
    static void setNotificationsEnabled(bool enabled);

    /// 시작 시 창을 띄우지 않고 트레이로만 올라갈지.
    static bool startMinimized();
    static void setStartMinimized(bool enabled);

    /// 창을 닫을 때 어떻게 할지 물어볼지.
    static bool askOnClose();
    static void setAskOnClose(bool enabled);

    /// 묻지 않을 때 할 일. true 면 완전 종료, false 면 트레이로 내려간다.
    ///
    /// 물어보기를 끌 때 사용자가 그 자리에서 고른 동작을 여기 적어 둔다.
    /// 끄기만 저장하고 무엇을 할지 적지 않으면, 그 뒤로 창을 닫을 때 무슨 일이
    /// 일어날지 아무도 알 수 없게 된다.
    static bool quitOnClose();
    static void setQuitOnClose(bool enabled);

    /// 첫 실행 여부. 시작프로그램 자동 등록을 한 번만 하기 위한 표시다.
    static bool isFirstRun();
    static void markFirstRunDone();

    /// 이미 알린 것을 기억하는 표식.
    static ccm::core::FiredMarkMap firedMarks();
    static void setFiredMarks(const ccm::core::FiredMarkMap &marks);

    /// 이력을 보관할 기간(시간). 차트가 읽는 범위보다 넉넉하게 둔다.
    static int historyRetentionHours();

    /// 설정 파일 경로. 화면에 보여 주기 위한 것이다.
    static QString filePath();
};

} // namespace ccm::infra

#endif // CCM_INFRA_SETTINGS_H
