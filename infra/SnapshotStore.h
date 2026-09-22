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
//  infra/SnapshotStore.h
//
//  스냅샷 파일의 기록과 읽기를 담당한다.
//
//  기록은 QSaveFile 을 사용해 원자적으로 교체한다.
//  위젯이 파일을 읽는 구조이므로, 부분 기록된 파일을 읽는 상황을
//  구조적으로 차단해야 한다.
// ============================================================================
#ifndef CCM_INFRA_SNAPSHOTSTORE_H
#define CCM_INFRA_SNAPSHOTSTORE_H

#include <QByteArray>
#include <QDateTime>
#include <QString>

namespace ccm::infra {

/// 파싱 이전의 읽기 결과. 실패해도 예외를 던지지 않고 사유를 담는다.
struct SnapshotReadResult {
    bool exists = false;         ///< 파일이 존재했는지 여부.
    bool ok = false;             ///< 읽기가 성공했는지 여부.
    QByteArray content;
    QDateTime modifiedAt;        ///< UTC. capturedAt 이 없을 때의 대체값.
    QString error;
};

class SnapshotStore
{
public:
    /// filePath 를 비우면 ClaudePaths::snapshotFilePath() 를 사용한다.
    explicit SnapshotStore(QString filePath = QString());

    QString filePath() const { return m_filePath; }

    /// statusline 이 넘긴 원본 JSON 을 capturedAt 과 함께 감싸 기록한다.
    /// statuslinePayload 는 stdin 으로 받은 JSON 원본,
    /// error 는 실패 사유를 받을 포인터다. 필요 없으면 nullptr.
    bool write(const QByteArray &statuslinePayload, QString *error) const;

    /// 기록해 둔 스냅샷 파일을 읽는다.
    SnapshotReadResult read() const;

private:
    QString m_filePath;
};

} // namespace ccm::infra

#endif // CCM_INFRA_SNAPSHOTSTORE_H
