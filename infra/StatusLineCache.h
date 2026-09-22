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
//  infra/StatusLineCache.h
//
//  위젯 앱이 아는 것을 훅에게 건네는 자리.
//
//  왜 필요한가
//   두 실행 파일이 보는 것이 다르다.
//
//     훅 (ccm_probe)  : Claude Code 가 넘기는 payload. five_hour, seven_day 뿐.
//     앱 (위젯)        : claude 의 get_usage 응답. 모델별 창까지 전부.
//
//   상태줄에 "Fable : 22%" 를 적으려면 훅이 모델별 값을 알아야 하는데, 훅에게는
//   그 값이 오지 않는다. 그래서 앱이 조회할 때마다 모델별 값만 작은 파일에
//   적어 두고, 훅이 그것을 읽어 뒤에 붙인다.
//
//  모델별 값만 담는 이유
//   five_hour 와 seven_day 는 훅이 payload 로 직접 받는다. 그쪽이 5초마다
//   갱신되는 신선한 값이므로, 여기에 또 담아 두면 어느 쪽이 참인지 헷갈린다.
//   훅이 모르는 것만 담는다.
//
//  낡은 값은 버린다
//   앱이 꺼져 있으면 이 파일은 그대로 멈춘다. 어제 값이 오늘 상태줄에 그대로
//   떠 있으면 거짓말이 되므로, 읽는 쪽이 기한을 정해 그보다 오래된 것은 없는
//   것으로 다룬다. 그러면 상태줄이 Session/Model 둘로 자연히 줄어든다.
//
//  파일 형식
//   { "schemaVersion": 1,
//     "at": "ISO8601",
//     "windows": [ { "label": "Fable", "usedPercent": 22.0 } ] }
// ============================================================================
#ifndef CCM_INFRA_STATUSLINECACHE_H
#define CCM_INFRA_STATUSLINECACHE_H

#include "core/StatusLineFormatter.h"
#include "core/UsageTypes.h"

#include <QList>
#include <QString>

namespace ccm::infra {

class StatusLineCache
{
public:
    /// filePath 를 비우면 ClaudePaths::statusLineCacheFilePath() 를 쓴다.
    explicit StatusLineCache(const QString &filePath = QString());

    /// 보고에서 모델별 창만 골라 적는다. 모델별 창이 없으면 빈 목록을 적는다.
    /// (그래야 모델이 사라졌을 때 옛 값이 남지 않는다)
    bool write(const ccm::core::UsageReport &report, QString *error = nullptr) const;

    /// 적혀 있는 모델별 값. 파일이 없거나 낡았으면 빈 목록.
    /// \param maxAgeMinutes 이 시간을 넘긴 파일은 없는 것으로 다룬다.
    QList<ccm::core::StatusLineExtra> read(int maxAgeMinutes) const;

    QString filePath() const { return m_filePath; }

    /// 읽는 쪽이 따로 정하지 않을 때 쓰는 기한.
    ///
    /// 폴링 간격(기본 5분)보다 넉넉해야 한다. 조회 한 번을 건너뛰었다고 상태줄
    /// 글자가 사라졌다 나타났다 하면 눈에 거슬린다.
    static constexpr int defaultMaxAgeMinutes() { return 30; }

private:
    QString m_filePath;
};

} // namespace ccm::infra

#endif // CCM_INFRA_STATUSLINECACHE_H
