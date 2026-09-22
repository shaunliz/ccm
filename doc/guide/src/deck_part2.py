# -*- coding: utf-8 -*-
# 1. 설치  ~  2. 기능 소개   (build_deck.py 가 exec 로 이어 붙인다)

# ===========================================================================
#  1. 설치
# ===========================================================================
divider("SECTION 1", "설치", ["설치 순서", "화면별 안내", "설치 후 확인", "조용한 설치"])

s, y = page("1. 설치", "1.1  설치 순서 한눈에", "설치 파일을 두 번 누르면 다섯 화면이 지나갑니다. 관리자 권한은 묻지 않습니다.")
steps = [
    ("①", "시작", "안내를 읽고 [다음]"),
    ("②", "라이선스", "GPLv3 전문. [다음]"),
    ("③", "설치 위치", "기본값 그대로 [설치]"),
    ("④", "설치 중", "파일 복사 (수 초)"),
    ("⑤", "완료", "확인란 2개 고르고 [마침]"),
]
bw = Inches(2.25)
gap = Inches(0.18)
for i, (no, t, d) in enumerate(steps):
    x = MARGIN + (bw + gap) * i
    rect(s, x, Inches(2.15), bw, Inches(1.55), fill=BG_SOFT)
    rect(s, x, Inches(2.15), bw, Inches(0.055), fill=ACCENT)
    _, tf = textbox(s, x + Inches(0.18), Inches(2.35), bw - Inches(0.36), Inches(1.2))
    para(tf, no, size=20, bold=True, color=ACCENT, space_after=2, first=True)
    para(tf, t, size=15, bold=True, color=PRIMARY, space_after=4)
    para(tf, d, size=11.5, color=INK_SOFT, space_after=0, line=1.18)
    if i < len(steps) - 1:
        _, tf = textbox(s, x + bw + Emu(20000), Inches(2.78), gap, Inches(0.3))
        para(tf, "›", size=18, bold=True, color=RULE, align=PP_ALIGN.CENTER,
             space_after=0, first=True)

