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
//  infra/HookRegistrar.h
//
//  ccm_probe 를 Claude Code 의 statusline 훅으로 등록한다.
//
//  설계 원칙
//   1. 사용자 설정 파일을 고치는 일이므로 쓰기 전에 반드시 백업한다.
//   2. 이미 다른 statusLine 설정이 있으면 force 없이는 건드리지 않는다.
//   3. 등록 전에 대상 실행 파일이 실제로 실행되는지 검증한다.
//      Qt 런타임 DLL 이 없으면 Claude Code 가 훅을 호출해도 아무 출력 없이
//      죽기 때문에, 등록해 두고 나중에 원인을 찾는 것보다 여기서 막는 편이 낫다.
//   4. 명령 문자열은 셸이 해석한다. Claude Code 는 윈도우에서도 bash 로 훅을
//      실행하므로 역슬래시 경로는 이스케이프로 사라진다. 슬래시 경로를 따옴표로
//      감싸 적는다.
//
//  주의: Qt 로 JSON 을 다시 쓰면 키가 알파벳순으로 정렬되고 들여쓰기가 바뀐다.
//        의미는 동일하지만 파일 전체가 재작성되므로 백업이 필수다.
// ============================================================================
#ifndef CCM_INFRA_HOOKREGISTRAR_H
#define CCM_INFRA_HOOKREGISTRAR_H

#include <QString>

namespace ccm::infra {

struct HookRegistrationResult {
    enum class Status {
        Registered,         ///< 새로 기록했다.
        AlreadyRegistered,  ///< 같은 명령으로 이미 등록되어 있어 아무것도 쓰지 않았다.
        Conflict,           ///< 다른 statusLine 설정이 있다. force 가 필요하다.
        ProbeMissing,       ///< ccm_probe 실행 파일을 찾지 못했다.
        ProbeNotRunnable,   ///< 실행 검증에 실패했다. (예: Qt DLL 누락)
        Failed              ///< 그 외 실패.
    };

    Status status = Status::Failed;
    QString message;          ///< 사람이 읽을 설명.
    QString probePath;        ///< 등록 대상 실행 파일.
    QString settingsPath;     ///< 대상 settings.json.
    QString backupPath;       ///< 만든 백업. 쓰지 않았으면 빈 문자열.
    QString existingCommand;  ///< Conflict 일 때 기존에 있던 명령.

    bool ok() const
    {
        return status == Status::Registered || status == Status::AlreadyRegistered;
    }
};

/// 훅을 지운 결과.
struct HookRemovalResult {
    enum class Status {
        Removed,        ///< 지웠다.
        NotRegistered,  ///< 지울 것이 없었다.
        Foreign,        ///< 남의 명령이라 손대지 않았다.
        NeedsConsent,   ///< 우리 프로그램이지만 다른 자리의 것이다. 물어봐야 한다.
        Failed          ///< 파일을 고치지 못했다.
    };

    Status status = Status::Failed;
    QString message;          ///< 사람이 읽을 설명.
    QString command;          ///< 지웠거나, 지우지 않고 둔 명령.
    QString settingsPath;     ///< 대상 settings.json.
    QString backupPath;       ///< 만든 백업. 쓰지 않았으면 빈 문자열.

    /// 지운 것과 지울 것이 없던 것. 둘 다 "이제 등록되어 있지 않다" 이다.
    bool ok() const
    {
        return status == Status::Removed || status == Status::NotRegistered;
    }
};

class HookRegistrar
{
public:
    HookRegistrar() = delete;

    /// 지금 settings.json 에 무엇이 적혀 있는가.
    enum class State {
        NotRegistered,   ///< statusLine 설정이 없다.
        ThisApp,         ///< 우리 ccm_probe 가 등록되어 있다. (경로까지 같다)
        OtherCopy,       ///< 이름은 ccm_probe 인데 다른 자리의 것이다.
        Other            ///< 남의 명령이다. 덮으려면 force 가 필요하다.
    };

    /// 등록 상태를 읽기만 한다. 파일을 고치지 않는다.
    ///
    /// 설정 화면이 "지금 어떤 상태인가" 를 보여 주기 위한 것이다. registerHook
    /// 으로 알아내려 하면 등록되지 않은 상태에서 등록해 버린다.
    ///
    /// \param registeredCommand 비어 있지 않으면 지금 적혀 있는 명령을 담아 준다.
    static State state(QString *registeredCommand = nullptr);

    /// 실행 파일과 같은 디렉터리의 ccm_probe 를 statusline 훅으로 등록한다.
    /// force 가 false 이면 다른 statusLine 설정이 있을 때 그대로 두고 Conflict 를 돌려준다.
    static HookRegistrationResult registerHook(bool force = false);

    /// 등록된 statusLine 설정을 지운다. **우리 것만 지운다.**
    ///
    /// 남의 상태줄 설정을 말없이 날리지 않기 위해, 지우기 전에 실행 파일을
    /// 견준다. 남의 명령이면 Foreign 을 돌려주고 파일을 열지도 않는다.
    ///
    /// 우리 프로그램이지만 다른 자리의 복사본(예전 빌드, 다른 설치본)이 등록된
    /// 경우는 따로 나눈다. 지워도 되는 것이 맞지만 사용자가 모르는 사이에
    /// 없어지면 안 되므로, allowOtherCopy 없이는 NeedsConsent 로 돌려준다.
    ///
    /// **예외가 하나 있다.** 그 복사본의 실행 파일이 이미 없으면 묻지 않고
    /// 지운다. 없는 파일을 가리키는 statusLine 은 Claude Code 에서 조용히
    /// 실패해 상태줄만 빈칸으로 만들고, 되살릴 방법도 없다. 파일이 살아 있는
    /// 복사본은 지금도 동작 중인 설정이므로 그대로 둔다.
    ///
    /// 고치기 전에 settings.json 을 백업한다. 등록할 때와 같은 규칙이다.
    static HookRemovalResult unregisterHook(bool allowOtherCopy = false);

    /// 등록 대상 ccm_probe 의 절대 경로. 현재 실행 파일 위치에서 도출한다.
    static QString probePath();

    /// 명령 문자열에서 실행 파일 부분만 떼어 낸다. 따옴표로 감싼 첫 덩이다.
    ///
    /// 등록된 명령이 아직 실재하는 파일을 가리키는지 밖에서 확인할 수 있게
    /// 열어 둔다. 구분자는 슬래시로 돌려준다.
    static QString executableOf(const QString &command);

    /// settings.json 에 적히는 명령 전문. (경로와 인자)
    static QString registrationCommand();
};

} // namespace ccm::infra

#endif // CCM_INFRA_HOOKREGISTRAR_H
