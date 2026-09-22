; Copyright (C) 2026 Innogrid Co., Ltd.
; SPDX-License-Identifier: GPL-3.0-only

; ===========================================================================
;  ClaudeCodeMonitor.nsi
;
;  NSIS 설치 스크립트. 포장 대상은 collect_dlls.bat 이 만든 installer\stage 다.
;
;  사용자 단위 설치인 이유
;   이 프로그램은 HKCU 의 Run 키에 자기를 등록하고, 설정과 이력을
;   %LOCALAPPDATA% 에 쓴다. 즉 애초에 사용자 하나에 매인 프로그램이다.
;   Program Files 에 깔면 관리자 권한(UAC)이 필요해지는데, 그렇게 얻은 권한으로
;   하는 일이 없다. 그래서 %LOCALAPPDATA%\Programs 아래에 권한 상승 없이 깐다.
;
;  제거할 때 남기는 것
;   설정/이력/알림 기록은 사용자 데이터이므로 물어보고 지운다. Claude Code 의
;   settings.json 에 등록된 statusLine 훅은 건드리지 않는다. 남의 설정 파일을
;   제거 관리자가 말없이 고치면 안 되고, 대신 안내 문구를 띄운다.
; ===========================================================================

Unicode true
RequestExecutionLevel user

; 버전은 CMake 가 만들어 준다. (installer/version.nsh.in -> version.nsh)
; 손으로 적어 두면 실행 파일과 어긋나는 날이 온다. 실제로 한 번 어긋났다.
!include "version.nsh"

!define APP_NAME        "Claude Code Monitor"
!define APP_EXE         "ClaudeCodeMonitor.exe"
!define PROBE_EXE       "ccm_probe.exe"
!define APP_PUBLISHER   "Innogrid Co., Ltd."
!define APP_DIR_NAME    "ClaudeCodeMonitor"
; 경로는 모두 이 스크립트가 있는 자리(installer\script)를 기준으로 적는다.
; 상대 경로를 그냥 적으면 makensis 를 어디서 부르느냐에 따라 달라진다.
!define STAGE_DIR       "${__FILEDIR__}\..\stage"
!define DIST_DIR        "${__FILEDIR__}\..\dist"

!define UNINST_KEY \
    "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_DIR_NAME}"
!define RUN_KEY "Software\Microsoft\Windows\CurrentVersion\Run"
!define RUN_VALUE "ClaudeCodeMonitor"

; 작업 관리자의 [시작 프로그램] 에서 켜고 끈 기록. Run 값을 지워도 여기는 남는다.
!define STARTUP_APPROVED_KEY \
    "Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run"

; 트레이 아이콘 표시 설정. Windows 11 은 프로그램이 트레이에 아이콘을 올릴 때
; 이 아래에 항목을 하나 만들어 두고, 프로그램이 사라져도 스스로 지우지 않는다.
; 그대로 두면 [설정 > 개인 설정 > 작업 표시줄 > 시스템 트레이 아이콘] 목록에
; 없는 프로그램이 계속 남는다.
!define TRAY_KEY "Control Panel\NotifyIconSettings"

; 위 항목의 ExecutablePath 는 알려진 폴더 아래의 경로를 GUID 로 줄여 적는다.
; 이것이 %LOCALAPPDATA% 다. (KNOWNFOLDERID 의 FOLDERID_LocalAppData)
!define FOLDERID_LOCALAPPDATA "{F1B32785-6FBA-4FCF-9D55-7B8E7F157091}"

!define USER_DATA_DIR "$LOCALAPPDATA\${APP_DIR_NAME}"

Name "${APP_NAME}"
OutFile "${DIST_DIR}\ClaudeCodeMonitor-${APP_VERSION}-setup.exe"
InstallDir "$LOCALAPPDATA\Programs\${APP_DIR_NAME}"
InstallDirRegKey HKCU "Software\${APP_DIR_NAME}" "InstallDir"
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma

