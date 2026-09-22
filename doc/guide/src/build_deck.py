# -*- coding: utf-8 -*-
"""Claude Code 사용량 모니터 v1.0.0 사용자 가이드 (PowerPoint)."""
import os
import sys
from pptx.util import Inches, Pt, Emu
from pptx.enum.text import PP_ALIGN

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from deck_lib import *   # noqa

IMG = r"C:\ws\qt\ClaudeCodeMonitor\doc\guide\images"
OUT = r"C:\ws\qt\ClaudeCodeMonitor\doc\ClaudeCodeMonitor-사용자가이드-v1.0.pptx"


def img(name):
    return os.path.join(IMG, name)


prs = new_deck()
PAGE = {"n": 0}


def page(section, title, sub=None):
    PAGE["n"] += 1
    s = blank(prs)
    y = page_frame(s, section, title, sub, PAGE["n"])
    return s, y


# ===========================================================================
#  표지
# ===========================================================================
s = blank(prs)
rect(s, Inches(0), Inches(0), SLIDE_W, SLIDE_H, fill=PRIMARY)
rect(s, Inches(0), Inches(5.55), SLIDE_W, Inches(1.95), fill=RGBColor(0x14, 0x3C, 0x69))
_, tf = textbox(s, Inches(1.1), Inches(2.05), Inches(11), Inches(0.5))
para(tf, "CLAUDE CODE USAGE MONITOR", size=14, bold=True,
     color=RGBColor(0x8F, 0xC2, 0xF5), space_after=10, first=True)
_, tf = textbox(s, Inches(1.1), Inches(2.55), Inches(11.2), Inches(1.5))
para(tf, "Claude Code 사용량 모니터", size=48, bold=True, color=WHITE,
     space_after=6, first=True)
para(tf, "사용자 가이드", size=30, bold=False, color=RGBColor(0xCF, 0xE2, 0xF7),
     space_after=0)
rect(s, Inches(1.1), Inches(4.55), Inches(1.4), Emu(9525 * 3), fill=ACCENT)
_, tf = textbox(s, Inches(1.1), Inches(4.85), Inches(11), Inches(0.5))
para(tf, "버전 1.0.0   ·   Windows 10 / 11 (64bit)", size=16,
     color=RGBColor(0xCF, 0xE2, 0xF7), space_after=0, first=True)
_, tf = textbox(s, Inches(1.1), Inches(6.05), Inches(11), Inches(1.0))
para(tf, "Innogrid Co., Ltd.", size=18, bold=True, color=WHITE, space_after=4, first=True)
para(tf, "GNU General Public License v3.0 only   ·   2026", size=12,
     color=RGBColor(0x9F, 0xC4, 0xE8), space_after=0)

# ===========================================================================
#  목차
# ===========================================================================
s, y = page("목차", "이 문서의 구성")
toc = [
    ("0. 개요", "개발 배경, 프로그램 구성, 동작 방식, 설치 전 준비"),
    ("1. 설치", "설치 순서와 각 화면, 설치 후 확인, 조용한 설치"),
    ("2. 기능 소개", "메인 창, 트레이 상주, 알림, 설정, 알림 관리, 디버그 콘솔"),
    ("3. 제거", "제거 방법과 제거 중 고르는 선택지, 지워지는 것과 남는 것"),
    ("4. Claude Code CLI 지원", "터미널 상태줄 표시, 훅 등록과 해제"),
    ("5. 기타", "라이선스, 문제 해결, 사양과 제한, 문의"),
]
top = Inches(1.95)
for i, (t, d) in enumerate(toc):
    yy = top + Inches(0.80) * i
    rect(s, MARGIN, yy, Inches(12.1), Inches(0.68), fill=BG_SOFT)
    rect(s, MARGIN, yy, Inches(0.055), Inches(0.68), fill=ACCENT)
    _, tf = textbox(s, MARGIN + Inches(0.30), yy + Inches(0.10), Inches(3.2), Inches(0.3))
    para(tf, t, size=16, bold=True, color=PRIMARY, space_after=0, first=True)
    _, tf = textbox(s, MARGIN + Inches(3.55), yy + Inches(0.14), Inches(8.3), Inches(0.3))
    para(tf, d, size=13, color=INK_SOFT, space_after=0, first=True)


