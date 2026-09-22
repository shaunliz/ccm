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

#include "ui/UsageChart.h"

#include "core/TimeFormat.h"
#include "core/WindowLabel.h"
#include "ui/UiColors.h"

#include <QChart>
#include <QChartView>
#include <QColor>
#include <QDateTimeAxis>
#include <QLabel>
#include <QLineSeries>
#include <QList>
#include <QPen>
#include <QStringList>
#include <QValueAxis>
#include <QVBoxLayout>

namespace ccm::ui {
namespace {

constexpr int kChartMinHeight = 240;
constexpr int kValueTickCount = 6;
constexpr int kTimeTickCount = 5;

/// 선 색. 창의 개수가 서버 사정에 따라 늘 수 있으므로 순환해서 쓴다.
///
/// 지표를 구분하는 색이므로 수준 색(levelColor)과는 쓰임이 다르다. 다만 주황과
/// 빨강은 넣지 않는다. 경고 임계치 점선(주황)과 헷갈리고, 선이 빨갛다는 것만으로
/// 위험해 보이기 때문이다.
const QList<QColor> &seriesPalette()
{
    static const QList<QColor> palette{
        QColor(0x2f, 0x81, 0xf7),   // 파랑
        QColor(0x1a, 0x9e, 0x6c),   // 초록
        QColor(0xb4, 0x6b, 0xd6),   // 보라
        QColor(0x2a, 0xa1, 0xb3),   // 청록
        QColor(0xc0, 0x54, 0x8c),   // 자홍
        QColor(0x5b, 0x6b, 0x7c),   // 회청
    };
    return palette;
}

/// 임계치 점선 색. 꺾은선과 같은 차례로 고른다.
///
/// 꺾은선 색을 그대로 쓰지 않는 이유는, 같은 색 실선과 점선이 나란히 있으면
/// 범례에서 둘을 가려내기 어렵기 때문이다. 같은 자리(세션은 파랑 계열)를
/// 지키되 채도를 올려 원색으로 둔다. 지표와의 짝은 유지하면서 점선만 또렷해진다.
const QList<QColor> &thresholdPalette()
{
    static const QList<QColor> palette{
        QColor(0x00, 0x00, 0xff),   // 원색 파랑
        QColor(0x00, 0xa8, 0x00),   // 원색 초록 (순수 초록은 흰 바탕에서 읽히지 않는다)
        QColor(0xe0, 0x00, 0xe0),   // 원색 자홍
        QColor(0x00, 0xb0, 0xb0),   // 원색 청록
        QColor(0xff, 0x66, 0x00),   // 원색 주황
        QColor(0x77, 0x00, 0xdd),   // 원색 보라
    };
    return palette;
}

/// 표시 순서를 고정한다. 세션 -> 주간 전체 -> 나머지(이름순).
/// 차트를 다시 그릴 때마다 색이 바뀌면 눈이 따라가지 못한다.
QStringList orderKeys(const QStringList &keys)
{
    QStringList ordered;
    for (const char *fixed : {"five_hour", "seven_day"}) {
        const QString key = QString::fromLatin1(fixed);
        if (keys.contains(key)) {
            ordered.append(key);
        }
    }
    QStringList rest;
    for (const QString &key : keys) {
        if (!ordered.contains(key)) {
            rest.append(key);
        }
    }
    rest.sort();
    ordered.append(rest);
    return ordered;
}

} // namespace

UsageChart::UsageChart(QWidget *parent)
    : QWidget(parent)
{
    // 처음에는 빈 차트를 끼워 둔다. 첫 갱신에서 통째로 교체된다.
    m_view = new QChartView(new QChart(), this);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(kChartMinHeight);

    m_emptyLabel = new QLabel(tr("아직 표본이 없습니다. 조회하면 쌓입니다."), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    applyHintTextColor(m_emptyLabel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(m_view);
    layout->addWidget(m_emptyLabel);
}

void UsageChart::setRangeHours(int hours)
{
    m_rangeHours = qMax(1, hours);
}

void UsageChart::refresh(const ccm::infra::UsageHistory &history,
                         const QHash<QString, QString> &labels,
                         const ccm::infra::ChartThresholds &thresholds)
{
    const QList<ccm::infra::UsageSample> samples = history.recent(m_rangeHours);
    const QStringList keys = orderKeys(history.keysIn(m_rangeHours));

    // 표본이 하나뿐이면 선이 점 하나라 보이지 않는다. 안내를 남겨 둔다.
    const bool drawable = samples.size() >= 2 && !keys.isEmpty();
    m_emptyLabel->setVisible(!drawable);
    if (samples.isEmpty()) {
        m_emptyLabel->setText(tr("아직 표본이 없습니다. 조회하면 쌓입니다."));
    } else if (!drawable) {
        m_emptyLabel->setText(tr("표본이 %1개뿐입니다. 두 번 이상 조회하면 추이가 그려집니다.")
                                  .arg(samples.size()));
    }

    // --- 차트를 새로 만든다 ---
    // 시리즈를 갖춘 상태로 처음 배치되어야 축 눈금 자리가 제대로 잡힌다.
    // 자세한 사정은 헤더 주석 참조.
    auto *chart = new QChart();
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundRoundness(0);

    auto *timeAxis = new QDateTimeAxis(chart);
    timeAxis->setTickCount(kTimeTickCount);
    chart->addAxis(timeAxis, Qt::AlignBottom);

    auto *valueAxis = new QValueAxis(chart);
    valueAxis->setRange(0.0, 100.0);
    valueAxis->setTickCount(kValueTickCount);
    // 눈금은 숫자만 쓴다. "%" 까지 붙이면 좁은 창에서 글자 자리가 모자라
    // Qt Charts 가 "..." 로 줄여 버린다. 단위는 그룹 제목이 말해 준다.
    valueAxis->setLabelFormat(QStringLiteral("%d"));
    chart->addAxis(valueAxis, Qt::AlignLeft);

    const QList<QColor> &palette = seriesPalette();
    int colorIndex = 0;

    // 임계치 점선이 같은 차례의 색을 쓰도록 지표별 순번을 기억해 둔다.
    QHash<QString, int> orderOf;

    for (const QString &key : keys) {
        auto *series = new QLineSeries(chart);
        // 범례는 이름이 가로로 늘어서는 자리다. 짧은 이름을 쓴다.
        series->setName(ccm::core::shortWindowLabel(key, labels.value(key, key)));

        const QColor color = palette.at(colorIndex % palette.size());
        orderOf.insert(key, colorIndex);

        QPen pen(color);
        pen.setWidth(2);
        series->setPen(pen);
        ++colorIndex;

        for (const ccm::infra::UsageSample &sample : samples) {
            // 그 시점에 없던 창은 점을 찍지 않는다. 0 으로 채우면 사용량이
            // 떨어진 것처럼 보인다.
            const auto it = sample.values.constFind(key);
            if (it == sample.values.constEnd()) {
                continue;
            }
            series->append(static_cast<qreal>(sample.at.toLocalTime().toMSecsSinceEpoch()),
                           it.value());
        }

        chart->addSeries(series);
        series->attachAxis(timeAxis);
        series->attachAxis(valueAxis);
    }

    // --- 임계치 점선 ---
    if (!samples.isEmpty()) {
        const qreal from =
            static_cast<qreal>(samples.first().at.toLocalTime().toMSecsSinceEpoch());
        const qreal to =
            static_cast<qreal>(samples.last().at.toLocalTime().toMSecsSinceEpoch());

        for (const ccm::infra::ChartThreshold &threshold : thresholds) {
            if (threshold.percent < 0.0 || threshold.windowKey.isEmpty()) {
                continue;
            }

            auto *line = new QLineSeries(chart);
            // "세션임계치". 꺾은선 이름(세션)과 글자가 겹치지 않아야 범례에서
            // 둘을 가려낼 수 있다.
            line->setName(tr("%1임계치")
                              .arg(ccm::core::shortWindowLabel(
                                  threshold.windowKey,
                                  labels.value(threshold.windowKey, threshold.windowKey))));

            // 그 지표와 같은 차례의 원색. 지표가 아직 그려지지 않았다면
            // (이력에 없는 창) 눈에 띄지 않는 회색으로 둔다.
            // QWidget:: 를 붙이는 이유는 위의 지역 palette 와 이름이 겹치기 때문이다.
            const QList<QColor> &vivid = thresholdPalette();
            const int order = orderOf.value(threshold.windowKey, -1);
            QPen pen(order >= 0 ? vivid.at(order % vivid.size())
                                : hintTextColor(QWidget::palette()));
            pen.setStyle(Qt::DashLine);
            pen.setWidth(1);
            line->setPen(pen);

            line->append(from, threshold.percent);
            line->append(to, threshold.percent);
            chart->addSeries(line);
            line->attachAxis(timeAxis);
            line->attachAxis(valueAxis);
        }
    }

    // 시간축은 실제 표본 구간에 맞춘다. 구간을 고정하면 켠 직후 선이 화면
    // 오른쪽 끝에 눌려 보인다.
    if (samples.isEmpty()) {
        const QDateTime now = QDateTime::currentDateTime();
        timeAxis->setRange(now.addSecs(-qint64(m_rangeHours) * 3600), now);
        timeAxis->setFormat(ccm::core::axisTimeFormat());
    } else {
        const QDateTime from = samples.first().at.toLocalTime();
        QDateTime to = samples.last().at.toLocalTime();
        if (from == to) {
            to = to.addSecs(60);
        }
        timeAxis->setRange(from, to);
        timeAxis->setFormat(from.date() == to.date() ? ccm::core::axisTimeFormat()
                                                     : ccm::core::axisDateTimeFormat());
    }

    QChart *previous = m_view->chart();
    m_view->setChart(chart);
    delete previous;
}

} // namespace ccm::ui
