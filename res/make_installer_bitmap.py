# Copyright (C) 2026 Innogrid Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-only
"""설치 마법사 시작/완료 화면의 옆 그림을 만든다.

왜 만드는가
  MUI 가 기본으로 쓰는 ${NSISDIR}\\Contrib\\Graphics\\Wizard\\win.bmp 는
  4비트(16색) 이미지다. 요즘 화면에서는 디더링 무늬가 그대로 드러나 파란
  잡티처럼 보인다. NSIS 가 함께 주는 nsis3-metro.bmp 는 24비트라 깔끔하지만
  우리 것이 아니다. 그래서 프로그램 아이콘과 같은 색으로 한 장 만든다.

크기
  164 x 314 는 MUI 가 정한 자리다. 다른 크기를 주면 늘어나거나 잘린다.

  python res/make_installer_bitmap.py
  -> res/installer-welcome.bmp (24비트)
"""
import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ICON = os.path.join(HERE, "ClaudeCodeMonitor.ico")
OUT = os.path.join(HERE, "installer-welcome.bmp")

W, H = 164, 314

# 프로그램 아이콘과 같은 계열의 파랑. 위가 밝고 아래로 내려갈수록 어둡다.
TOP = (37, 99, 165)
BOTTOM = (16, 45, 82)
ACCENT = (94, 165, 235)


def vertical_gradient(size, top, bottom):
    w, h = size
    img = Image.new("RGB", (1, h))
    px = img.load()
    for y in range(h):
        t = y / (h - 1)
        px[0, y] = tuple(round(a + (b - a) * t) for a, b in zip(top, bottom))
    return img.resize((w, h), Image.BILINEAR)


def pick_font(size, bold=False):
    """맑은 고딕을 쓰되, 없으면 기본 글꼴로 물러난다.

    MUI 는 이 그림을 제 자리(109u x 193u)에 맞춰 조금 눌러 그린다. 얇고 작은
    글자는 그 과정에서 뭉개지므로 굵게, 크게 잡는다.
    """
    names = ("malgunbd.ttf", "segoeuib.ttf") if bold else ("malgun.ttf", "segoeui.ttf")
    for name in names + ("arial.ttf",):
        path = os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts", name)
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, size)
            except OSError:
                pass
    return ImageFont.load_default()


def centered(draw, y, text, font, fill):
    left, top, right, bottom = draw.textbbox((0, 0), text, font=font)
    draw.text(((W - (right - left)) / 2 - left, y), text, font=font, fill=fill)
    return bottom - top


img = vertical_gradient((W, H), TOP, BOTTOM)
draw = ImageDraw.Draw(img)

# 아래쪽을 한 단 더 어둡게 깔아 글자가 뜨게 한다.
for y in range(H - 96, H):
    t = (y - (H - 96)) / 95
    base = img.getpixel((0, y))
    draw.line([(0, y), (W, y)],
              fill=tuple(round(c * (1 - 0.28 * t)) for c in base))

# 프로그램 아이콘. ico 는 여러 장이 들어 있으므로 가장 큰 것을 골라 줄인다.
# 작은 것을 키우면 뭉개진다.
icon = Image.open(ICON)
largest = max(icon.info.get("sizes", {(128, 128)}))
icon.size = largest
icon.load()
icon = icon.convert("RGBA").resize((76, 76), Image.LANCZOS)
img.paste(icon, ((W - 76) // 2, 62), icon)

# 이름. 줄을 적게 두는 편이 눌려 그려질 때 덜 상한다.
f_name = pick_font(19, bold=True)
f_small = pick_font(12, bold=True)

centered(draw, 156, "Claude Code", f_name, (255, 255, 255))
centered(draw, 180, "사용량 모니터", f_name, (255, 255, 255))

draw.line([(W // 2 - 26, 214), (W // 2 + 26, 214)], fill=ACCENT, width=2)

# 버전은 적지 않는다. 그림에 구워 두면 판을 올릴 때마다 어긋난다.
# 마침 화면의 글에 이미 나온다.
centered(draw, H - 40, "Innogrid Co., Ltd.", f_small, (138, 175, 215))

img.convert("RGB").save(OUT, "BMP")
print("만들었습니다:", OUT, img.size)
