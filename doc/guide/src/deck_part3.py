# -*- coding: utf-8 -*-
# 3. 제거  ~  5. 기타

# ===========================================================================
#  3. 제거
# ===========================================================================
divider("SECTION 3", "제거", ["제거하는 세 가지 방법", "화면별 선택지", "지워지는 것과 남는 것"])

s, y = page("3. 제거", "3.1  제거하는 방법", "어느 길로 들어가도 같은 제거 관리자가 뜹니다")
ways = [
    ("①", "시작 메뉴", "[Claude Code Monitor] > [제거]"),
    ("②", "설정 앱", "설정 > 앱 > 설치된 앱 > Claude Code Monitor > 제거"),
    ("③", "설치 폴더", "설치 폴더의 uninstall.exe 를 직접 실행"),
]
for i, (no, t, d) in enumerate(ways):
    yy = Inches(2.0) + Inches(0.82) * i
    rect(s, MARGIN, yy, Inches(12.1), Inches(0.70), fill=BG_SOFT)
    rect(s, MARGIN, yy, Inches(0.055), Inches(0.70), fill=ACCENT)
    _, tf = textbox(s, MARGIN + Inches(0.28), yy + Inches(0.17), Inches(0.5), Inches(0.4))
    para(tf, no, size=17, bold=True, color=ACCENT, space_after=0, first=True)
    _, tf = textbox(s, MARGIN + Inches(0.85), yy + Inches(0.09), Inches(2.6), Inches(0.4))
    para(tf, t, size=15, bold=True, color=PRIMARY, space_after=0, first=True)
    _, tf = textbox(s, MARGIN + Inches(3.5), yy + Inches(0.19), Inches(8.4), Inches(0.4))
    para(tf, d, size=13, color=INK, space_after=0, first=True)

