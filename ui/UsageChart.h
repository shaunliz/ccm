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
//  ui/UsageChart.h
//
//  사용률 추이 차트. Qt Charts 의 QChartView 를 감싼다.
//
//  자료원은 infra/UsageHistory 다. get_usage 는 "지금 값" 만 주므로 과거는
//  우리가 쌓은 것밖에 없다. 켠 직후에는 점이 하나뿐이라 선이 보이지 않는데,
//  그 상태를 오류로 오해하지 않도록 안내 문구를 함께 띄운다.
//
//  갱신할 때 QChart 를 새로 만드는 이유
//   시리즈가 없는 상태로 차트가 처음 배치되면 축 눈금 글자 자리가 0 에 가깝게
//   잡히고, 뒤늦게 시리즈를 붙여도 다시 계산되지 않는다. 그러면 눈금 숫자가
//   "..." 로 줄어든 채 남는다. (창 크기를 바꾸면 정상으로 돌아오는 것이 근거다)
//   layout()->invalidate() 로는 풀리지 않으므로, 시리즈를 갖춘 차트를 새로
//   만들어 끼운다. 선이 몇 개뿐이라 비용이 문제되지 않는다.
//
//  임계치 선
//   지표마다 임계치가 여러 개이고 값도 서로 다르므로, 전부 그으면 화면이
//   선으로 덮인다. 어느 것을 그릴지는 호출하는 쪽이 정해서 넘긴다.
//   (설정 화면의 확인란으로 고른 것들. 최대 세 개)
//
//   선 색은 그 임계치가 속한 지표와 같은 차례의 원색이다. 수준 색(알림 초록,
//   경고 주황)을 쓰지 않는 이유는, 고른 셋이 모두 "알림" 이면 점선 세 개가 같은
//   초록이 되어 어느 지표의 것인지 알 수 없기 때문이다. 임계치는 어느 지표의
//   것인지를 알아야 뜻이 생긴다.
//
//   꺾은선과 똑같은 색을 쓰지 않는 것은 범례 때문이다. 같은 색 실선과 점선이
//   나란히 놓이면 범례에서 가려내기 어렵다. 자리(파랑 계열)는 지키고 채도만
//   올린다. 이름도 "세션" 과 "세션임계치" 로 갈라 둔다.
// ============================================================================
#ifndef CCM_UI_USAGECHART_H
#define CCM_UI_USAGECHART_H

#include "core/AlertTypes.h"
#include "infra/Settings.h"
#include "infra/UsageHistory.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QChartView;
class QLabel;
QT_END_NAMESPACE

namespace ccm::ui {

class UsageChart : public QWidget
{
    Q_OBJECT

public:
    explicit UsageChart(QWidget *parent = nullptr);

    /// 이력을 받아 다시 그린다.
    /// \param labels     창 키 -> 표시 이름. 이력에는 키만 남으므로 이름을 따로 받는다.
    /// \param thresholds 점선으로 표시할 임계치들. 비우면 점선을 그리지 않는다.
    void refresh(const ccm::infra::UsageHistory &history,
                 const QHash<QString, QString> &labels,
                 const ccm::infra::ChartThresholds &thresholds);

    /// 차트가 보여 줄 구간(시간).
    void setRangeHours(int hours);
    int rangeHours() const { return m_rangeHours; }

private:
    QChartView *m_view = nullptr;
    QLabel *m_emptyLabel = nullptr;
    int m_rangeHours = 24;
};

} // namespace ccm::ui

#endif // CCM_UI_USAGECHART_H
