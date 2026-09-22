# Copyright (C) 2026 Innogrid Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.19)
# 버전은 여기 하나뿐이다. 실행 파일 리소스(VERSIONINFO), --version 출력, NSIS
# 스크립트가 모두 이 값을 받아 간다. (core/Version.h 주석 참조)
project(ClaudeCodeMonitor VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts Network LinguistTools)

qt_standard_project_setup()

# ============================================================================
#  플랫폼 선택
#
#  운영체제마다 다른 구현은 <계층>/platform/<os>/ 에 같은 이름으로 둔다.
#  여기서 고른 폴더 하나만 빌드에 들어간다. 선언(헤더)은 계층 바로 아래에
#  그대로 있으므로, 부르는 쪽은 운영체제를 따지지 않는다.
#
#    infra/AutoStart.h        →  infra/platform/<os>/AutoStart.cpp
#    infra/AppIdentity.h      →  infra/platform/<os>/AppIdentity.cpp
#    infra/ConsoleEncoding.h  →  infra/platform/<os>/ConsoleEncoding.cpp
#    ui/DebugConsole.h        →  ui/platform/<os>/DebugConsole.cpp
#
#  파일을 나눌 만큼 크지 않은 차이(실행 파일 이름, 경로 대소문자 등)는 나누지
#  않고 infra/PlatformTraits.h 한곳에 모아 두었다.
#
#  지금 채워져 있는 것은 windows 뿐이다. macos 와 linux 는 "아직 지원하지
#  않는다" 고 답하는 자리만 잡아 두었고, 각 운영체제의 개발 환경에서 그 폴더만
#  채우면 된다. 공통 코드는 건드릴 일이 없다.
# ============================================================================
if(WIN32)
    set(CCM_PLATFORM windows)
elseif(APPLE)
    set(CCM_PLATFORM macos)
else()
    set(CCM_PLATFORM linux)
endif()
message(STATUS "ClaudeCodeMonitor: 플랫폼 구현 = ${CCM_PLATFORM}")

# ============================================================================
#  ccm_core : statusline 훅 코어 (UI 없음, QObject 미사용)
#
#  core/  파싱과 문자열 생성
#  infra/ 경로 결정, 파일 출력, 로깅
#
#  상세는 README.md 참조.
# ============================================================================

set(CCM_CORE_SOURCES
    core/UsageTypes.h
    core/Version.h
    core/SnapshotParser.h
    core/SnapshotParser.cpp
    core/StatusLineFormatter.h
    core/StatusLineFormatter.cpp
    core/UsageReportParser.h
    core/UsageReportParser.cpp
    core/TimeFormat.h
    core/TimeFormat.cpp
    core/WindowLabel.h
    core/WindowLabel.cpp
    core/AlertTypes.h
    core/AlertTypes.cpp
    core/AlertEvaluator.h
    core/AlertEvaluator.cpp

    infra/Logger.h
    infra/Logger.cpp
    infra/ClaudePaths.h
    infra/ClaudePaths.cpp
    infra/ClaudeCli.h
    infra/ClaudeCli.cpp
    infra/HookRegistrar.h
    infra/HookRegistrar.cpp
    infra/SnapshotStore.h
    infra/SnapshotStore.cpp
    infra/UsageQuery.h
    infra/UsageQuery.cpp
    infra/Settings.h
    infra/Settings.cpp
    infra/UsageHistory.h
    infra/UsageHistory.cpp
    infra/StatusLineCache.h
    infra/StatusLineCache.cpp
    infra/AlertStore.h
    infra/AlertStore.cpp
    infra/AutoStart.h
    infra/AppIdentity.h
    infra/ConsoleEncoding.h
    infra/PlatformTraits.h
    infra/Translations.h
    infra/Translations.cpp

    # 플랫폼 구현. 위에서 고른 폴더 하나만 들어간다.
    infra/platform/${CCM_PLATFORM}/AutoStart.cpp
    infra/platform/${CCM_PLATFORM}/AppIdentity.cpp
    infra/platform/${CCM_PLATFORM}/ConsoleEncoding.cpp
)