; 하단 문구. 기본값은 언어 파일의 ^Branding("Nullsoft Install System %s") 이다.
; SetCompressor 보다 뒤에 두어야 한다. 헤더를 먼저 건드리면 압축기를 바꿀 수 없다.
BrandingText /TRIMCENTER "${APP_NAME} v${APP_VERSION}"

VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "ProductName"     "${APP_NAME}"
VIAddVersionKey "FileDescription" "${APP_NAME} 설치 관리자"
VIAddVersionKey "FileVersion"     "${APP_VERSION}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey "LegalCopyright"  "Copyright (C) 2026 Innogrid Co., Ltd. GPL-3.0-only."

; ---------------------------------------------------------------------------
;  UI
; ---------------------------------------------------------------------------
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"   ; GetSize
!include "nsDialogs.nsh"   ; 제거 화면의 확인란

!define MUI_ABORTWARNING
!define MUI_ICON "${__FILEDIR__}\..\..\res\ClaudeCodeMonitor.ico"
!define MUI_UNICON "${__FILEDIR__}\..\..\res\ClaudeCodeMonitor.ico"

; 시작/마침 화면 왼쪽의 세로 그림.
;
; 지정하지 않으면 MUI 가 ${NSISDIR}\Contrib\Graphics\Wizard\win.bmp 를 쓰는데,
; 그것은 4비트(16색) 이미지다. 요즘 화면에서는 디더링 무늬가 그대로 드러나
; 파란 잡티가 낀 것처럼 보인다. 우리 것을 24비트로 만들어 쓴다.
; (res/make_installer_bitmap.py 가 만든다. 크기 164x314 는 MUI 가 정한 자리다)
!define MUI_WELCOMEFINISHPAGE_BITMAP "${__FILEDIR__}\..\..\res\installer-welcome.bmp"
; 마지막 화면의 확인란 두 개.
;
;  Field 4 = 지금 실행           (MUI_FINISHPAGE_RUN)
;  Field 5 = 로그인 시 자동 실행 (MUI_FINISHPAGE_SHOWREADME 자리를 빌려 쓴다)
;
; MUI 는 확인란을 두 개까지만 내주고, 두 번째는 "추가 정보 보기" 용도로 준비된
; 자리다. 거기에 다른 일을 얹는 것은 NSIS 에서 흔히 쓰는 방식이다.
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "지금 실행 (트레이에 상주합니다)"

!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_TEXT "로그인할 때 자동으로 실행"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION FinishReadmeNoop

!define MUI_FINISHPAGE_TEXT \
    "설치가 끝났습니다.$\r$\n$\r$\n\
자동 실행은 나중에 프로그램의 [설정 > 환경 설정] 에서도 바꿀 수 있습니다.$\r$\n$\r$\n\
상태줄 표시를 함께 쓰려면 프로그램의 [도구 > statusline 훅 등록] 을 한 번 눌러 주십시오."

!insertmacro MUI_PAGE_WELCOME

; GPLv3 는 쓰기 위해 동의를 받아야 하는 계약이 아니다. 그래도 배포본에 사본을
; 함께 주어야 하므로(GPLv3 제4조), 설치 과정에서 한 번 보여 주고 설치 폴더에도
; 남긴다. 단추 글자를 "동의함" 대신 "다음" 으로 두는 것도 그래서다.
!define MUI_LICENSEPAGE_BUTTON "다음"
!define MUI_LICENSEPAGE_TEXT_BOTTOM "이 프로그램은 GNU 일반 공중 사용 허가서 \
버전 3 으로 배포됩니다. 계속하려면 [다음] 을 누르십시오."
!insertmacro MUI_PAGE_LICENSE "${__FILEDIR__}\..\..\LICENSE"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; 확인란을 읽어 자동 실행을 등록하거나 지우는 함수를 마침 화면에 붙인다.
;
; 켜짐과 꺼짐을 한곳에서 처리한다. MUI 는 확인란이 켜졌을 때만
; MUI_FINISHPAGE_SHOWREADME_FUNCTION 을 부르므로, 껐을 때 이미 있던 등록을
; 지우는 일은 이 떠나기 함수가 맡는다. 이 함수가 프로그램 실행보다 먼저 도는
; 것도 이유가 된다. (NSIS 의 Contrib\Modern UI\System.nsh 에서 차례를 확인했다)
;
; !define 을 이 자리에 두어야 한다. MUI_PAGE_CUSTOMFUNCTION_LEAVE 는 "다음에
; 끼우는 페이지" 에 붙고, 쓰이는 순간 !undef 된다. 위쪽 MUI_FINISHPAGE_* 정의
; 사이에 끼워 두면 마침 화면이 아니라 그 아래 첫 페이지(시작 화면)에 붙는다.
; 그러면 확인란은 아무 일도 하지 않고, 시작 화면에서 [다음] 을 누르는 것만으로
; 이미 있던 자동 실행 등록이 지워진다. 실제로 그렇게 동작했다.
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE ApplyAutoStartChoice
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM

