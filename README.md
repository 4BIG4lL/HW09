# NumBall
UnrealEngine 5.5.4 + C++를 활용한 데디케이티드 서버 기반 멀티플레이 게임 만들기 실습 과제

채팅으로 간단하게 진행하는 **숫자 야구 게임**을 구현

## 게임 설명

서버가 1~9 사이에서 중복 없는 3자리 정답을 생성.

각 플레이어는 채팅에 정확히 3자리 숫자를 입력해서 추리.

```123456```처럼 3자리 초과 연속 숫자가 있으면 제출로 취급하지 않음.

```12a3``` 같은 건 무시.


### 판정
```
- 자리&값 같음 → S(Strike)

- 자리 다르지만 값 존재 → B(Ball)

- 둘 다 아님 → OUT
```

### 턴/타이머
```
- 한 번에 한 명만 턴. 내 턴에서만 타이머가 흐름.

- 턴 타임아웃 시, 그 플레이어는 기회 1회 소진 후 턴이 넘어감.
```

### 승패/리셋
```
- 먼저 3S면 승리.

- 두 사람 모두 3회 안에 못 맞추면 무승부.

- 결과 메시지를 3초 보여준 뒤, 새 게임 자동 시작.
```
### UI
```
- 채팅창(스크롤) + 입력박스, 남은 기회 표시, 턴 타이머/상태 표시.

- “새 게임 시작”, “승리/무승부” 공지 표시(자동 사라짐).
```
## 프로젝트 구조
```
NumBall/
├─ Source/NumBall/
│  ├─ Game/
│  │  ├─ NumBallGameMode.h / .cpp        # 전체 경기 로직(정답생성/판정/승패/턴전개)
│  │  └─ NumBallGameState.h / .cpp       # 복제 상태(턴/타이머/매치상태), 멀티캐스트 브로드캐스트
│  ├─ Player/
│  │  ├─ NumBallPlayerController.h / .cpp # 채팅 RPC, UI 스폰/갱신, 남은기회 즉시반영
│  │  └─ NumBallPlayerState.h / .cpp      # 시도 횟수/턴 행동 플래그 복제
│  └─ UI/
│     ├─ ChatWidget.h / .cpp              # 채팅 목록/입력, 남은 기회 표시
│     ├─ NoticeWidget.h / .cpp            # 상단 공지
│     └─ TurnTimerWidget.h / .cpp         # 내 턴/상대 턴 + 타임바
└─ Content/NumBall/UMG/
   ├─ WBP_Chat.uasset          # ChatWidget용 UMG
   ├─ WBP_Notice.uasset        # NoticeWidget용 UMG
   └─ WBP_TurnTimer.uasset     # TurnTimerWidget용 UMG
```


## 실행 설정
```
Number of Players = 2

Net Mode = Play As Client

Advanced → Launch Separate Server = ON

Run Under One Process = OFF
```

# 구현 설명
## 필수 기능
```
- GameModeBase 판정 로직 구현

  - 정답 생성: ANumBallGameMode::GenerateAnswer3Digits()

  - 유효성 검사/패턴 추출: ExtractValidGuess(const FString&, FString&)

    - 정확히 3자리만 제출 인정(0 금지, 중복 금지)

    - 길이 4↑ 연속 숫자 발견 시 제출 무효 (순수 채팅)

  - 판정: Judge(const FString& Answer, const FString& Guess)

    - 3글자 자리 비교로 S/B/Out 계산

  - 서버 처리: HandleChatFromPlayer(...)

    - 서버 권위에서만 채팅 → 판정 → 브로드캐스트

    - 상대 턴 입력은 제출로 취급 X(턴 유지)

  - 승/무/리셋: EvaluateWinConditions(...) + ResetMatch()

    - 3S → 승리, 전원 3회 소진 → 무승부

    - 결과 공지 3초 후 ResetMatch()로 새 게임
```
```
- 기본 멀티플레이 채팅

  - 브로드캐스트: ANumBallGameState::MulticastChat(...)

  - 클라 수신/UI 갱신: ANumBallPlayerController::ClientAppendChat(...)

    - 중복 라인 디더프(직전 라인과 동일 시 무시) 적용

  - 정답 숫자 3자리 생성 함수 GenerateAnswer3Digits()

    - 1~9 풀에서 중복 제거하며 3자리 생성

  - 숫자야구 입력 유효성 판단 함수 ExtractValidGuess(...)

    - 메시지 내 숫자 연속 구간을 추출

    - 0 포함/중복/3자리가 아님 → 무효

    - 3자리 초과 구간 존재 시 → 이번 메시지 전체를 제출로 취급하지 않음

  - S/B/OUT 계산 함수 Judge(...)

    - 자리 동일 → Strikes++

    - 값만 존재 → Balls++

    - 둘 다 0 → Out=true

  - 문자열 변환: FJudgeResult::ToResultString() → "2S0B", "OUT"

    - PlayerState로 시도 횟수 관리

    - ANumBallPlayerState

    - MaxAttempts(기본 3), CurrentAttempts, bActedThisTurn 전부 Replicate

    - IncreaseAttempt(), ResetAttempts(), CanAttempt()
```
```
- 사용 흐름

  - 유효 제출 시에만 IncreaseAttempt() (상대 턴/무효 입력은 X)

  - 타임아웃 시, 해당 턴에서 행동 안 했으면 IncreaseAttempt()

  - 시도 횟수 UI (남은 기회)

    - ANumBallPlayerController::UpdateAttemptsText()

    - "남은 기회: (Max - Current)" 표기

    - 리셋 시 강제 갱신: ClientRefreshAttempts() 멀티캐스트로 호출

  - 승리/무승부/게임 리셋 & 공지 위젯

    - 공지 브로드캐스트:

    - 즉시: MulticastAnnounce(...)

  - 지속시간 지정: MulticastAnnounceTimed(Message, Seconds)

    - 승/무 결과 3초 노출 후 자동 리셋 → ResetMatch() → 인원 충분 시 즉시 다음 라운드 시작
```

##도전 기능
```
- 턴 제어 + 남은 시간 UI

  - 서버 타이머: ANumBallGameState::TickTurnTimer()

    - 내 턴에서만 TurnTimeRemaining 감소

    - 0초 도달 → ANumBallGameMode::AdvanceTurn(true)

  - 클라 UI: UTurnTimerWidget::NativeTick()

    - 내 턴: 게이지/“남은 시간 Ns”

    - 상대 턴: “상대 턴”
```
 턴 내 미행동 소진(도전): 행동 플래그 기반 기회 차감
