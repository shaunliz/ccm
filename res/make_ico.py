# Copyright (C) 2026 Innogrid Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-only

"""PNG 여러 장을 .ico 한 개로 묶는다.

빌드에 들어가지 않는다. 아이콘 그림을 고친 뒤 한 번씩 손으로 돌린다.
쓰는 방법은 res/README.md 참조.

왜 직접 쓰는가
  PIL 같은 외부 묶음을 끌어오지 않기 위해서다. ICO 는 머리 몇 바이트에 PNG 를
  그대로 담을 수 있는 단순한 형식이라 표준 라이브러리로 충분하다.

  16~48px 을 BMP 로 담는 옛 방식도 있지만, Windows Vista 이후는 PNG 를 모든
  크기에서 읽는다. 이 프로그램은 Windows 10 이상을 본다.
"""
import pathlib
import struct
import sys

# 담을 크기. 작업 표시줄, 탐색기, 설치 관리자가 각자 골라 쓴다.
SIZES = [16, 20, 24, 32, 48, 64, 128, 256]


def build(png_dir: pathlib.Path, out_path: pathlib.Path) -> None:
    images = []
    for size in SIZES:
        path = png_dir / f"icon_{size}.png"
        if not path.is_file():
            raise SystemExit(f"없는 파일: {path}")
        images.append((size, path.read_bytes()))

    # ICONDIR: 예약(2) + 형식(2, 1=아이콘) + 개수(2)
    header = struct.pack("<HHH", 0, 1, len(images))

    # 데이터는 모든 ICONDIRENTRY 뒤에 이어 붙는다.
    offset = len(header) + 16 * len(images)

    entries = bytearray()
    payload = bytearray()
    for size, data in images:
        # 폭과 높이는 1바이트다. 256 은 0 으로 적는 것이 규칙이다.
        dimension = 0 if size >= 256 else size
        entries += struct.pack(
            "<BBBBHHII",
            dimension,      # 폭
            dimension,      # 높이
            0,              # 색 수. 32비트이므로 0
            0,              # 예약
            1,              # 색 평면
            32,             # 픽셀당 비트
            len(data),      # 이 그림의 바이트 수
            offset,         # 파일 안에서의 자리
        )
        payload += data
        offset += len(data)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(header + bytes(entries) + bytes(payload))
    print(f"{out_path} ({out_path.stat().st_size:,} 바이트, {len(images)}개 크기)")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit("사용법: make_ico.py <PNG 디렉터리> <출력 .ico>")
    build(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]))