; 훅을 함께 지울지 묻는 화면. MUI 는 제거 쪽에 확인란 자리를 내주지 않으므로
; nsDialogs 로 한 장 만든다.
UninstPage custom un.HookPage un.HookPageLeave

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
;  마지막 화면의 확인란 처리
; ---------------------------------------------------------------------------

; MUI 는 "추가 정보" 확인란이 켜져 있을 때 이 함수를 부른다. 실제 처리는
; ApplyAutoStartChoice 가 먼저 끝내 두므로 여기서 할 일이 없다. 그래도 비워
; 둔 채로 필요하다. 이 함수를 주지 않으면 MUI 가 빈 문자열을 ExecShell 로
; 열려고 든다.
Function FinishReadmeNoop
FunctionEnd

; 확인란을 읽어 로그인 시 자동 실행을 등록하거나 지운다.
;
; 켜짐과 꺼짐을 한곳에서 다루는 이유는, MUI 가 켜졌을 때만 함수를 부르기
; 때문이다. 껐을 때 이미 있던 등록을 지우는 일은 아무도 하지 않게 된다.
;
; 프로그램이 쓰는 것과 글자 하나까지 같아야 한다. 다르면 설정 창이
; "다른 실행 파일이 등록되어 있습니다" 로 읽는다. (infra/AutoStart.cpp)
; ---------------------------------------------------------------------------
;  제거 화면 - statusline 훅을 함께 지울지
; ---------------------------------------------------------------------------

Var HookCheckbox
Var HookRemoveWanted
Var HookState        ; none / this-app / other-copy / foreign / unknown
Var HookCommand      ; settings.json 에 적혀 있는 명령 전문
Var HookExists       ; 그 명령이 가리키는 파일이 아직 있는가 (1/0)
Var HookAllowOther   ; 1 이면 --allow-other-copy 를 붙여 부른다

; 지금 무엇이 걸려 있는지 읽어 온다. 고치지 않고 읽기만 한다.
;
; 화면에 "statusline 훅 등록도 함께 지우기" 라고만 적어 두면 사용자는 무엇을
; 지우는지 모르고 누르게 된다. 등록 쪽 확인 창은 경로를 보여 주는데 제거 쪽만
; 보여 주지 않았다.
;
; 읽는 일은 ccm_probe 가 한다. NSIS 가 JSON 을 파싱하는 것보다 낫고, 우리
; 것인지 가리는 규칙이 한 곳에만 있게 된다. 결과는 UTF-16LE INI 로 받는다.
; ReadINIStr(GetPrivateProfileStringW)가 유니코드로 읽어 주는 유일한 꼴이라,
; 경로에 한글이 섞여도 깨지지 않는다.
Function un.ReadHookState
    StrCpy $HookState "unknown"
    StrCpy $HookCommand ""
    StrCpy $HookExists ""
    StrCpy $HookAllowOther 0

    ${IfNot} ${FileExists} "$INSTDIR\${PROBE_EXE}"
        Return
    ${EndIf}

    Delete "$PLUGINSDIR\hookstate.ini"
    ; nsExec 로 부른다. ExecWait 는 콘솔 창이 한 번 번쩍인다.
    nsExec::Exec '"$INSTDIR\${PROBE_EXE}" --print-hook-state --out "$PLUGINSDIR\hookstate.ini"'
    Pop $0
    ${If} $0 == "error"
    ${OrIf} $0 == "timeout"
        Return
    ${EndIf}

    ReadINIStr $1 "$PLUGINSDIR\hookstate.ini" "hook" "state"
    ${If} $1 != ""
        StrCpy $HookState $1
        ReadINIStr $HookCommand "$PLUGINSDIR\hookstate.ini" "hook" "command"
        ReadINIStr $HookExists "$PLUGINSDIR\hookstate.ini" "hook" "exists"
    ${EndIf}