add_library(ccm_core STATIC ${CCM_CORE_SOURCES})

# 헤더를 "core/..." , "infra/..." 형태로 참조하기 위한 포함 경로.
# 프로젝트 루트가 곧 포함 루트다.
target_include_directories(ccm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ccm_core PUBLIC Qt::Core)

if(WIN32)
    # SetCurrentProcessExplicitAppUserModelID (infra/platform/windows/AppIdentity.cpp).
    # Qt 가 이미 끌고 오지만 의존을 명시해 둔다.
    target_link_libraries(ccm_core PUBLIC shell32)
endif()

# 버전 문자열을 코드로 넘긴다. PUBLIC 이므로 두 실행 파일도 함께 받는다.
target_compile_definitions(ccm_core PUBLIC CCM_VERSION_STRING="${PROJECT_VERSION}")

# Q_OBJECT 를 사용하지 않으므로 moc/uic/rcc 를 끈다. 빌드 시간을 줄인다.
set_target_properties(ccm_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

# ============================================================================
#  ccm_probe : statusline 훅 실행 파일
#   WIN32_EXECUTABLE 을 끄지 않으면 stdout 이 Claude Code 로 전달되지 않는다.
# ============================================================================

# ----------------------------------------------------------------------------
#  아이콘 리소스
#
#  탐색기와 작업 표시줄이 읽는 아이콘은 실행 파일에 박힌 리소스다. 그림의
#  원본은 ui/AppIcon 의 그리기 코드이고, .ico 는 그것으로 만든 산출물이다.
#  (res/README.md 참조)
#
#  두 실행 파일이 같은 .rc 를 쓴다. 훅도 탐색기에 보이므로 기본 아이콘으로
#  두지 않는다.
# ----------------------------------------------------------------------------
if(WIN32)
    # 실행 파일마다 자기 이름을 적은 리소스를 따로 만든다. OriginalFilename 이
    # 서로 달라야 하기 때문이다. 아이콘과 버전은 같은 원본에서 온다.
    set(CCM_VERSION "${PROJECT_VERSION}")
    set(CCM_VER_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CCM_VER_MINOR "${PROJECT_VERSION_MINOR}")
    set(CCM_VER_PATCH "${PROJECT_VERSION_PATCH}")
    set(CCM_ICO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/res/ClaudeCodeMonitor.ico")

    set(CCM_RC_DESCRIPTION "Claude Code usage monitor")
    set(CCM_RC_INTERNAL_NAME "ClaudeCodeMonitor")
    set(CCM_RC_ORIGINAL_NAME "ClaudeCodeMonitor.exe")
    configure_file(res/ClaudeCodeMonitor.rc.in
                   "${CMAKE_CURRENT_BINARY_DIR}/ClaudeCodeMonitor_app.rc" @ONLY)
    set(CCM_APP_RC "${CMAKE_CURRENT_BINARY_DIR}/ClaudeCodeMonitor_app.rc")

    set(CCM_RC_DESCRIPTION "Claude Code statusline hook")
    set(CCM_RC_INTERNAL_NAME "ccm_probe")
    set(CCM_RC_ORIGINAL_NAME "ccm_probe.exe")
    configure_file(res/ClaudeCodeMonitor.rc.in
                   "${CMAKE_CURRENT_BINARY_DIR}/ccm_probe.rc" @ONLY)
    set(CCM_PROBE_RC "${CMAKE_CURRENT_BINARY_DIR}/ccm_probe.rc")

    # 설치 관리자도 같은 번호를 쓴다. 손으로 맞추면 반드시 어긋난다.
    configure_file(installer/script/version.nsh.in
                   "${CMAKE_CURRENT_SOURCE_DIR}/installer/script/version.nsh" @ONLY)
elseif(APPLE)
    # ------------------------------------------------------------------------
    #  macOS 자리 (미구현)
    #
    #  .rc 에 해당하는 것이 .app 묶음의 Info.plist 와 .icns 다. 채울 때는
    #  MACOSX_BUNDLE_GUI_IDENTIFIER / MACOSX_BUNDLE_BUNDLE_VERSION 과
    #  MACOSX_BUNDLE_ICON_FILE 을 set_target_properties 로 준다. 알림의 주인을
    #  정하는 값도 그 식별자다. (infra/platform/macos/AppIdentity.cpp 참조)
    #  아이콘 원본은 res/ 에 있는 그림이고, iconutil 로 .icns 를 만든다.
    # ------------------------------------------------------------------------
    set(CCM_APP_RC "")
    set(CCM_PROBE_RC "")
else()
    # ------------------------------------------------------------------------
    #  Linux 자리 (미구현)
    #
    #  아이콘과 앱 이름은 실행 파일이 아니라 데스크톱 항목 파일이 가진다.
    #  ClaudeCodeMonitor.desktop 을 만들어 share/applications 에 설치하고,
    #  아이콘은 share/icons/hicolor/<크기>/apps 에 넣는다. 알림의 주인을 정하는
    #  값도 그 파일 이름이다. (infra/platform/linux/AppIdentity.cpp 참조)
    # ------------------------------------------------------------------------
    set(CCM_APP_RC "")
    set(CCM_PROBE_RC "")
endif()

add_executable(ccm_probe app/probe_main.cpp ${CCM_PROBE_RC})

target_link_libraries(ccm_probe PRIVATE ccm_core Qt::Core)

set_target_properties(ccm_probe PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)

if(MSVC)
    target_compile_options(ccm_core  PRIVATE /W4 /utf-8)
    target_compile_options(ccm_probe PRIVATE /W4 /utf-8)
else()
    target_compile_options(ccm_core  PRIVATE -Wall -Wextra)
    target_compile_options(ccm_probe PRIVATE -Wall -Wextra)
endif()

# ----------------------------------------------------------------------------
#  Qt 런타임을 실행 파일 옆에 배포한다.
#
#  Claude Code 는 시스템 PATH 만 가진 상태로 훅을 실행하므로, Qt bin 이
#  PATH 에 없으면 ccm_probe 는 0xC0000135 (DLL not found) 로 죽는다.
#  출력이 전혀 없어 원인을 찾기 어려우므로 빌드 단계에서 막는다.
# ----------------------------------------------------------------------------
# macOS 와 Linux 에서는 이 단계가 필요 없다. 훅은 설치된 Qt 를 그대로 쓰고,
# 배포본을 만드는 일은 아래 qt_generate_deploy_app_script 가 맡는다.
if(WIN32 AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET ccm_probe POST_BUILD
        COMMAND Qt6::windeployqt
                --no-translations --no-system-d3d-compiler --no-opengl-sw
                --$<IF:$<CONFIG:Debug>,debug,release>
                "$<TARGET_FILE:ccm_probe>"
        COMMENT "ccm_probe 옆에 Qt 런타임 배포"
        VERBATIM
    )
endif()

# ============================================================================
#  ClaudeCodeMonitor : Qt 위젯 애플리케이션
# ============================================================================

# ui/ 는 위젯 계층이다. QObject 를 쓰므로 moc 가 필요하고, 따라서 ccm_core 가
# 아니라 위젯 타깃에 직접 들어간다.
set(CCM_WIDGET_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    ui/UsageService.h
    ui/UsageService.cpp
    ui/AlertVisuals.h
    ui/AlertVisuals.cpp
    ui/AppIcon.h
    ui/AppIcon.cpp
    ui/CloseChoice.h
    ui/CloseChoice.cpp
    ui/DebugConsole.h
    ui/platform/${CCM_PLATFORM}/DebugConsole.cpp
    ui/HealthScanner.h
    ui/HealthScanner.cpp
    ui/HookSetup.h
    ui/HookSetup.cpp
    ui/UiColors.h
    ui/UiColors.cpp
    ui/UsageGauge.h
    ui/UsageGauge.cpp
    ui/UsageChart.h
    ui/UsageChart.cpp
    ui/UsagePopup.h
    ui/UsagePopup.cpp
    ui/TrayIcon.h
    ui/TrayIcon.cpp
    ui/SettingsDialog.h
    ui/SettingsDialog.cpp
    ui/StandardButtons.h
    ui/StandardButtons.cpp
    ui/AlertLogDialog.h
    ui/AlertLogDialog.cpp
    ui/AboutDialog.h
    ui/AboutDialog.cpp
)

qt_add_executable(ClaudeCodeMonitor
    WIN32 MACOSX_BUNDLE
    ${CCM_WIDGET_SOURCES}
    ${CCM_APP_RC}
)

# ----------------------------------------------------------------------------
#  라이선스 전문을 실행 파일에 싣는다
#
#  정보 창의 [라이선스 전문] 이 이것을 읽는다. 설치 폴더의 LICENSE 파일을 읽는
#  방법도 있지만, 개발 빌드나 파일을 지운 경우에 못 읽는다. GPLv3 가 요구하는
#  것이 "사본을 볼 수 있는 방법" 이므로, 그 약속이 파일의 유무에 걸리지 않게
#  한다. 35KB 이고 설치 파일은 9MB 대이므로 무게는 문제되지 않는다.
# ----------------------------------------------------------------------------
qt_add_resources(ClaudeCodeMonitor "legal"
    PREFIX "/legal"
    FILES LICENSE
)

# ----------------------------------------------------------------------------
#  번역
#
#  원문은 한국어다. 한국어 환경에서는 번역 파일 없이 그대로 나오고, 영어
#  환경에서는 ClaudeCodeMonitor_en.qm 이 얹힌다. 상세는 infra/Translations.h.
#
#  SOURCES 에 ccm_core 만 넘기는 이유
#   화면 글자의 상당수가 ccm_core(core/, infra/) 안에 있다. 타깃 소스만 훑게
#   두면 lupdate 가 그것들을 보지 못해 번역 파일이 반쪽이 된다.
#
#   반대로 위젯 소스와 훅 진입점은 여기 적지 않는다. TARGETS 가 이미 그 파일을
#   주므로 두 번 적으면 lupdate 가 같은 파일을 두 번 훑고, 그때 namespace 를
#   놓쳐 `UsageChart` 와 `ccm::ui::UsageChart` 처럼 같은 내용이 두 컨텍스트로
#   나온다. 번역하는 사람이 같은 문장을 두 번 보게 된다.
# ----------------------------------------------------------------------------
#  -nounfinished 를 주는 이유
#   lrelease 는 기본적으로 "미완료" 로 표시된 번역도 글자가 있으면 .qm 에 넣는다.
#   번역이 반쯤 된 파일이면 화면의 일부만 다른 언어로 바뀌어 섞인다.
#   완료된 것만 싣게 해 두면, 번역을 채우는 도중에도 화면이 한 언어로 유지된다.
qt_add_translations(
    TARGETS ClaudeCodeMonitor ccm_probe
    TS_FILES ClaudeCodeMonitor_en.ts
    SOURCES ${CCM_CORE_SOURCES}
    LRELEASE_OPTIONS -nounfinished
)

target_link_libraries(ClaudeCodeMonitor
    PRIVATE
        ccm_core
        Qt::Core
        Qt::Widgets
        Qt::Charts
        Qt::Network
)

if(MSVC)
    target_compile_options(ClaudeCodeMonitor PRIVATE /W4 /utf-8)
else()
    target_compile_options(ClaudeCodeMonitor PRIVATE -Wall -Wextra)
endif()

install(TARGETS ClaudeCodeMonitor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(WIN32 AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET ClaudeCodeMonitor POST_BUILD
        COMMAND Qt6::windeployqt
                --no-translations
                --$<IF:$<CONFIG:Debug>,debug,release>
                "$<TARGET_FILE:ClaudeCodeMonitor>"
        COMMENT "ClaudeCodeMonitor 옆에 Qt 런타임 배포"
        VERBATIM
    )
endif()

qt_generate_deploy_app_script(
    TARGET ClaudeCodeMonitor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