# ===========================================================================
#  구역 표지
# ===========================================================================
def divider(no, title, lines):
    PAGE["n"] += 1
    s = blank(prs)
    rect(s, Inches(0), Inches(0), SLIDE_W, SLIDE_H, fill=PRIMARY)
    rect(s, Inches(1.1), Inches(2.55), Inches(0.9), Emu(9525 * 4), fill=ACCENT)
    _, tf = textbox(s, Inches(1.1), Inches(1.85), Inches(11), Inches(0.6))
    para(tf, no, size=15, bold=True, color=RGBColor(0x8F, 0xC2, 0xF5),
         space_after=0, first=True)
    _, tf = textbox(s, Inches(1.1), Inches(2.85), Inches(11), Inches(1.0))
    para(tf, title, size=40, bold=True, color=WHITE, space_after=0, first=True)
    _, tf = textbox(s, Inches(1.12), Inches(4.05), Inches(10.5), Inches(1.6))
    for i, ln in enumerate(lines):
        para(tf, "— " + ln, size=15, color=RGBColor(0xCF, 0xE2, 0xF7),
             space_after=8, first=(i == 0))
    _, tf = textbox(s, Inches(11.9), Inches(7.02), Inches(0.9), Inches(0.3))
    para(tf, str(PAGE["n"]), size=10, color=RGBColor(0x8F, 0xC2, 0xF5),
         align=PP_ALIGN.RIGHT, space_after=0, first=True)
    return s


# ===========================================================================
#  0. 개요
# ===========================================================================
divider("SECTION 0", "개요", ["개발 배경", "프로그램 구성", "동작 방식", "설치 전 준비"])

s, y = page("0. 개요", "0.1  개발 배경", "왜 만들었는가")
bullet_block(s, [
    ("한도는 있는데 눈에 보이지 않았다.  ",
     "Claude Code 는 5시간 세션 한도와 주간 한도를 쓰지만, 작업 중에는 얼마나 썼는지 알 길이 없습니다. "
     "한도에 걸린 뒤에야 알게 되고, 그때는 이미 작업이 끊깁니다."),
    ("확인하려면 손이 갔다.  ",
     "브라우저의 Claude 앱을 열어 사용량 화면까지 들어가야 합니다. 코드를 쓰다 말고 할 일은 아닙니다."),
    ("모델별 주간 한도는 특히 안 보였다.  ",
     "세션과 전체 주간은 터미널 상태줄에 올릴 수 있지만, 모델별 주간 한도(예: Fable)는 "
     "Claude Code 가 상태줄로 넘기는 자료에 아예 들어 있지 않습니다."),
], MARGIN, Inches(1.95), Inches(12.1), Inches(2.2), size=14, gap=13)

note_bar(s, "그래서 트레이에 상주하면서 세 지표를 스스로 조회하고, 한도에 가까워지면 먼저 알려 주는 프로그램을 만들었습니다.",
         MARGIN, Inches(4.35), Inches(12.1), kind="info")