_, tf = textbox(s, MARGIN, Inches(4.65), Inches(12.1), Inches(0.4))
para(tf, "제거 순서", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
steps = [("①", "제거 확인"), ("②", "statusline 훅"), ("③", "사용자 데이터"), ("④", "완료")]
bw = Inches(2.85)
for i, (no, t) in enumerate(steps):
    x = MARGIN + (bw + Inches(0.22)) * i
    rect(s, x, Inches(5.12), bw, Inches(0.78), fill=BG_SOFT)
    rect(s, x, Inches(5.12), bw, Inches(0.05), fill=ACCENT)
    _, tf = textbox(s, x + Inches(0.22), Inches(5.32), bw - Inches(0.4), Inches(0.4))
    rich(tf, [(no + "  ", {"bold": True, "color": ACCENT, "size": 15}),
              (t, {"bold": True, "color": PRIMARY, "size": 14})], space_after=0, first=True)

note_bar(s, "실행 중이면 제거 관리자가 먼저 알립니다. 트레이 아이콘에서 [종료] 한 뒤 [확인] 을 누르십시오.",
         MARGIN, Inches(6.15), Inches(12.1), kind="warn")

# --- 3.2 ① ② ---
s, y = page("3. 제거", "3.2  ① 제거 확인 · ② statusline 훅")
fit_image(s, img("51-confirm.png"), MARGIN, Inches(2.0), Inches(5.9), Inches(3.3))
caption(s, "① 무엇을 어디서 지울지 확인합니다", MARGIN, Inches(5.42), Inches(5.9))
fit_image(s, img("52-hookpage.png"), Inches(6.85), Inches(2.0), Inches(5.9), Inches(3.3))
caption(s, "② statusline 훅을 함께 지울지 고릅니다", Inches(6.85), Inches(5.42), Inches(5.9))
note_bar(s, "② 화면은 지금 Claude Code 설정에 무엇이 등록되어 있는지 그대로 보여 줍니다. 무엇을 지우는지 모르고 누르지 않도록, 등록된 명령을 화면에 적습니다.",
         MARGIN, Inches(5.90), Inches(12.1), kind="info")

s, y = page("3. 제거", "3.3  ② statusline 훅 화면이 고르는 것",
            "지금 등록되어 있는 것에 따라 화면의 말과 확인란이 달라집니다")
table(s, MARGIN, Inches(2.0), Inches(7.2),
      ["지금 등록된 것", "화면이 하는 말", "확인란"],
      [["없음", "지울 것이 없습니다", "꺼짐 · 잠김"],
       ["이 설치본이 등록한 것", "이 프로그램이 등록한 항목입니다", "켜짐"],
       ["다른 복사본 (파일 없음)", "없는 파일을 가리켜 상태줄이 빈칸으로 보입니다", "켜짐"],
       ["다른 복사본 (파일 있음)", "지금 이 설치본이 등록한 것이 아닙니다", "꺼짐"],
       ["다른 프로그램의 설정", "건드리지 않습니다", "꺼짐 · 잠김"]],
      col_w=[Inches(2.5), Inches(3.7), Inches(1.0)], size=11.5, row_h=Inches(0.42))
fit_image(s, img("56-hookpage-othercopy.png"), Inches(8.05), Inches(2.0), Inches(4.7), Inches(2.9))
caption(s, "다른 복사본이 등록되어 있을 때", Inches(8.05), Inches(5.02), Inches(4.7))
bullet_block(s, [
    ("없는 파일을 가리키는 등록은 그냥 지웁니다  ",
     "되살릴 방법이 없고, 남겨 두면 Claude Code 의 상태줄이 이유 없이 빈칸으로 보입니다."),
    ("살아 있는 다른 복사본은 기본으로 두지 않습니다  ",
     "그쪽은 지금도 동작 중인 설정입니다. 경로를 보고 직접 켜야 지웁니다."),
    ("남의 설정은 파일을 열지도 않습니다  ",
     "제거 관리자가 남의 설정 파일을 말없이 고쳐서는 안 됩니다."),
], MARGIN, Inches(4.82), Inches(7.2), Inches(2.0), size=12, gap=10)

# --- 3.3 ③ ④ ---
s, y = page("3. 제거", "3.4  ③ 사용자 데이터 · ④ 완료")
fit_image(s, img("53-userdata.png"), MARGIN, Inches(2.05), Inches(3.9), Inches(1.9))
caption(s, "③ 설정과 사용률 이력을 지울지 묻습니다", MARGIN, Inches(4.05), Inches(3.9))
fit_image(s, img("54-notice.png"), Inches(4.6), Inches(2.05), Inches(2.2), Inches(1.9))
caption(s, "④ 끝났다는 안내", Inches(4.6), Inches(4.05), Inches(2.2))
fit_image(s, img("55-done.png"), Inches(7.2), Inches(2.05), Inches(5.55), Inches(3.1))
caption(s, "④ 무엇을 지웠는지 한 줄씩 남습니다", Inches(7.2), Inches(5.22), Inches(5.55))
table(s, MARGIN, Inches(4.45), Inches(6.35),
      ["③ 의 선택", "결과"],
      [["예", "설정·이력·알림 목록을 모두 지웁니다"],
       ["아니요", "그대로 남깁니다. 다시 설치하면 이어서 씁니다"]],
      col_w=[Inches(1.5), Inches(4.85)], size=12, row_h=Inches(0.46))
note_bar(s, "되돌릴 수 없는 쪽이므로, 다시 설치할 생각이 조금이라도 있으면 [아니요] 를 권합니다.",
         MARGIN, Inches(5.95), Inches(6.35), kind="warn")

# --- 3.4 무엇이 지워지나 ---
s, y = page("3. 제거", "3.5  지워지는 것과 남는 것", "제거 관리자는 자기가 만든 것만 지웁니다")
table(s, MARGIN, Inches(2.0), Inches(12.1),
      ["대상", "어떻게", "비고"],
      [["설치한 파일", "지웁니다", "설치 폴더를 통째로 지우지 않고 설치한 파일만 이름으로 지웁니다"],
       ["시작 메뉴 바로가기", "지웁니다", ""],
       ["로그인 시 자동 실행 등록", "지웁니다", "없는 파일을 가리키는 항목이 시작 앱 목록에 남지 않도록"],
       ["시작 프로그램 켜고 끈 기록", "지웁니다", "작업 관리자에서 껐던 기록. Run 값과 짝입니다"],
       ["트레이 아이콘 표시 설정", "지웁니다", "우리 실행 파일을 가리키는 항목만. 다른 프로그램 것은 건드리지 않습니다"],
       ["제어판 등록 정보", "지웁니다", "[설치된 앱] 목록에서 사라집니다"],
       ["설정 · 이력 · 알림 목록", "물어봅니다", "③ 에서 고릅니다"],
       ["statusline 훅 등록", "골라서", "② 에서 고릅니다"]],
      col_w=[Inches(3.3), Inches(1.7), Inches(7.1)], size=11.5, row_h=Inches(0.37))
note_bar(s, "트레이 아이콘 표시 설정을 지운 결과는 다시 로그인해야 [설정 > 개인 설정 > 작업 표시줄] 목록에 반영됩니다. 그 목록은 탐색기가 메모리에 들고 있는 것을 그리기 때문입니다.",
         MARGIN, Inches(5.55), Inches(12.1), kind="info")
_, tf = textbox(s, MARGIN, Inches(6.42), Inches(12.1), Inches(0.5))
para(tf, "조용한 제거", size=13, bold=True, color=PRIMARY, space_after=4, first=True)
rich(tf, [("uninstall.exe /S", {"face": MONO, "bold": True}),
          ("  — 화면이 없으므로 statusline 훅과 사용자 데이터는 모두 ", {}),
          ("남깁니다", {"bold": True}),
          (". 되돌릴 수 없는 쪽을 묻지 않고 하지 않기 위해서입니다.", {})],
     size=12, space_after=0)

# ===========================================================================
#  4. Claude Code CLI 지원
# ===========================================================================
divider("SECTION 4", "Claude Code CLI 지원",
        ["터미널 아래 상태줄", "훅 등록과 해제", "값의 출처와 신선도"])

s, y = page("4. CLI 지원", "4.1  터미널 아래에 한 줄", "Claude Code 를 쓰는 동안 눈을 떼지 않고 사용률을 봅니다")
fit_image(s, img("41-cli-statusline.png"), MARGIN, Inches(2.0), Inches(12.1), Inches(1.15),
          border=False)
caption(s, "Claude Code 화면 아래에 나오는 상태줄", MARGIN, Inches(3.30), Inches(12.1))
table(s, MARGIN, Inches(3.80), Inches(12.1),
      ["조각", "어디서 오는가", "얼마나 신선한가"],
      [["Session", "Claude Code 가 훅에게 넘기는 자료의 5시간 창", "상태줄이 갱신될 때마다 (약 5초)"],
       ["Model", "같은 자료의 주간 창 (모든 모델)", "위와 같음"],
       ["Fable 등", "트레이 본체가 남겨 둔 모델별 값", "본체의 조회 간격만큼 (기본 5분)"]],
      col_w=[Inches(2.2), Inches(6.4), Inches(3.5)], size=12.5)
note_bar(s, "색이 들어갑니다. 사용률이 오를수록 진해지며, 검은 바탕에서 읽기 좋도록 채도를 낮춰 잡았습니다.",
         MARGIN, Inches(5.45), Inches(12.1), kind="info")
note_bar(s, "세 번째 조각(모델별)은 트레이 본체가 켜져 있을 때만 나옵니다. 본체를 끄면 그 조각이 사라지고 앞의 두 개만 남습니다.",
         MARGIN, Inches(6.12), Inches(12.1), kind="warn")

s, y = page("4. CLI 지원", "4.2  훅 등록하기", "프로그램에서 한 번 누르면 끝납니다")
bullet_block(s, [
    ("어디서 누르는가  ", "[도구 > statusline 훅 등록] 또는 [설정 > 환경 설정 > statusline 훅 > 훅 등록]"),
    ("이미 다른 설정이 있으면  ", "무엇이 걸려 있는지 보여 주고 물어봅니다. 덮어쓰기 전에 확인할 수 있습니다."),
    ("백업  ", "고치기 전에 settings.json.bak.<타임스탬프> 로 백업합니다."),
    ("적용 시점  ", "Claude Code 는 세션을 시작할 때 설정을 읽습니다. 등록한 뒤에는 Claude Code 를 다시 시작해야 상태줄이 나옵니다."),
], MARGIN, Inches(2.0), Inches(5.6), Inches(3.0), size=13, gap=13)
fit_image(s, img("34-hook-confirm.png"), Inches(6.4), Inches(1.95), Inches(6.35), Inches(1.9))
caption(s, "이미 다른 설정이 있을 때", Inches(6.4), Inches(3.90), Inches(6.35))
fit_image(s, img("35-hook-done.png"), Inches(6.4), Inches(4.30), Inches(6.35), Inches(2.0))
caption(s, "등록이 끝나면 무엇을 어디에 적었는지 알려 줍니다", Inches(6.4), Inches(6.35), Inches(6.35))
note_bar(s, "훅은 Claude Code 의 설정 파일을 건드리는 유일한 기능입니다. 그래서 확인 없이는 아무것도 고치지 않습니다.",
         MARGIN, Inches(5.25), Inches(5.6), kind="info")

s, y = page("4. CLI 지원", "4.3  등록되는 내용과 해제", "무엇이 적히고, 어떻게 지우는가")
_, tf = textbox(s, MARGIN, Inches(1.95), Inches(12.1), Inches(0.4))
para(tf, "적히는 자리와 내용", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
code_block(s, [
    r"%USERPROFILE%\.claude\settings.json",
    "",
    '{',
    '  "statusLine": {',
    '    "type": "command",',
    '    "command": "\\"C:/Users/.../ClaudeCodeMonitor/ccm_probe.exe\\" --color"',
    '  }',
    '}',
], MARGIN, Inches(2.40), Inches(6.9), size=11.5)
bullet_block(s, [
    ("경로를 슬래시로 적는 이유  ", "Claude Code 는 상태줄 명령을 bash 에 넘깁니다. 역슬래시를 그대로 적으면 bash 가 이스케이프로 먹어 명령을 찾지 못합니다."),
    ("CLAUDE_CONFIG_DIR  ", "이 환경변수가 있으면 그 경로의 settings.json 을 씁니다."),
], MARGIN, Inches(5.05), Inches(6.9), Inches(1.6), size=12, gap=10)

_, tf = textbox(s, Inches(7.7), Inches(1.95), Inches(5.05), Inches(0.4))
para(tf, "해제 — 우리 것만 지웁니다", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
table(s, Inches(7.7), Inches(2.40), Inches(5.05),
      ["등록된 것", "하는 일"],
      [["없음", "알리고 끝"],
       ["우리 ccm_probe", "확인받고 지웁니다"],
       ["다른 복사본 · 있음", "그 사실을 밝히고 확인받은 뒤 지웁니다"],
       ["다른 복사본 · 없음", "묻지 않고 지웁니다"],
       ["남의 명령", "파일을 열지도 않습니다. 알리기만 합니다"]],
      col_w=[Inches(1.75), Inches(3.3)], size=11.5, row_h=Inches(0.44))
bullet_block(s, [
    "[설정 > 환경 설정 > 훅 해제] 를 누르거나,",
    "터미널에서 ccm_probe --unregister 를 실행합니다.",
], Inches(7.7), Inches(5.20), Inches(5.05), Inches(0.9), size=12, gap=9)
note_bar(s, "가리키는 파일이 이미 없는 등록은 묻지 않고 지웁니다. 되살릴 방법이 없기 때문입니다.",
         Inches(7.7), Inches(6.15), Inches(5.05), kind="ok")

s, y = page("4. CLI 지원", "4.4  실제 터미널 화면", "Claude Code 를 띄우면 아래쪽에 한 줄이 붙습니다")
fit_image(s, img("40-cli-full.png"), MARGIN, Inches(1.95), Inches(8.0), Inches(4.6))
bullet_block(s, [
    ("상태줄이 비어 있다면  ", "훅을 등록한 뒤 Claude Code 를 다시 시작했는지 확인하십시오."),
    ("모델별 조각이 없다면  ", "트레이 본체가 꺼져 있거나, 켠 뒤 아직 한 번도 조회하지 않은 것입니다."),
    ("값이 낡았다면  ", "본체의 조회 간격(기본 5분)만큼 늦습니다. [지금 조회] 를 누르면 바로 새로 고쳐집니다."),
    ("훅은 본체 없이도 돕니다  ", "본체를 지워도 세션과 모든 모델 두 조각은 계속 나옵니다."),
], Inches(8.85), Inches(2.05), Inches(3.9), Inches(4.0), size=12, gap=12)

# ===========================================================================
#  5. 기타
# ===========================================================================
divider("SECTION 5", "기타", ["라이선스", "문제 해결", "사양과 제한", "문의"])

s, y = page("5. 기타", "5.1  라이선스", "GNU General Public License v3.0 only")
fit_image(s, img("23-about.png"), MARGIN, Inches(1.95), Inches(5.4), Inches(4.3))
caption(s, "[도움말 > ClaudeCodeMonitor 정보]", MARGIN, Inches(6.32), Inches(5.4))
bullet_block(s, [
    ("자유 소프트웨어입니다  ", "쓰고, 복제하고, 고치고, 다시 배포할 수 있습니다."),
    ("다시 배포할 때  ", "GPLv3 전문(LICENSE)을 함께 주어야 합니다. 설치 폴더와 zip 묶음 모두에 들어 있습니다."),
    ("고쳐서 배포할 때  ", "고친 것도 같은 GPLv3 로 내야 합니다."),
    ("보증  ", "쓸모가 있기를 바라며 배포되지만 어떠한 보증도 하지 않습니다."),
    ("전문 보기  ", "정보 창의 [라이선스 전문] 단추를 누르면 실행 파일에 실린 전문이 열립니다."),
], Inches(6.25), Inches(2.05), Inches(6.5), Inches(3.8), size=12.5, gap=12)
note_bar(s, "GPLv3 를 고른 것은 선택이 아니라 의무였습니다. 이 프로그램은 Qt 를 씁니다.",
         Inches(6.25), Inches(6.05), Inches(6.5), kind="info")

s, y = page("5. 기타", "5.2  문제 해결", "자주 묻는 것")
table(s, MARGIN, Inches(1.95), Inches(12.1),
      ["증상", "원인", "해결"],
      [["값이 채워지지 않고 [조회 실패]", "claude 를 찾지 못하거나 로그인되어 있지 않음",
        "터미널에서 claude --version 과 로그인 상태를 확인하십시오"],
       ["차트에 선이 없다", "켠 직후라 표본이 하나뿐",
        "몇 분 두면 그려집니다. 이력은 7일 보관합니다"],
       ["[훅 미등록] 이 계속 뜬다", "statusline 훅을 등록하지 않음",
        "[도구 > statusline 훅 등록]. 등록하지 않아도 조회와 알림은 동작합니다"],
       ["터미널 상태줄이 비어 있다", "등록 후 Claude Code 를 다시 시작하지 않음",
        "Claude Code 는 세션 시작 때 설정을 읽습니다. 다시 시작하십시오"],
       ["트레이 아이콘이 안 보인다", "Windows 11 이 숨김 영역에 넣음",
        "[숨겨진 아이콘 표시(^)] 를 누르거나, 작업 표시줄 설정에서 항상 보이게 하십시오"],
       ["폴더를 옮겼더니 자동 실행이 깨졌다", "등록된 명령이 절대 경로",
        "설정 창에서 자동 실행을 껐다 켜고, 훅도 다시 등록하십시오"],
       ["알림이 뜨지 않는다", "윈도우 알림을 껐거나 집중 지원 중",
        "[설정 > 알림 설정 > 윈도우 알림 띄우기] 와 Windows 알림 설정을 확인하십시오"]],
      col_w=[Inches(3.4), Inches(3.5), Inches(5.2)], size=11.5, row_h=Inches(0.52))
note_bar(s, "원인을 찾기 어려우면 [도구 > 디버그 콘솔] 을 켜고 [지금 조회] 를 눌러 보십시오. 같은 내용이 monitor.log 에도 남습니다.",
         MARGIN, Inches(6.15), Inches(12.1), kind="info")

s, y = page("5. 기타", "5.3  사양과 제한", "알아 두면 좋은 값들")
table(s, MARGIN, Inches(2.0), Inches(5.95),
      ["항목", "값"],
      [["기본 조회 간격", "5분"],
       ["조회 간격 범위", "1분 ~ 720분 (0 = 자동 조회 안 함)"],
       ["조회 1회 소요", "1~2초"],
       ["토큰 비용", "없음 (모델을 호출하지 않음)"],
       ["차트가 보는 구간", "최근 24시간"],
       ["이력 보관", "7일"]],
      col_w=[Inches(2.5), Inches(3.45)], size=12, row_h=Inches(0.42))
table(s, Inches(6.85), Inches(2.0), Inches(5.9),
      ["항목", "값"],
      [["알림 목록 보관", "최대 500건 (확인한 것부터 버림)"],
       ["차트 임계치 점선", "최대 3개"],
       ["설치 크기", "약 32 MB"],
       ["관리자 권한", "필요 없음"],
       ["인스턴스", "하나만 (두 번 실행해도 하나)"],
       ["언어", "한국어 · 영어 (OS 설정을 따름)"]],
      col_w=[Inches(2.5), Inches(3.4)], size=12, row_h=Inches(0.42))
_, tf = textbox(s, MARGIN, Inches(5.12), Inches(12.1), Inches(0.4))
para(tf, "알아 둘 제한", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
bullet_block(s, [
    "과거 값은 프로그램이 켜져 있는 동안 쌓은 것뿐입니다. 꺼 둔 동안의 사용량은 차트에 남지 않습니다.",
    "모델별 주간 값은 트레이 본체가 켜져 있을 때만 얻을 수 있습니다.",
    "get_usage 는 Claude Code SDK 의 내부 규약이라 Claude Code 판이 바뀌면 달라질 수 있습니다. 실패해도 상태줄 경로는 그대로 동작합니다.",
    "Windows 전용입니다. 트레이 상주와 레지스트리 등록이 Windows 방식에 맞춰져 있습니다.",
], MARGIN, Inches(5.57), Inches(12.1), Inches(1.5), size=12.5, gap=8)

# --- 끝 ---
PAGE["n"] += 1
s = blank(prs)
rect(s, Inches(0), Inches(0), SLIDE_W, SLIDE_H, fill=PRIMARY)
_, tf = textbox(s, Inches(1.1), Inches(2.5), Inches(11), Inches(1.2))
para(tf, "문의", size=15, bold=True, color=RGBColor(0x8F, 0xC2, 0xF5), space_after=12, first=True)
para(tf, "ilkweon.ban@innogrid.com", size=34, bold=True, color=WHITE, space_after=0)
rect(s, Inches(1.1), Inches(4.15), Inches(1.4), Emu(9525 * 3), fill=ACCENT)
_, tf = textbox(s, Inches(1.1), Inches(4.50), Inches(11), Inches(1.6))
para(tf, "Claude Code 사용량 모니터  v1.0.0", size=17, color=RGBColor(0xCF, 0xE2, 0xF7),
     space_after=8, first=True)
para(tf, "Copyright (C) 2026 Innogrid Co., Ltd.  ·  GNU General Public License v3.0 only",
     size=13, color=RGBColor(0x9F, 0xC4, 0xE8), space_after=0)

prs.save(OUT)
print("저장:", OUT)
print("슬라이드 수:", len(prs.slides.__iter__.__self__._sldIdLst))
