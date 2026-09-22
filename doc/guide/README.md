# doc/guide

사용자 가이드(PowerPoint)를 만드는 자리입니다. 완성본은 한 단계 위에 있습니다.

```
doc/
├── ClaudeCodeMonitor-사용자가이드-v1.0.pptx   완성본
└── guide/
    ├── README.md        이 문서
    ├── images/          문서에 들어가는 화면 캡처
    └── src/             문서를 만드는 스크립트
        ├── deck_lib.py      슬라이드 뼈대 (색, 글꼴, 표, 안내 상자, 그림 맞춤)
        ├── build_deck.py    표지 · 목차 · 0장
        ├── deck_part2.py    1장 설치 · 2장 기능 소개
        └── deck_part3.py    3장 제거 · 4장 CLI 지원 · 5장 기타
```

## 다시 만드는 법

`python-pptx` 와 `Pillow` 가 필요합니다.

```
pip install python-pptx Pillow
python doc/guide/src/build_deck.py
```

`build_deck.py` 가 `deck_part2.py` `deck_part3.py` 를 차례로 이어 붙여 한 벌을
만듭니다. 세 파일로 나눈 것은 한 파일이 길어져서일 뿐, 장마다 독립적이지는
않습니다. 위에서 아래로 한 번에 돕니다.

## 화면을 다시 찍어야 할 때

`images/` 의 파일 이름이 곧 스크립트가 찾는 이름입니다. 같은 이름으로 덮어쓰면
됩니다. 크기는 달라도 되며, `fit_image()` 가 비율을 지키며 자리에 맞춥니다.

| 이름 | 무엇 |
|---|---|
| `01-welcome` ~ `05-finish` | 설치 마법사 다섯 화면 |
| `10-main` | 메인 창 |
| `20-settings-alerts` `21-settings-env` | 설정 창 두 탭 |
| `22-alertlog` `23-about` | 알림 관리 창, 정보 창 |
| `31-tray-popup` `32-tray-menu` | 트레이 팝업, 트레이 우클릭 메뉴 |
| `33-closechoice` | 닫기 확인 창 |
| `34-hook-confirm` `35-hook-done` | 훅 등록 확인·완료 |
| `36-debugconsole` | 디버그 콘솔 |
| `40-cli-full` `41-cli-statusline` | Claude Code 터미널 전체와 상태줄 |
| `51-confirm` ~ `55-done` | 제거 마법사 화면들 |

## 만들 때 걸렸던 것

**한글 글꼴은 `a:ea` 까지 지정해야 합니다.** `run.font.name` 만 주면 라틴 글꼴만
바뀌고 한글은 기본 글꼴로 나옵니다. `deck_lib.py` 의 `_set_face()` 가 `a:latin`
`a:ea` `a:cs` 를 함께 적습니다.

**굵은 머리말과 설명 사이는 보통 공백으로 벌릴 수 없습니다.** 줄 끝에 오면
지워지기 때문입니다. `bullet_block()` 이 em 공백(` `)을 넣습니다.

**안내 상자는 높이를 글자 수로 가늠해 잡습니다.** 고정 높이로 두면 긴 글이
상자 밖으로 흘러 아래 요소와 겹칩니다.

**표와 안내 상자를 세로로 쌓을 때는 자리를 계산해 두어야 합니다.** python-pptx
는 자동 배치를 하지 않으므로, 줄이 늘어나면 조용히 겹칩니다. 고친 뒤에는 반드시
한 장씩 눈으로 확인하십시오. PowerPoint 로 그림을 뽑아 보는 것이 가장 빠릅니다.