FunctionEnd

Function un.HookPage
    !insertmacro MUI_HEADER_TEXT "statusline 훅" \
        "Claude Code 터미널 아래의 사용률 표시를 함께 지울지 정합니다."

    Call un.ReadHookState

    nsDialogs::Create 1018
    Pop $0
    ${If} $0 == error
        Abort
    ${EndIf}

    ; 상태마다 할 말이 다르다.
    ;   $2 설명 / $3 확인란 초기 상태 / $4 확인란을 쓸 수 있는가 / $5 덧붙이는 말
    ${If} $HookState == "none"
        StrCpy $2 "Claude Code 의 설정 파일에 등록된 statusLine 항목이 없습니다.$\r$\n$\r$\n지울 것이 없으므로 이 화면에서 할 일은 없습니다."
        StrCpy $3 0
        StrCpy $4 0
        StrCpy $5 "설치 후 [도구 > statusline 훅 등록] 을 누른 적이 없거나, 이미 지운 상태입니다."
    ${ElseIf} $HookState == "this-app"
        StrCpy $2 "이 프로그램이 등록한 statusLine 항목이 Claude Code 의 설정 파일에 있습니다.$\r$\n$\r$\n함께 지우면 터미널 아래의 사용률 표시가 사라집니다. Claude Code 의 다른 설정은 건드리지 않으며, 고치기 전에 설정 파일을 백업합니다."
        StrCpy $3 1
        StrCpy $4 1
        StrCpy $5 "사용량 조회와 알림은 훅과 상관없이 동작합니다. 프로그램을 지우면 어차피 함께 없어집니다."
    ${ElseIf} $HookState == "other-copy"
        ${If} $HookExists == "0"
            StrCpy $2 "이 프로그램의 다른 복사본이 등록되어 있고, 그 파일은 이미 없습니다.$\r$\n$\r$\n이대로 두면 없는 파일을 가리키는 항목이 남아, Claude Code 의 상태줄이 이유 없이 빈칸으로 보입니다."
            StrCpy $3 1
            StrCpy $5 "지워도 잃을 것이 없습니다. 가리키는 파일이 이미 없기 때문입니다."
        ${Else}
            StrCpy $2 "이 프로그램의 다른 복사본이 등록되어 있습니다. 지금 이 설치본이 등록한 것이 아닙니다.$\r$\n$\r$\n그 복사본이 아직 남아 있으므로 상태줄은 계속 동작할 수 있습니다. 지울지는 아래 경로를 보고 정하십시오."
            StrCpy $3 0
            StrCpy $5 "다른 설치본이나 예전 빌드가 등록해 둔 것입니다. 그쪽을 계속 쓸 생각이면 켜지 마십시오."
        ${EndIf}
        StrCpy $4 1
    ${ElseIf} $HookState == "foreign"
        StrCpy $2 "다른 프로그램의 statusLine 설정이 등록되어 있습니다.$\r$\n$\r$\n우리가 등록한 것이 아니므로 건드리지 않습니다. 설정 파일을 열지도 않습니다."
        StrCpy $3 0
        StrCpy $4 0
        StrCpy $5 "남의 설정 파일을 제거 관리자가 말없이 고쳐서는 안 됩니다."
    ${Else}
        StrCpy $2 "지금 무엇이 등록되어 있는지 확인하지 못했습니다.$\r$\n$\r$\n켜 두면 지우기를 시도하고, 우리가 등록한 것이 아니면 그대로 둡니다. 고치기 전에 설정 파일을 백업합니다."
        StrCpy $3 1
        StrCpy $4 1
        StrCpy $5 "이 프로그램이 등록한 것만 지웁니다. 다른 프로그램이 상태줄을 쓰고 있으면 그대로 둡니다."
    ${EndIf}

    ${NSD_CreateLabel} 0 0 100% 34u "$2"
    Pop $0

    ${NSD_CreateLabel} 0 38u 100% 10u "지금 등록되어 있는 명령"
    Pop $0

    ; 읽기 전용 입력 칸에 담는다. 경로가 길어 한눈에 들어오지 않더라도
    ; 사용자가 끝까지 훑어보고 복사할 수 있어야 한다. 라벨로는 그렇게 되지 않는다.
    ${If} $HookCommand == ""
        ${NSD_CreateText} 0 49u 100% 13u "(없음)"
    ${Else}
        ${NSD_CreateText} 0 49u 100% 13u "$HookCommand"
    ${EndIf}
    Pop $0
    SendMessage $0 ${EM_SETREADONLY} 1 0

    ${NSD_CreateCheckbox} 0 68u 100% 12u "statusline 훅 등록도 함께 지우기"
    Pop $HookCheckbox
    ${If} $3 == 1
        ${NSD_Check} $HookCheckbox
    ${EndIf}
    ${If} $4 == 0
        EnableWindow $HookCheckbox 0
    ${EndIf}

    ${NSD_CreateLabel} 0 84u 100% 24u "$5"
    Pop $0

    nsDialogs::Show