_, tf = textbox(s, MARGIN, Inches(4.05), Inches(12.1), Inches(0.4))
para(tf, "설치 파일", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
code_block(s, ["ClaudeCodeMonitor-1.0.0-setup.exe        약 9.5 MB"],
           MARGIN, Inches(4.50), Inches(12.1), size=13)

note_bar(s, "이미 실행 중이면 설치 관리자가 먼저 알립니다. 트레이 아이콘에서 [종료] 한 뒤 다시 시작하십시오.",
         MARGIN, Inches(5.25), Inches(12.1), kind="warn")
note_bar(s, "인터넷에서 받은 파일은 SmartScreen 경고가 뜰 수 있습니다. [추가 정보] > [실행] 으로 진행하십시오.",
         MARGIN, Inches(5.92), Inches(12.1), kind="info")


def step_slide(no, title, image, bullets, note=None, note_kind="info"):
    s, y = page("1. 설치", title)
    fit_image(s, img(image), MARGIN, Inches(1.95), Inches(6.55), Inches(4.15))
    caption(s, f"화면 {no}", MARGIN, Inches(6.25), Inches(6.55))
    bullet_block(s, bullets, Inches(7.5), Inches(2.05), Inches(5.25), Inches(3.6),
                 size=13, gap=11)
    if note:
        note_bar(s, note, Inches(7.5), Inches(5.55), Inches(5.25), kind=note_kind)
    return s


step_slide("①", "1.2  ① 시작 화면", "01-welcome.png", [
    ("무엇을 하는 화면인가  ", "설치를 시작한다는 안내만 보여 줍니다."),
    ("할 일  ", "[다음] 을 누릅니다."),
    ("되돌리기  ", "이 화면에서는 [취소] 로 언제든 그만둘 수 있습니다. 아직 아무것도 바뀌지 않습니다."),
])

step_slide("②", "1.3  ② 라이선스 화면", "02-license.png", [
    ("무엇을 하는 화면인가  ", "GNU 일반 공중 사용 허가서 버전 3(GPLv3) 전문입니다."),
    ("단추가 [동의함] 이 아닌 이유  ",
     "GPLv3 는 프로그램을 쓰기 위해 동의를 받아야 하는 계약이 아닙니다. 보여 주기만 하므로 단추도 [다음] 입니다."),
    ("할 일  ", "[다음] 을 누릅니다. 전문은 설치 폴더에도 LICENSE 파일로 남습니다."),
])

step_slide("③", "1.4  ③ 설치 위치 화면", "03-location.png", [
    ("기본 설치 위치  ", "%LOCALAPPDATA%\\Programs\\ClaudeCodeMonitor"),
    ("바꿔도 되는가  ", "됩니다. 다만 기본값을 권합니다. 관리자 권한이 필요 없는 자리이고, 제거할 때도 그대로 찾아갑니다."),
    ("필요한 공간  ", "약 32 MB. 화면 아래에 남은 공간과 함께 표시됩니다."),
    ("할 일  ", "[설치] 를 누릅니다."),
], note="Program Files 에 설치하지 않는 것은 일부러입니다. 사용자 한 사람에게 매인 프로그램이라 권한 상승이 필요 없습니다.")

step_slide("④", "1.5  ④ 설치 중 화면", "04-installing.png", [
    ("무엇이 일어나는가  ", "실행 파일과 Qt 런타임, C 런타임을 설치 폴더에 풉니다. 보통 수 초면 끝납니다."),
    ("시작 메뉴 바로가기  ", "[Claude Code Monitor] 와 [제거] 두 개가 만들어집니다."),
    ("제어판 등록  ", "[앱 및 기능] 목록에 올라가 나중에 거기서도 제거할 수 있습니다."),
    ("자세히 보기  ", "무엇이 풀리는지 한 줄씩 확인할 수 있습니다."),
])

s = step_slide("⑤", "1.6  ⑤ 완료 화면 — 확인란 두 개", "05-finish.png", [], note=None)
table(s, Inches(7.5), Inches(2.05), Inches(5.25),
      ["확인란", "켜면"],
      [["지금 실행", "설치 관리자를 닫으면서 프로그램을 띄웁니다"],
       ["로그인할 때\n자동으로 실행", "로그인할 때마다 트레이에 자동으로 올라옵니다"]],
      col_w=[Inches(1.75), Inches(3.5)], size=12, row_h=Inches(0.62))
bullet_block(s, [
    "둘 다 기본으로 켜져 있습니다. 권장값입니다.",
    "자동 실행은 나중에 [설정 > 환경 설정] 이나 트레이 우클릭 메뉴에서도 바꿀 수 있습니다.",
    "끄고 마치면 자동 실행 등록을 하지 않습니다.",
], Inches(7.5), Inches(3.95), Inches(5.25), Inches(1.5), size=12.5, gap=9)
note_bar(s, "[마침] 을 누르면 설치가 끝납니다.", Inches(7.5), Inches(5.55), Inches(5.25), kind="ok")

# --- 1.7 설치 후 확인 ---
s, y = page("1. 설치", "1.7  설치 후 확인", "세 군데만 보면 제대로 깔렸는지 알 수 있습니다")
items = [
    ("트레이 아이콘", "작업 표시줄 오른쪽에 사용률 숫자가 적힌 아이콘이 생깁니다. 보이지 않으면 [숨겨진 아이콘 표시(^)] 안에 있습니다.", "31-tray-popup.png"),
    ("시작 메뉴", "[Claude Code Monitor] 폴더 안에 실행과 제거 바로가기가 있습니다.", None),
    ("자동 실행", "작업 관리자 > 시작 앱 목록에 ClaudeCodeMonitor 가 보입니다.", None),
]
bullet_block(s, [
    ("트레이 아이콘  ", "작업 표시줄 오른쪽에 사용률 숫자가 적힌 아이콘이 생깁니다. 보이지 않으면 [숨겨진 아이콘 표시(^)] 안에 있습니다."),
    ("시작 메뉴  ", "[Claude Code Monitor] 폴더 안에 실행 바로가기와 [제거] 가 있습니다."),
    ("자동 실행  ", "작업 관리자 > [시작 앱] 목록에 ClaudeCodeMonitor 가 보입니다. 완료 화면에서 확인란을 켠 경우입니다."),
    ("첫 값이 뜨기까지  ", "프로그램이 뜬 뒤 claude 에게 물어보는 데 1~2초가 걸립니다. 그 사이 값은 비어 있습니다."),
], MARGIN, Inches(1.95), Inches(7.3), Inches(3.0), size=13.5, gap=13)
fit_image(s, img("31-tray-popup.png"), Inches(8.3), Inches(1.95), Inches(4.4), Inches(3.6))
caption(s, "트레이 아이콘을 한 번 누르면 나오는 팝업", Inches(8.3), Inches(5.62), Inches(4.4))
note_bar(s, "설치 직후에는 [훅 미등록] 로 표시됩니다. 터미널 상태줄 표시를 함께 쓰려면 4장을 보십시오. 훅을 등록하지 않아도 사용량 조회와 알림은 그대로 동작합니다.",
         MARGIN, Inches(6.10), Inches(12.1), kind="info")

# --- 1.8 조용한 설치 ---
s, y = page("1. 설치", "1.8  조용한 설치와 재설치", "여러 대에 배포하거나 다시 깔 때")
_, tf = textbox(s, MARGIN, Inches(1.95), Inches(12.1), Inches(0.4))
para(tf, "화면 없이 설치하기", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
code_block(s, [
    "ClaudeCodeMonitor-1.0.0-setup.exe /S",
    "ClaudeCodeMonitor-1.0.0-setup.exe /S /D=C:\\Tools\\ClaudeCodeMonitor",
], MARGIN, Inches(2.40), Inches(12.1), size=13)
bullet_block(s, [
    ("/S  ", "화면 없이 설치합니다. 확인란이 없으므로 자동 실행은 권장값(등록함)으로 처리합니다."),
    ("/D=  ", "설치 위치를 지정합니다. 반드시 맨 마지막에 적고, 따옴표를 쓰지 않습니다."),
], MARGIN, Inches(3.35), Inches(12.1), Inches(0.9), size=13, gap=9)

_, tf = textbox(s, MARGIN, Inches(4.25), Inches(12.1), Inches(0.4))
para(tf, "다시 설치할 때", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
table(s, MARGIN, Inches(4.70), Inches(12.1),
      ["상황", "어떻게 되는가"],
      [["같은 위치에 덮어쓰기", "기존 파일을 덮어씁니다. 설정과 사용률 이력은 그대로 남습니다"],
       ["실행 중인 채로 설치", "설치 관리자가 먼저 알립니다. 트레이에서 종료한 뒤 진행하십시오"],
       ["다른 위치에 설치", "이전 폴더는 남습니다. 먼저 제거하는 편이 깔끔합니다"]],
      col_w=[Inches(3.2), Inches(8.9)], size=12.5)

# ===========================================================================
#  2. 기능 소개
# ===========================================================================
divider("SECTION 2", "기능 소개", ["메인 창", "트레이 상주", "알림과 임계치", "설정", "부가 기능"])

s, y = page("2. 기능 소개", "2.1  메인 창 한눈에", "위에서부터 네 칸입니다")
fit_image(s, img("10-main.png"), MARGIN, Inches(1.90), Inches(4.6), Inches(4.9))
areas = [
    ("① 상태 표시등", "빛 하나가 좌우로 오갑니다. 색은 사용량 수준, 움직임은 프로그램 상태를 말합니다."),
    ("② 사용률 추이 (최근 24시간)", "지표별 꺾은선과 임계치 점선. 쌓아 둔 표본으로 그립니다."),
    ("③ 현재 사용량", "지표별 가로 막대. 색이 알림 수준(정상·알림·경고·위험)을 따릅니다. 재설정 시각이 함께 나옵니다."),
    ("④ 요약", "세 지표의 사용률과 재설정 시각, 플랜, 마지막 업데이트 시각, 자동 조회 간격."),
    ("⑤ 아래쪽 단추 셋", "[지금 조회] · [알림 관리 (N)] · [설정]. N 은 아직 확인하지 않은 알림 건수입니다."),
]
bullet_block(s, areas, Inches(5.55), Inches(2.0), Inches(7.2), Inches(4.2),
             size=13, gap=13)
note_bar(s, "켠 직후에는 표본이 하나뿐이라 선이 그려지지 않습니다. 차트 아래에 그 사실을 알리는 문구가 뜹니다.",
         Inches(5.55), Inches(6.05), Inches(7.2), kind="info")

# --- 2.2 상태 표시등 ---
s, y = page("2. 기능 소개", "2.2  상태 표시등", "색과 움직임이 서로 다른 것을 말합니다")
_, tf = textbox(s, MARGIN, Inches(1.95), Inches(5.9), Inches(0.35))
para(tf, "색 — 사용량 수준", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
table(s, MARGIN, Inches(2.38), Inches(5.9),
      ["색", "뜻"],
      [["파랑", "임계치를 하나도 넘지 않음"],
       ["초록", "알림 임계치를 넘음"],
       ["주황", "경고 임계치를 넘음"],
       ["빨강", "위험 임계치를 넘음"]],
      col_w=[Inches(1.3), Inches(4.6)], size=12.5)

_, tf = textbox(s, Inches(6.85), Inches(1.95), Inches(5.85), Inches(0.35))
para(tf, "움직임 — 프로그램 상태", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
table(s, Inches(6.85), Inches(2.38), Inches(5.85),
      ["움직임", "뜻"],
      [["천천히 오간다", "정상. 옆 글자는 사용량 수준 이름"],
       ["빠르게 오간다", "조회 중"],
       ["주황 세모가 붙는다", "훅 미등록"],
       ["오가지 않고 깜박인다", "조회 실패"]],
      col_w=[Inches(2.1), Inches(3.75)], size=12.5)

note_bar(s, "색에 프로그램 상태를 싣지 않는 이유 — 그러면 주황이 겹칩니다. [훅 미등록] 의 주황과 [경고 도달] 의 주황을 구별할 수 없습니다. 색은 사용량만 말하므로, 화면 어디서든 빨강은 언제나 “사용량이 위험하다” 입니다.",
         MARGIN, Inches(4.60), Inches(12.1), kind="info")
note_bar(s, "기준은 세 지표 중 가장 심각한 것입니다. 트레이 아이콘의 색과 정확히 같은 값을 씁니다.",
         MARGIN, Inches(5.55), Inches(12.1), kind="ok")

# --- 2.3 트레이 ---
s, y = page("2. 기능 소개", "2.3  트레이 상주", "창을 닫아도 트레이에서 계속 지켜봅니다")
fit_image(s, img("31-tray-popup.png"), MARGIN, Inches(1.95), Inches(3.5), Inches(3.5))
caption(s, "한 번 누르면 뜨는 팝업", MARGIN, Inches(5.55), Inches(3.5))
fit_image(s, img("32-tray-menu.png"), Inches(4.35), Inches(1.95), Inches(2.5), Inches(3.5))
caption(s, "우클릭 메뉴", Inches(4.35), Inches(5.55), Inches(2.5))
table(s, Inches(7.2), Inches(1.95), Inches(5.55),
      ["동작", "결과"],
      [["아이콘 보기", "가장 심각한 지표의 사용률이 숫자로 그려집니다"],
       ["마우스 올리기", "세 지표·마지막 갱신·미확인 알림 건수가 즉시 뜹니다"],
       ["한 번 누르기", "팝업이 열리고 그때 새로 조회합니다"],
       ["두 번 누르기", "메인 창을 띄웁니다"],
       ["우클릭", "창 열기 · 지금 조회 · 알림 관리 · 설정 · 자동 실행 · 종료"]],
      col_w=[Inches(1.7), Inches(3.85)], size=12, row_h=Inches(0.52))
note_bar(s, "마우스를 올릴 때는 조회하지 않습니다. 도구 설명은 마지막 조회값을 보여 주고, 실제 조회는 누를 때 합니다.",
         Inches(7.2), Inches(5.10), Inches(5.55), kind="info")
note_bar(s, "단일 인스턴스입니다. 두 번 실행해도 아이콘이 둘로 늘지 않습니다.",
         Inches(7.2), Inches(6.05), Inches(5.55), kind="ok")

# --- 2.4 알림 ---
s, y = page("2. 기능 소개", "2.4  임계치와 알림", "정해 둔 사용률을 넘으면 알려 줍니다")
table(s, MARGIN, Inches(1.95), Inches(6.3),
      ["수준", "아이콘", "쓰임"],
      [["(기본)", "없음 · 파랑", "임계치를 하나도 넘지 않은 상태"],
       ["알림", "느낌표 · 초록", "슬슬 신경 쓸 때"],
       ["경고", "세모 · 주황", "곧 한도에 닿을 때"],
       ["위험", "엑스 · 빨강", "거의 다 썼을 때"]],
      col_w=[Inches(1.5), Inches(2.0), Inches(2.8)], size=12.5)
bullet_block(s, [
    ("지표 하나에 여러 줄  ", "현재 세션 70% 알림 · 85% 경고 · 95% 위험 처럼 층을 둘 수 있습니다."),
    ("한 번 알린 것은 다시 알리지 않습니다  ", "같은 임계치는 재설정 전까지 한 번만 뜹니다."),
    ("한 번에 여러 개를 넘으면  ", "알림은 한 건으로 묶어 띄우고, 목록에는 임계치마다 한 줄씩 남습니다."),
    ("알림을 꺼도  ", "윈도우 알림만 뜨지 않을 뿐, 알림 관리 목록에는 계속 쌓입니다."),
], Inches(7.0), Inches(2.0), Inches(5.75), Inches(3.2), size=13, gap=12)
_, tf = textbox(s, MARGIN, Inches(4.35), Inches(6.3), Inches(0.35))
para(tf, "알림이 가는 곳 두 군데", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
table(s, MARGIN, Inches(4.78), Inches(6.3),
      ["경로", "지우는 주체"],
      [["윈도우 알림 (토스트)", "알림 센터에 쌓이고 OS 가 관리합니다"],
       ["프로그램의 알림 관리 창", "파일에 누적되고 프로그램에서 지웁니다"]],
      col_w=[Inches(2.6), Inches(3.7)], size=12, row_h=Inches(0.45))
note_bar(s, "자리를 비운 동안 뜬 알림도 [알림 관리] 에서 나중에 볼 수 있습니다.",
         Inches(7.0), Inches(5.55), Inches(5.75), kind="ok")

# --- 2.5 설정: 알림 ---
s, y = page("2. 기능 소개", "2.5  설정 — 알림 설정 탭", "언제 알릴 것인가")
fit_image(s, img("20-settings-alerts.png"), MARGIN, Inches(1.90), Inches(5.0), Inches(4.9))
bullet_block(s, [
    ("윈도우 알림 띄우기  ", "끄면 토스트가 뜨지 않습니다. 목록에는 그대로 쌓입니다."),
    ("알림 활성  ", "그 줄의 임계치로 알릴지 정합니다. 지표의 모든 줄을 끄면 그 지표는 알리지 않습니다."),
    ("차트 표시  ", "사용률 추이 차트에 점선으로 그릴 임계치를 고릅니다. 최대 3개까지이며, 세 개가 차면 나머지가 잠깁니다."),
    ("지표 · 사용률 · 수준  ", "어느 지표의 몇 %에서 어떤 수준으로 알릴지 정합니다."),
    ("알림 추가 / 선택 삭제 / 기본값으로  ", "줄을 늘리거나 지우고, 처음 상태로 되돌립니다."),
], Inches(5.95), Inches(2.0), Inches(6.8), Inches(4.0), size=12.5, gap=12)
note_bar(s, "[알림 활성] 과 [차트 표시] 는 서로 상관이 없습니다. 알리지 않으면서 차트에만 그릴 수도 있고 그 반대도 됩니다.",
         Inches(5.95), Inches(5.95), Inches(6.8), kind="info")

# --- 2.6 설정: 환경 ---
s, y = page("2. 기능 소개", "2.6  설정 — 환경 설정 탭", "어떻게 동작할 것인가")
fit_image(s, img("21-settings-env.png"), MARGIN, Inches(1.90), Inches(5.0), Inches(4.9))
bullet_block(s, [
    ("자동 조회 간격  ", "기본 5분. 0 으로 두면 자동 조회를 하지 않습니다(수동 조회만). 최소 1분입니다."),
    ("로그인 시 자동 실행  ", "켜고 [저장] 하면 등록됩니다. 등록될 명령이 화면에 함께 보입니다."),
    ("시작할 때 트레이에서 실행  ", "창을 띄우지 않고 트레이로만 올라옵니다."),
    ("창을 닫을 때 물어보기  ", "닫기(X)를 눌렀을 때 어떻게 할지 묻습니다. 끄면 지금 동작이 한 줄로 표시됩니다."),
    ("statusline 훅  ", "[훅 등록] / [훅 해제] 와 현재 등록 상태. 자세한 것은 4장에 있습니다."),
], Inches(5.95), Inches(2.0), Inches(6.8), Inches(4.0), size=12.5, gap=12)
note_bar(s, "등록된 자동 실행 명령이 지금 실행 파일과 다르면 그 사실을 알려 줍니다. 설치 위치를 옮겼거나 다른 빌드로 등록해 둔 경우입니다.",
         Inches(5.95), Inches(5.95), Inches(6.8), kind="warn")

# --- 2.7 알림 관리 ---
s, y = page("2. 기능 소개", "2.7  알림 관리 창", "지나간 알림을 모아 봅니다")
fit_image(s, img("22-alertlog.png"), MARGIN, Inches(1.95), Inches(7.1), Inches(3.9))
table(s, Inches(8.0), Inches(1.95), Inches(4.75),
      ["단추", "하는 일"],
      [["선택 확인", "고른 줄을 확인 처리 (목록에는 남음)"],
       ["모두 확인", "전부 확인 처리"],
       ["선택 삭제", "고른 줄만 지움"],
       ["전체 삭제", "목록을 비움"]],
      col_w=[Inches(1.5), Inches(3.25)], size=12, row_h=Inches(0.45))
bullet_block(s, [
    "미확인 줄은 굵게 나오고 첫 열에 [미확인] 이라고 적힙니다.",
    "줄을 두 번 누르면 확인 처리됩니다.",
    "최대 500건을 보관하고, 확인한 것부터 버립니다.",
    "미확인 건수는 메인 창의 [알림 관리 (N)] 단추와 트레이 도구 설명에도 나옵니다.",
], Inches(8.0), Inches(4.05), Inches(4.75), Inches(1.8), size=12, gap=9)
note_bar(s, "확인과 삭제는 다른 동작입니다. 확인은 “봤다” 이고 목록에 남으며, 삭제는 목록에서 없앱니다.",
         MARGIN, Inches(6.10), Inches(12.1), kind="info")

# --- 2.8 닫기 동작 ---
s, y = page("2. 기능 소개", "2.8  창을 닫을 때", "트레이 상주 프로그램의 닫기는 뜻이 둘입니다")
fit_image(s, img("33-closechoice.png"), MARGIN, Inches(2.0), Inches(4.3), Inches(3.2))
table(s, Inches(5.3), Inches(2.0), Inches(7.45),
      ["선택", "동작"],
      [["백그라운드에서 계속", "창만 숨기고 트레이에서 계속 조회·알립니다 (기본 선택)"],
       ["완전 종료", "조회와 알림을 모두 멈춥니다"],
       ["취소", "닫지 않습니다. Esc 도 여기로 걸립니다"]],
      col_w=[Inches(2.5), Inches(4.95)], size=12.5, row_h=Inches(0.5))
bullet_block(s, [
    ("다시 묻지 않음  ", "켜면 그때 고른 동작을 기억해 다음부터 바로 그렇게 합니다."),
    ("취소와 함께 켠 체크는 무시합니다  ", "“묻지 말고 앞으로 취소해라” 는 말이 성립하지 않습니다."),
    ("다시 묻게 하려면  ", "[설정 > 환경 설정 > 창을 닫을 때 어떻게 할지 물어보기] 를 켭니다."),
    ("묻지 않는 자리  ", "[파일 > 종료] 와 트레이 메뉴의 [종료] 는 뜻이 분명하므로 묻지 않습니다."),
], Inches(5.3), Inches(3.9), Inches(7.45), Inches(2.2), size=12.5, gap=11)

# --- 2.9 디버그 콘솔 + 데이터 파일 ---
s, y = page("2. 기능 소개", "2.9  디버그 콘솔", "무슨 일이 일어나는지 눈으로 봅니다  ([도구 > 디버그 콘솔])")
fit_image(s, img("36-debugconsole.png"), MARGIN, Inches(1.95), Inches(7.5), Inches(4.0))
bullet_block(s, [
    "조회 한 번의 처음부터 끝까지, 설정을 저장한 내용, 어긋난 구간의 사정이 흐릅니다.",
    "릴리즈 빌드에서도 동작합니다. 개발자 전용 장치가 아닙니다.",
    "같은 내용이 monitor.log 파일에도 쌓입니다.",
    "여는 동안 로그 수준을 DEBUG 로 올리고, 닫으면 되돌립니다.",
], Inches(8.4), Inches(2.05), Inches(4.35), Inches(2.4), size=12.5, gap=11)
note_bar(s, "이 창을 닫으면 프로그램도 함께 종료됩니다. 콘솔을 떼려면 [도구 > 디버그 콘솔] 을 다시 눌러 끄십시오.",
         Inches(8.4), Inches(4.75), Inches(4.35), kind="stop")
note_bar(s, "문제를 문의할 때 이 화면이나 monitor.log 를 함께 보내 주시면 원인을 빨리 찾을 수 있습니다.",
         MARGIN, Inches(6.25), Inches(12.1), kind="info")

s, y = page("2. 기능 소개", "2.10  데이터가 쌓이는 자리", r"모두 %LOCALAPPDATA%\ClaudeCodeMonitor\ 아래에 있습니다")
table(s, MARGIN, Inches(2.05), Inches(12.1),
      ["파일", "내용", "지워지면"],
      [["settings.ini", "조회 간격, 임계치 목록, 알림 설정, 종료 동작", "기본값으로 되돌아갑니다"],
       ["history.json", "사용률 표본. 7일 보관하며 차트가 최근 24시간을 읽습니다", "차트가 비고 다시 쌓입니다"],
       ["alerts.json", "알림 목록. 확인 여부 포함, 최대 500건", "지나간 알림이 사라집니다"],
       ["snapshot.json", "statusline 훅이 남긴 원본 자료", "훅이 다시 만듭니다"],
       ["monitor.log", "실행 로그", "다시 쌓입니다"]],
      col_w=[Inches(2.3), Inches(6.5), Inches(3.3)], size=12.5)
note_bar(s, "설정을 레지스트리가 아니라 INI 파일로 두는 이유 — 제거할 때 지우기 쉽고, 문제가 생겼을 때 사용자가 직접 열어 볼 수 있기 때문입니다. 자동 실행 등록만 OS 가 정한 자리(레지스트리)를 씁니다.",
         MARGIN, Inches(5.05), Inches(12.1), kind="info")
note_bar(s, "이 폴더는 프로그램을 지워도 물어본 뒤에만 지워집니다. 3장을 보십시오.",
         MARGIN, Inches(5.95), Inches(12.1), kind="warn")
