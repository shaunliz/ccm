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
//  infra/AlertStore.h
//
//  알림 목록. 윈도우 알림 팝업은 몇 초 뒤 사라지고 자리를 비운 동안 뜬 것은
//  아예 보지 못하므로, 발생한 알림을 파일에 누적한다.
//
//  확인 여부를 함께 담는다
//   사용자가 본 것과 아직 안 본 것을 구분해야 하므로 항목마다 acknowledged 를
//   둔다. 지우는 것과 확인하는 것은 다른 동작이다. 확인은 "봤다" 이고 목록에
//   남으며, 삭제는 목록에서 없앤다.
//
//  파일 형식
//   { "schemaVersion": 3, "alerts": [ { ... AlertEvent ... } ] }
//
//   수준은 AlertLevel 의 정수값으로 담는다. 알림 수준이 셋으로 늘면서 그 값이
//   한 칸씩 밀렸으므로(경고 1 -> 2, 위험 2 -> 3), 버전 2 로 적힌 파일은 읽을 때
//   옮겨 준다. 그러지 않으면 예전 "경고" 가 "알림" 으로 보인다.
//
//  상한을 두는 이유
//   알림은 임계치를 넘는 순간에만 생기므로 폭증하지 않지만, 몇 달을 켜 두면
//   쌓인다. 오래된 것부터 버려 파일 크기를 묶어 둔다. 다만 아직 확인하지 않은
//   항목은 남겨 둔다. 보지 못한 알림을 조용히 버리면 알림의 뜻이 없다.
// ============================================================================
#ifndef CCM_INFRA_ALERTSTORE_H
#define CCM_INFRA_ALERTSTORE_H

#include "core/AlertTypes.h"

#include <QList>
#include <QString>

namespace ccm::infra {

class AlertStore
{
public:
    /// filePath 를 비우면 ClaudePaths::alertFilePath() 를 쓴다.
    explicit AlertStore(const QString &filePath = QString());

    void load();

    /// 사건들을 더하고 곧바로 기록한다. 새 항목은 미확인 상태로 들어간다.
    bool append(const QList<ccm::core::AlertEvent> &events, QString *error = nullptr);

    /// 최근 것부터 정렬된 목록.
    const QList<ccm::core::AlertEvent> &alerts() const { return m_alerts; }

    int unacknowledgedCount() const;

    /// 하나를 확인 처리한다. 없는 id 면 false.
    bool acknowledge(const QString &id, QString *error = nullptr);

    /// 전부 확인 처리한다. 바뀐 것이 없으면 기록도 하지 않는다.
    bool acknowledgeAll(QString *error = nullptr);

    /// 하나를 지운다. 없는 id 면 false.
    bool remove(const QString &id, QString *error = nullptr);

    /// 여러 개를 한 번에 지운다. 지운 개수를 돌려준다.
    int removeMany(const QList<QString> &ids, QString *error = nullptr);

    /// 목록을 모두 지운다.
    bool clear(QString *error = nullptr);

    /// 보관 상한.
    static int capacity();

    QString filePath() const { return m_filePath; }

private:
    bool save(QString *error);
    void sortNewestFirst();
    void trim();

    QString m_filePath;
    QList<ccm::core::AlertEvent> m_alerts;   ///< 최근 것이 앞.
};

} // namespace ccm::infra

#endif // CCM_INFRA_ALERTSTORE_H