FunctionEnd

Function un.HookPageLeave
    ${NSD_GetState} $HookCheckbox $HookRemoveWanted

    ; 다른 자리의 복사본은 기본적으로 손대지 않는다. 다만 이 화면이 그 경로를
    ; 보여 주었고 사용자가 그것을 보고 켰다면, "모르는 사이에 없어지면 안 된다"
    ; 는 조건은 이미 채워졌다. 그때만 --allow-other-copy 를 붙인다.
    ${If} $HookRemoveWanted == 1
    ${AndIf} $HookState == "other-copy"
        StrCpy $HookAllowOther 1
    ${EndIf}
FunctionEnd

Function ApplyAutoStartChoice
    ; 확인란의 상태를 창에서 직접 읽는다.
    ;
    ; MUI2 의 마침 화면은 InstallOptions 가 아니라 nsDialogs 로 만들어진다.
    ; ($NSISDIR\Contrib\Modern UI 2\Pages\Finish.nsh 의 nsDialogs::Create 1044)
    ; 그래서 ioSpecial.ini 라는 파일 자체가 없고, 거기서 읽으면 언제나 빈
    ; 값이 나온다. 그러면 이 함수는 확인란을 켜 두어도 늘 "껐다" 로 읽어
    ; 등록을 지운다. 실제로 그렇게 동작했다.
    ;
    ; MUI 자신도 같은 함수 안에서 이 방법으로 읽는다. 우리 떠나기 함수가
    ; 그 앞에 끼워지므로 확인란은 아직 살아 있다.
    SendMessage $mui.FinishPage.ShowReadme ${BM_GETCHECK} 0 0 $0
    ${If} $0 == ${BST_CHECKED}
        WriteRegStr HKCU "${RUN_KEY}" "${RUN_VALUE}" '"$INSTDIR\${APP_EXE}" --tray'
        DetailPrint "로그인 시 자동 실행을 등록했습니다."
    ${Else}
        DeleteRegValue HKCU "${RUN_KEY}" "${RUN_VALUE}"
        DetailPrint "로그인 시 자동 실행을 등록하지 않았습니다."
    ${EndIf}
FunctionEnd

; ---------------------------------------------------------------------------
;  제거 - 트레이 아이콘 표시 설정
; ---------------------------------------------------------------------------

Var TrayExePath
Var TrayExeAlt

