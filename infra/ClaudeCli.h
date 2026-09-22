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
//  infra/ClaudeCli.h
//
//  claude CLI 의 get_usage control request 프로토콜을 한곳에 모아 둔다.
//
//  왜 CLI 를 띄우는가
//   statusline 훅이 넘기는 payload 의 rate_limits 에는 five_hour 와 seven_day
//   두 개만 들어 있다. Claude 앱의 사용량 화면에 보이는 모델별 주간 창("Fable")
//   은 그 payload 에 아예 담기지 않는다. 세 개를 모두 받으려면 값을 요청하는
//   경로가 필요하고, 그게 SDK 용 control request 인 get_usage 다.
//
//  왜 여기에는 실행 코드가 없는가
//   조회 1회에 claude 세션이 하나 뜨고 1~2초가 걸린다. GUI 가 그동안 멈추면
//   안 되므로 실제 실행은 비동기로 해야 하고, 그것은 QObject 와 시그널이 필요해
//   위젯 계층(ui/UsageService)의 일이다. 여기서는 "무엇을 어떻게 주고받는가"
//   만 정의한다. 프로토콜 지식이 두 곳으로 갈라지지 않게 하기 위함이다.
//
//  주의: control request 는 Anthropic 의 SDK 내부 프로토콜이다. 버전에 따라
//        바뀔 수 있으므로 실패를 정상 경로로 다룬다. (스냅샷 조회가 대안이다)
// ============================================================================
#ifndef CCM_INFRA_CLAUDECLI_H
#define CCM_INFRA_CLAUDECLI_H

#include "core/UsageTypes.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace ccm::infra {

class ClaudeCli
{
public:
    ClaudeCli() = delete;

    /// claude 실행 파일의 절대 경로. 찾지 못하면 빈 문자열.
    static QString findExecutable();

    /// claude 를 SDK 모드로 띄우는 인자.
    static QStringList sdkArguments();

    /// 표준 입력에 넣을 get_usage 요청 한 줄. 줄바꿈까지 포함한다.
    static QByteArray usageRequestLine();

    /// 표준 출력에서 응답 줄을 알아보는 표식.
    static QByteArray responseMarker();

    /// 조회에 허용할 기본 제한 시간.
    static int defaultTimeoutMs();

    /// 보고를 사람이 읽을 여러 줄 텍스트로. 팝업과 로그가 함께 쓴다.
    static QString describe(const ccm::core::UsageReport &report);
};

} // namespace ccm::infra

#endif // CCM_INFRA_CLAUDECLI_H