_, tf = textbox(s, MARGIN, Inches(5.15), Inches(12.1), Inches(0.4))
para(tf, "만들면서 지킨 것", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
bullet_block(s, [
    "토큰을 쓰지 않는다 — 사용량 조회는 모델을 호출하지 않으므로 요금이 붙지 않습니다.",
    "관리자 권한을 요구하지 않는다 — 사용자 폴더에만 설치하고 씁니다.",
    "남의 설정을 말없이 고치지 않는다 — Claude Code 의 설정 파일은 물어보고, 고치기 전에 백업합니다.",
], MARGIN, Inches(5.60), Inches(12.1), Inches(1.2), size=13, gap=7)

# --- 0.2 구성 ---
s, y = page("0. 개요", "0.2  프로그램 구성", "실행 파일 두 개로 나뉩니다")
box_w = Inches(5.85)
for i, (name, role, items, tint) in enumerate([
    ("ClaudeCodeMonitor.exe", "트레이 상주 본체",
     ["claude 를 띄워 세 지표를 주기적으로 조회한다",
      "추이 차트와 게이지로 보여 준다",
      "임계치를 넘으면 윈도우 알림을 띄운다",
      "로그인할 때 자동으로 뜬다"], ACCENT),
    ("ccm_probe.exe", "Claude Code statusline 훅",
     ["Claude Code 가 넘긴 자료를 읽는다",
      "스냅샷 파일로 남긴다",
      "터미널 아래에 한 줄을 그린다",
      "본체가 없어도 혼자 동작한다"], GREEN),
]):
    x = MARGIN + (box_w + Inches(0.4)) * i
    rect(s, x, Inches(2.0), box_w, Inches(2.55), fill=BG_SOFT)
    rect(s, x, Inches(2.0), box_w, Inches(0.06), fill=tint)
    _, tf = textbox(s, x + Inches(0.30), Inches(2.28), box_w - Inches(0.6), Inches(0.4))
    para(tf, name, size=17, bold=True, color=PRIMARY, face=MONO, space_after=2, first=True)
    para(tf, role, size=12, color=INK_SOFT, space_after=0)
    bullet_block(s, items, x + Inches(0.30), Inches(3.15), box_w - Inches(0.6),
                 Inches(1.6), size=12.5, gap=8, marker_color=tint)

note_bar(s, "둘은 따로 돕니다. 훅만 등록해도 상태줄은 나오고, 본체만 켜도 트레이 감시는 동작합니다. 함께 쓰면 상태줄에 모델별 값까지 붙습니다.",
         MARGIN, Inches(4.72), Inches(12.1), kind="info")

table(s, MARGIN, Inches(5.42), Inches(12.1),
      ["보는 지표", "무엇", "주기"],
      [["현재 세션", "5시간 단위 세션 한도 사용률", "5시간마다 재설정"],
       ["모든 모델 (주간)", "일주일 동안 모든 모델을 합친 사용률", "주 단위 재설정"],
       ["모델별 (주간)", "특정 모델의 주간 사용률 (예: Fable)", "주 단위 재설정"]],
      col_w=[Inches(2.6), Inches(6.4), Inches(3.1)], size=12, row_h=Inches(0.36))

# --- 0.3 동작 방식 ---
s, y = page("0. 개요", "0.3  동작 방식", "사용량을 얻는 길이 둘이고, 얻을 수 있는 지표 수가 다릅니다")
table(s, MARGIN, Inches(2.0), Inches(12.1),
      ["경로", "누가 부르는가", "얻는 지표", "비용", "신선도"],
      [["① statusline 훅", "Claude Code 가 ccm_probe 를 부른다", "2개 (세션·모든 모델)",
        "없음 (파일 한 번 쓰기)", "약 5초마다"],
       ["② claude CLI 조회", "본체가 claude 를 띄워 물어본다", "3개 이상 (모델별 포함)",
        "1~2초, 토큰 비용 없음", "조회 간격만큼 (기본 5분)"]],
      col_w=[Inches(2.2), Inches(3.5), Inches(2.5), Inches(2.2), Inches(1.7)], size=12)

_, tf = textbox(s, MARGIN, Inches(3.55), Inches(12.1), Inches(0.4))
para(tf, "왜 두 길이 필요한가", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
bullet_block(s, [
    ("모델별 주간 한도는 ①에 없습니다.  ",
     "Claude Code 가 상태줄로 넘기는 자료에는 세션과 전체 주간 둘뿐입니다. 프로그램이 버리는 것이 아니라 처음부터 들어오지 않습니다."),
    ("①은 요청할 수 없습니다.  ",
     "Claude Code 가 줄 때만 받는 구조라, 사용자가 보고 싶을 때 값을 당겨올 수 없습니다. 그래서 본체는 ②로 직접 물어봅니다."),
    ("그래서 서로를 채웁니다.  ",
     "본체가 ②로 알아낸 모델별 값을 남겨 두면, 훅이 상태줄을 그릴 때 그 값을 함께 적습니다."),
], MARGIN, Inches(4.0), Inches(12.1), Inches(1.8), size=13.5, gap=11)

note_bar(s, "사용량 조회는 모델을 호출하지 않습니다. 조회를 아무리 자주 해도 토큰 요금은 붙지 않습니다. 다만 조회 1회에 claude 세션이 하나 뜨므로 간격을 너무 짧게 두지는 마십시오.",
         MARGIN, Inches(6.05), Inches(12.1), kind="ok")

# --- 0.4 준비 ---
s, y = page("0. 개요", "0.4  설치 전 준비", "확인할 것은 세 가지뿐입니다")
table(s, MARGIN, Inches(2.0), Inches(12.1),
      ["항목", "요구 사항", "확인 방법"],
      [["운영체제", "Windows 10 또는 11 (64bit)", "설정 > 시스템 > 정보"],
       ["Claude Code", "설치되어 있고 로그인되어 있을 것", "터미널에서 claude --version"],
       ["관리자 권한", "필요 없음", "UAC 창이 뜨지 않습니다"],
       ["디스크 공간", "약 32 MB", "설치 화면에 필요한 공간이 표시됩니다"],
       ["Qt · C 런타임", "따로 설치할 것 없음", "설치본에 함께 들어 있습니다"]],
      col_w=[Inches(2.3), Inches(4.6), Inches(5.2)], size=12.5)

note_bar(s, "Claude Code 가 없거나 로그인되어 있지 않으면 사용량 조회가 실패합니다. 프로그램은 뜨지만 값이 채워지지 않고 상태 표시등이 [조회 실패] 로 바뀝니다.",
         MARGIN, Inches(4.55), Inches(12.1), kind="warn")

_, tf = textbox(s, MARGIN, Inches(5.35), Inches(12.1), Inches(0.4))
para(tf, "설치되는 자리", size=15, bold=True, color=PRIMARY, space_after=8, first=True)
code_block(s, [
    "프로그램   %LOCALAPPDATA%\\Programs\\ClaudeCodeMonitor\\",
    "설정·이력  %LOCALAPPDATA%\\ClaudeCodeMonitor\\",
    r"자동 실행  HKCU\Software\Microsoft\Windows\CurrentVersion\Run",
], MARGIN, Inches(5.80), Inches(12.1), size=13)

# --- 나머지 장 ---
_here = os.path.dirname(os.path.abspath(__file__))
for _part in ("deck_part2.py", "deck_part3.py"):
    with open(os.path.join(_here, _part), encoding="utf-8") as _f:
        exec(compile(_f.read(), _part, "exec"), globals())