; HKCU\Control Panel\NotifyIconSettings 를 훑어 우리 실행 파일을 가리키는 항목만
; 지운다.
;
; 하위 키 이름은 실행 파일 경로를 Windows 가 해시한 숫자라 미리 알 수 없다. 그래서
; 하나씩 열어 ExecutablePath 를 읽고 견준다. 다른 프로그램의 항목은 건드리지 않는다.
;
; Windows 10 의 TrayNotify\IconStreams 는 모든 프로그램의 설정이 한 덩어리 이진 값에
; 들어 있어 우리 것만 떼어낼 수 없다. 그쪽은 손대지 않는다.
Function un.CleanTrayIconSettings
    StrCpy $TrayExePath "$INSTDIR\${APP_EXE}"
    StrCpy $TrayExeAlt ""

    ; 설치 위치가 %LOCALAPPDATA% 아래면 GUID 로 줄여 적힌 꼴로 남는다. 두 꼴을 모두
    ; 견준다. 설치 위치를 다른 데로 바꾼 사람은 앞의 꼴로 남는다.
    StrLen $R0 "$LOCALAPPDATA"
    StrCpy $R1 "$INSTDIR" $R0
    ${If} $R1 == "$LOCALAPPDATA"
        StrCpy $R2 "$INSTDIR" "" $R0
        StrCpy $TrayExeAlt "${FOLDERID_LOCALAPPDATA}$R2\${APP_EXE}"
    ${EndIf}

    StrCpy $R0 0
    ${Do}
        EnumRegKey $R1 HKCU "${TRAY_KEY}" $R0
        ${If} $R1 == ""
            ${ExitDo}
        ${EndIf}

        ReadRegStr $R2 HKCU "${TRAY_KEY}\$R1" "ExecutablePath"

        ; $TrayExeAlt 가 빈 값일 때 ExecutablePath 가 없는 항목과 같다고 판정되지
        ; 않도록 먼저 걸러 낸다.
        StrCpy $R3 0
        ${If} $R2 != ""
            ${If} $R2 == $TrayExePath
                StrCpy $R3 1
            ${ElseIf} $R2 == $TrayExeAlt
                StrCpy $R3 1
            ${EndIf}
        ${EndIf}

        ${If} $R3 == 1
            DeleteRegKey HKCU "${TRAY_KEY}\$R1"
            DetailPrint "트레이 아이콘 표시 설정을 지웠습니다. ($R2)"
            ; 하나를 지우면 뒤 항목의 번호가 앞으로 당겨진다. 번호를 올리지 않고
            ; 같은 자리를 다시 읽는다.
        ${Else}
            IntOp $R0 $R0 + 1
        ${EndIf}
    ${Loop}
FunctionEnd

