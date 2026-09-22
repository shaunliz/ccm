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
//  ui/UsageGauge.h
//
//  한도 창 하나를 가로 막대로 보여 주는 위젯.
//
//  QProgressBar 를 쓰지 않는 이유
//   알림 수준에 따라 색이 바뀌어야 하고, 막대 위에 이름/사용률/재설정 시각을
//   함께 얹어야 한다. QProgressBar 를 스타일시트로 비틀면 수준별 색을 바꿀
//   때마다 스타일시트를 다시 파싱하게 되고, 텍스트는 한 줄밖에 못 넣는다.
//   직접 그리는 편이 짧다.
// ============================================================================
#ifndef CCM_UI_USAGEGAUGE_H
#define CCM_UI_USAGEGAUGE_H

#include "core/AlertTypes.h"
#include "core/UsageTypes.h"
#include "ui/AlertVisuals.h"

#include <QSize>
#include <QString>
#include <QWidget>

namespace ccm::ui {

class UsageGauge : public QWidget
{
    Q_OBJECT

public:
    explicit UsageGauge(QWidget *parent = nullptr);

    void setWindow(const ccm::core::UsageWindow &window, ccm::core::AlertLevel level);

    /// 값을 비운다. 조회 전이나 실패 후에 쓴다.
    void clearValue(const QString &label);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_label;
    QString m_resetText;
    double m_usedPercent = 0.0;
    bool m_hasValue = false;
    ccm::core::AlertLevel m_level = ccm::core::AlertLevel::Normal;
};

} // namespace ccm::ui

#endif // CCM_UI_USAGEGAUGE_H
