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
//  infra/UsageHistory.h
//
//  사용률 표본을 시간순으로 쌓고 읽는다. 추이 차트의 자료원이다.
//
//  왜 저장이 필요한가
//   get_usage 는 "지금 값" 하나만 준다. 과거 값을 주는 경로가 없으므로 추이를
//   그리려면 폴링할 때마다 직접 쌓아야 한다. 프로그램을 껐다 켜도 차트가
//   비지 않도록 파일에 남긴다.
//
//  파일 형식
//   { "schemaVersion": 1,
//     "samples": [ { "at": "ISO8601", "values": { "five_hour": 11.0, ... } } ] }
//
//   창 목록이 늘거나 줄 수 있으므로 표본마다 키-값 묶음으로 담는다. 어떤 표본에
//   없는 키는 그 시점에 그 창이 없었다는 뜻이며, 차트는 선을 끊어 표현한다.
// ============================================================================
#ifndef CCM_INFRA_USAGEHISTORY_H
#define CCM_INFRA_USAGEHISTORY_H

#include "core/UsageTypes.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

namespace ccm::infra {

/// 한 시점의 표본.
struct UsageSample {
    QDateTime at;                      ///< UTC.
    QHash<QString, double> values;     ///< UsageWindow::key -> 사용률(0~100).
};

class UsageHistory
{
public:
    /// filePath 를 비우면 ClaudePaths::historyFilePath() 를 쓴다.
    explicit UsageHistory(const QString &filePath = QString());

    /// 파일을 읽어 메모리에 올린다. 파일이 없으면 빈 상태로 시작한다.
    void load();

    /// 보고를 표본 하나로 더한다. 보관 기간을 넘긴 표본은 버린다.
    /// 기록까지 함께 한다. 폴링 주기가 분 단위라 쓰기 비용이 문제되지 않는다.
    bool append(const ccm::core::UsageReport &report, QString *error = nullptr);

    /// 최근 hours 시간 안의 표본. 오래된 것부터.
    QList<UsageSample> recent(int hours) const;

    /// 보관 중인 전체 표본. 오래된 것부터.
    const QList<UsageSample> &samples() const { return m_samples; }

    /// 표본에 한 번이라도 등장한 창 키. 차트가 선을 몇 개 그릴지 정할 때 쓴다.
    QStringList keysIn(int hours) const;

    QString filePath() const { return m_filePath; }

private:
    bool save(QString *error);
    void prune();

    QString m_filePath;
    QList<UsageSample> m_samples;
};

} // namespace ccm::infra

#endif // CCM_INFRA_USAGEHISTORY_H