; ---------------------------------------------------------------------------
;  설치
; ---------------------------------------------------------------------------
Section "설치" SecInstall

    ; 실행 중이면 파일을 덮어쓸 수 없다. 먼저 알린다.
    ; (조용히 죽이지 않는다. 사용자가 무엇을 닫는지 알아야 한다)
    FindWindow $0 "" "Claude Code 사용량 모니터"
    ${If} $0 != 0
        MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
            "${APP_NAME} 가 실행 중입니다.$\r$\n트레이 아이콘에서 종료한 뒤 [확인] 을 누르십시오." \
            IDOK continue_install
        Abort "설치를 취소했습니다."
    ${EndIf}
    continue_install:

    SetOutPath "$INSTDIR"
    SetOverwrite try

    ; stage 전체를 그대로 옮긴다. 어느 DLL 이 필요한지는 windeployqt 가
    ; 이미 판단했으므로 여기서 다시 고르지 않는다.
    File /r "${STAGE_DIR}\*.*"

    ; MSVC 재배포 파일이 함께 왔으면 조용히 설치한다.
    ; windeployqt --compiler-runtime 이 넣어 주며, 없을 수도 있다.
    ${If} ${FileExists} "$INSTDIR\vc_redist.x64.exe"
        DetailPrint "Visual C++ 재배포 패키지 확인 중..."
        ExecWait '"$INSTDIR\vc_redist.x64.exe" /install /quiet /norestart' $0
        DetailPrint "  종료 코드: $0"
        Delete "$INSTDIR\vc_redist.x64.exe"
    ${EndIf}

    WriteRegStr HKCU "Software\${APP_DIR_NAME}" "InstallDir" "$INSTDIR"

    ; 로그인 시 자동 실행은 마지막 화면의 확인란이 정한다.
    ;
    ; 프로그램에도 "처음 실행이면 스스로 등록한다" 는 길이 있다. zip 으로 받아
    ; 쓰는 사람을 위한 것인데, 그대로 두면 설치할 때 끄기를 고른 사람의 선택을
    ; 첫 실행이 덮어 버린다. 그래서 첫 실행 표시를 여기서 미리 찍어 둔다.
    ; 설정 파일의 다른 값은 건드리지 않는다. (WriteINIStr 는 해당 항목만 고친다)
    CreateDirectory "${USER_DATA_DIR}"
    WriteINIStr "${USER_DATA_DIR}\settings.ini" "app" "firstRunDone" "true"

    ; 조용한 설치(/S)에는 마지막 화면이 없다. 권장값으로 등록해 둔다.
    ${If} ${Silent}
        WriteRegStr HKCU "${RUN_KEY}" "${RUN_VALUE}" '"$INSTDIR\${APP_EXE}" --tray'
    ${EndIf}
    ; 시작 메뉴 바로가기
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\제거.lnk" "$INSTDIR\uninstall.exe"

    ; 제어판 프로그램 목록 (사용자 단위이므로 HKCU)
    WriteRegStr   HKCU "${UNINST_KEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr   HKCU "${UNINST_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKCU "${UNINST_KEY}" "Publisher"       "${APP_PUBLISHER}"
    WriteRegStr   HKCU "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${APP_EXE}"
    WriteRegStr   HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKCU "${UNINST_KEY}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr   HKCU "${UNINST_KEY}" "QuietUninstallString" \
        "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1

    ; 설치 크기를 적어 둔다. 제어판이 보여 준다.
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "${UNINST_KEY}" "EstimatedSize" "$0"

    WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

; ---------------------------------------------------------------------------
;  제거
; ---------------------------------------------------------------------------
Section "Uninstall"

    FindWindow $0 "" "Claude Code 사용량 모니터"
    ${If} $0 != 0
        MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
            "${APP_NAME} 가 실행 중입니다.$\r$\n트레이 아이콘에서 종료한 뒤 [확인] 을 누르십시오." \
            IDOK continue_uninstall
        Abort "제거를 취소했습니다."
    ${EndIf}
    continue_uninstall:

    ; 로그인 시 자동 실행 등록을 지운다. 지우지 않으면 없는 파일을 가리키는
    ; 항목이 시작 앱 목록에 남는다.
    DeleteRegValue HKCU "${RUN_KEY}" "${RUN_VALUE}"

    ; 작업 관리자의 [시작 프로그램] 에서 켜고 끈 기록. Run 값과 짝이라 함께 지운다.
    ; 남겨 두면 다시 깔았을 때 예전에 꺼 둔 상태가 되살아난다.
    DeleteRegValue HKCU "${STARTUP_APPROVED_KEY}" "${RUN_VALUE}"

    ; 트레이 아이콘 표시 설정. 지우지 않으면 [설정 > 개인 설정 > 작업 표시줄] 의
    ; 목록에 없는 프로그램이 계속 남는다. 그 목록은 explorer 가 들고 있는 것을
    ; 그리므로, 지운 결과는 다시 로그인해야 보인다.
    Call un.CleanTrayIconSettings

    ; statusline 훅. 확인란을 켠 경우에만.
    ;
    ; 파일을 지우기 전에 해야 한다. 지우는 일을 ccm_probe 자신이 하기 때문이다.
    ; NSIS 가 JSON 을 직접 고치는 것보다 이쪽이 낫다. 우리 것인지 가리는 규칙과
    ; 백업이 이미 그 안에 들어 있고, 규칙이 한 곳에만 있게 된다.
    ;
    ; 조용한 제거(/S)에는 화면이 없어 확인란도 없다. 그때는 남긴다.
    ; 남의 설정 파일을 묻지 않고 고치지 않는다는 원칙을 따른다.
    ${If} $HookRemoveWanted == 1
        ${If} ${FileExists} "$INSTDIR\${PROBE_EXE}"
            ; 제거 화면이 등록된 경로를 보여 주었고 사용자가 그것을 보고 켰을
            ; 때만 다른 자리의 복사본까지 지운다. (un.HookPageLeave 참조)
            StrCpy $R5 ""
            ${If} $HookAllowOther == 1
                StrCpy $R5 " --allow-other-copy"
            ${EndIf}
            DetailPrint "statusline 훅 등록을 확인하는 중..."
            ExecWait '"$INSTDIR\${PROBE_EXE}" --unregister$R5' $R6
            ${If} $R6 == 0
                DetailPrint "  훅 등록을 지웠습니다."
            ${ElseIf} $R6 == 5
                DetailPrint "  이 프로그램이 등록한 것이 아니라 그대로 두었습니다."
            ${Else}
                DetailPrint "  훅 등록을 지우지 못했습니다. (종료 코드 $R6)"
            ${EndIf}
        ${EndIf}
    ${EndIf}

    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\제거.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    ; 설치한 파일만 지운다. RMDir /r 로 $INSTDIR 를 통째로 지우는 것은
    ; 설치 경로를 사용자가 바꿨을 때 위험하다.
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\${PROBE_EXE}"
    Delete "$INSTDIR\*.qm"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\uninstall.exe"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\networkinformation"
    RMDir /r "$INSTDIR\tls"
    RMDir /r "$INSTDIR\generic"
    RMDir "$INSTDIR"

    DeleteRegKey HKCU "${UNINST_KEY}"
    DeleteRegKey HKCU "Software\${APP_DIR_NAME}"

    ; 사용자 데이터는 물어본다. 임계치 설정과 사용률 이력이 들어 있다.
    ;
    ; 조용한 제거(/S)에서는 묻지 않고 남긴다. NSIS 는 조용한 모드에서
    ; MessageBox 를 띄우지 않고 첫 번째 단추를 누른 것으로 치는데, 그것이
    ; 여기서는 [예] 다. 그대로 두면 아무 물음 없이 설정과 이력이 사라진다.
    ; 지우는 쪽이 되돌릴 수 없으므로 남기는 쪽을 기본으로 삼는다.
    IfSilent keep_user_data
    ${If} ${FileExists} "${USER_DATA_DIR}\*.*"
        MessageBox MB_YESNO|MB_ICONQUESTION \
            "설정과 사용률 이력도 지울까요?$\r$\n$\r$\n${USER_DATA_DIR}" \
            IDNO keep_user_data
        RMDir /r "${USER_DATA_DIR}"
        keep_user_data:
    ${EndIf}

    ; statusline 훅은 Claude Code 의 설정 파일에 있다. 남의 설정 파일을
    ; 말없이 고치지 않고 안내만 한다.
    IfSilent skip_hook_notice
    ${If} $HookRemoveWanted == 1
        MessageBox MB_OK|MB_ICONINFORMATION "제거했습니다."
    ${ElseIf} $HookState == "none"
        MessageBox MB_OK|MB_ICONINFORMATION "제거했습니다."
    ${ElseIf} $HookState == "foreign"
        MessageBox MB_OK|MB_ICONINFORMATION \
            "제거했습니다.$\r$\n$\r$\n\
Claude Code 의 statusLine 설정은 다른 프로그램의 것이라 건드리지 않았습니다."
    ${Else}
        MessageBox MB_OK|MB_ICONINFORMATION \
            "제거했습니다.$\r$\n$\r$\n\
statusline 훅 등록은 남겨 두었습니다. 지우려면 Claude Code 의 settings.json 에서$\r$\n\
$\"statusLine$\" 항목을 직접 지우십시오:$\r$\n$PROFILE\.claude\settings.json"
    ${EndIf}
    skip_hook_notice:

SectionEnd
