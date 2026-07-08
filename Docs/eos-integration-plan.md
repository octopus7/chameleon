# EOS 적용 계획

작성 기준: 2026-07-09  
대상 프로젝트: `HideSeek/HideSeek.uproject`  
엔진 기준: Unreal Engine 5.7

## 목적

현재 `HideSeek`는 로컬 단일 플레이 테스트 중심의 위장/채색 프로토타입이다. EOS 적용의 1차 목표는 게임플레이 자체를 EOS에 의존시키는 것이 아니라, Unreal의 서버 권위 복제 모델 위에 EOS 로그인, 세션 또는 로비, 친구/초대, P2P 연결 보조를 붙여 온라인 테스트가 가능한 상태로 만드는 것이다.

EOS는 무료 라이선스로 사용할 수 있지만, Epic Developer Portal에서 제품, 배포, 클라이언트 정책, SDK 자격 증명을 설정해야 한다. 콘솔 SDK 접근은 별도 요청이 필요하다. 자세한 정책은 Epic의 [EOS 서비스 페이지](https://onlineservices.epicgames.com/), [라이선스 페이지](https://onlineservices.epicgames.com/licensing), [시작 페이지](https://onlineservices.epicgames.com/sdk)를 기준으로 확인한다.

## 현재 게임 상태

| 영역 | 현재 상태 | EOS 적용 관점 |
| --- | --- | --- |
| 프로젝트 구조 | `HideSeek` 런타임 모듈과 `ChameleonPainter` 런타임/에디터 플러그인으로 구성 | 온라인 기능은 우선 게임 모듈 또는 별도 런타임 모듈에 둘지 결정 필요 |
| 기본 맵 | `/Game/ChameleonPainterTest/Maps/L_ChameleonPainter_Test` | 멀티 PIE와 패키징 테스트의 기본 맵으로 사용 가능 |
| 기본 GameMode | `BP_ChameleonPainterGameMode`, 부모 `AChameleonPainterGameMode` | 현재 `DefaultPawnClass`만 지정된 최소 상태라 GameState/PlayerState/라운드 흐름 추가 필요 |
| GameInstance | `BP_ChameleonPainterGameInstance`, 부모 `UChameleonPainterGameInstance` | 입력 설정만 보관 중이며, EOS 로그인/세션 수명 관리 위치 후보 |
| 플레이어 캐릭터 | `AChameleonHiderCharacter` | 로컬 입력, 카메라, 페인트, 컬러피커 중심. 네트워크 권위/RPC/복제 없음 |
| 캐릭터 몸체 | `UChameleonMetaballBodyComponent` | 절차 메시, vertex color 페인트, 절차 걷기 애니메이션. 페인트 스트로크 배열은 현재 비복제 private 상태 |
| 입력 | Enhanced Input 기반 `DA_ChameleonPainterInputConfig` | 멀티 환경에서 로컬 플레이어만 mapping context 추가되도록 유지 필요 |
| UI | `WBP_ChameleonColorPicker`와 C++ fallback 위젯 | 온라인 메뉴, 로그인 상태, 호스트/참가 UI는 아직 없음 |
| 게임 루프 | 숨는 캐릭터와 채색 테스트 중심 | 헌터, 라운드 상태, 승패, 재시작이 아직 필요 |
| 온라인 설정 | `OnlineSubsystem` 주석만 있고 EOS 플러그인/설정 없음 | EOS 적용 전 로컬 복제 설계부터 필요 |

## 적용 원칙

1. EOS는 매치 발견, 로그인, 초대, P2P 연결 보조 계층으로 사용한다.
2. 실제 게임 상태는 Unreal 서버 권위 모델로 복제한다.
3. 먼저 LAN/멀티 PIE에서 게임 루프가 닫히게 만든 뒤 EOS 세션을 붙인다.
4. 페인트 상태는 정점색 전체를 매 프레임 보내지 않고, 압축 가능한 도메인 이벤트로 복제한다.
5. Developer Portal의 Client Secret, Deployment ID 등 민감하거나 환경별 값은 소스에 직접 커밋하지 않는다.

## 권장 단계

### 0. 온라인 범위 결정

결정할 것:

- 첫 온라인 형태: listen server 우선, dedicated server는 후순위.
- 방 모델: 빠른 프로토타입은 EOS Sessions, 친구 초대/대기방 UX가 중요해지면 EOS Lobbies 검토.
- 계정 요구: Epic Account 로그인 필수로 할지, Game Services 중심으로 최소화할지 결정.
- 목표 인원: 초기 2-4명 기준으로 복제량과 UI를 설계.

완료 기준:

- 온라인 테스트 목표가 `1 host + N clients`인지, dedicated server까지 포함하는지 명시된다.
- 제품/샌드박스/배포 이름 규칙과 빌드 환경 구분이 정해진다.

### 1. EOS 없이 멀티플레이 기반 먼저 만들기

추가/수정 후보:

- `AChameleonPainterGameMode`: 플레이어 입장, 역할 배정, 라운드 시작 조건.
- `AChameleonPainterGameState`: 라운드 상태, 남은 시간, 현재 phase 복제.
- `AChameleonPainterPlayerState`: 역할, 생존/발견 여부, 표시 이름, 팀 정보.
- `AChameleonPainterPlayerController`: 로컬 UI와 서버 요청 RPC의 진입점.
- 헌터 pawn 또는 hunter 모드: 최소 trace 판정으로 hider 발견 처리.

완료 기준:

- 멀티 PIE 2클라이언트에서 서버 권위로 hider/hunter 역할이 나뉜다.
- `Waiting`, `Hide`, `Seek`, `Result` 같은 라운드 상태가 모든 클라이언트에 동일하게 보인다.
- EOS 없이 `open Map?listen`, `open 127.0.0.1` 또는 PIE NetMode로 기본 흐름이 검증된다.

### 2. 채색/위장 상태 복제 설계

현재 위험:

- `UChameleonMetaballBodyComponent`의 `PaintStrokes`는 private 배열이며 복제되지 않는다.
- `ApplyPaintStrokeFromHit`가 로컬 입력에서 바로 실행되므로, 클라이언트별 색 상태가 갈라질 수 있다.
- vertex color 결과 전체를 복제하면 비용이 커진다.

권장 구조:

- 클라이언트 입력은 `ServerRequestPaintStroke` RPC로 보낸다.
- 서버가 trace 또는 입력값 검증 후 `FChameleonPaintStrokeNet` 같은 경량 stroke 데이터를 확정한다.
- 확정된 stroke는 `FFastArraySerializer` 또는 순번 기반 replicated array로 클라이언트에 전파한다.
- 클라이언트는 받은 stroke 이벤트를 로컬 절차 메시의 vertex color에 재적용한다.
- 전체 재동기화가 필요할 때를 위해 base color, stroke revision, 최근 N개 stroke snapshot을 둔다.

완료 기준:

- 한 클라이언트가 칠한 색이 다른 클라이언트에서 같은 위치와 색으로 재현된다.
- 샘플링 색, 브러시 반경/강도, 쿨다운은 서버가 최종 검증한다.
- late join 클라이언트가 현재 hider 색 상태를 따라잡는다.

### 3. EOS 플러그인과 빌드 설정 추가

수정 후보:

- `HideSeek/HideSeek.uproject`
  - UE 5.7에서 제공되는 EOS 관련 플러그인 이름을 확인 후 활성화.
  - 후보: `OnlineSubsystem`, `OnlineSubsystemEOS`, `EOSShared` 또는 UE 5.7의 Online Services 계열 EOS 플러그인.
- `HideSeek/Source/HideSeek/HideSeek.Build.cs`
  - 세션 인터페이스를 게임 모듈에서 직접 다루면 온라인 관련 모듈 의존성 추가.
- `HideSeek/Plugins/ChameleonPainter/Source/ChameleonPainter/ChameleonPainter.Build.cs`
  - 순수 게임플레이 플러그인으로 남길 수 있으면 EOS 의존성은 넣지 않는다.
- `HideSeek/Config/DefaultEngine.ini`
  - Online Subsystem/EOS 설정 섹션 추가.
  - 실제 Product ID, Sandbox ID, Deployment ID, Client ID 등은 환경별 설정으로 분리.

완료 기준:

- 프로젝트가 UE 5.7에서 컴파일된다.
- Editor 실행 시 EOS 플러그인 로딩 오류가 없다.
- 로컬 개발용 credentials가 git에 민감정보로 남지 않는다.

### 4. Developer Portal 설정

필요 작업:

- Epic Developer Portal에서 Organization/Product 생성 또는 기존 product 선택.
- Sandbox/Deployment 구성.
- Client policy 생성.
- 사용할 서비스 범위 선택: Auth, Connect, Sessions 또는 Lobbies, Friends/Overlay, P2P.
- 개발/스테이징/릴리스 환경별 값을 분리.

완료 기준:

- 개발 빌드에서 EOS SDK credentials로 초기화가 성공한다.
- 로그인 실패, 서비스 초기화 실패, 네트워크 불가 상태가 UI와 로그에 명확히 표시된다.

### 5. 온라인 진입 UI 추가

추가 후보:

- 시작 UI 또는 디버그 메뉴:
  - 로그인/로그아웃
  - Host
  - Find Sessions
  - Join
  - Leave
  - 현재 네트워크 상태 표시
- 세션 속성:
  - map name
  - max players
  - game phase
  - build id
  - public/private

완료 기준:

- 에디터/패키지 빌드에서 호스트 생성, 검색, 참가, 나가기가 가능하다.
- 세션 참가 후 기본 맵으로 이동하고 역할 배정까지 완료된다.

### 6. EOS 세션 또는 로비 연결

권장 순서:

1. UE Online Session 인터페이스로 Host/Find/Join 최소 구현.
2. listen server travel을 붙여 실제 게임 맵에 입장.
3. 세션 속성으로 빌드 버전과 게임 모드 필터링.
4. 친구 초대, 대기방 ready 상태, 팀 선택이 필요해지는 시점에 Lobby 기반으로 확장.

완료 기준:

- 서로 다른 두 PC 또는 서로 다른 계정 환경에서 세션 검색/참가가 된다.
- 참가 실패, 만원, 버전 불일치, 호스트 종료 처리가 된다.

### 7. 검증 계획

| 단계 | 테스트 |
| --- | --- |
| 로컬 복제 | PIE 2-4 clients, listen server, 역할/라운드/페인트 동기화 |
| Standalone | 한 PC에서 host/client standalone 실행 |
| LAN | 같은 네트워크의 두 PC에서 접속 |
| EOS Dev | Developer Portal dev deployment로 로그인/세션 생성/검색/참가 |
| 패키지 | Development 패키지에서 EOS 초기화와 세션 이동 확인 |
| 회귀 | 컬러피커, 좌클릭 페인트, E 샘플링, Tab/우클릭 UI가 멀티에서도 동작 |

## 우선순위 로드맵

### P0: EOS 전 선행 작업

- GameState/PlayerState/PlayerController 추가.
- hider/hunter 역할과 라운드 흐름 최소 구현.
- 페인트 stroke 서버 권위 복제.
- 멀티 PIE에서 기본 플레이 루프 검증.

### P1: EOS 최소 통합

- EOS 플러그인과 config skeleton 추가.
- GameInstance 또는 OnlineSessionSubsystem 형태의 세션 관리 코드 추가.
- 로그인, Host, Find, Join, Leave UI 추가.
- Dev Portal 개발 deployment로 연결 테스트.

### P2: 온라인 UX 확장

- 친구 초대/overlay.
- 로비 ready 상태와 역할 선택.
- host migration 또는 dedicated server 전환 검토.
- reconnect, late join, round 중 관전 처리.

### P3: 운영/릴리스 준비

- 환경별 credentials 관리.
- 빌드 버전/프로토콜 버전 불일치 차단.
- 패키징 자동화.
- 크래시/로그 수집, 네트워크 오류 사용자 메시지 정리.

## 주요 리스크

| 리스크 | 영향 | 대응 |
| --- | --- | --- |
| 페인트 상태 복제량 증가 | 대역폭과 join 동기화 비용 증가 | stroke 이벤트, revision, snapshot 전략 사용 |
| 클라이언트 권위 페인트 | 치트와 클라별 상태 불일치 | 서버 RPC 검증 후 확정 stroke만 복제 |
| EOS 설정값 커밋 | 보안/환경 오염 | placeholder config와 로컬 override 분리 |
| UE 5.7 온라인 API 차이 | 빌드 실패 또는 deprecated API 사용 | 엔진 내 플러그인/문서 기준으로 최종 API 확인 후 구현 |
| 라운드 흐름 미완성 상태에서 EOS 선적용 | 연결은 되지만 게임 테스트가 불가능 | 로컬 멀티 루프를 먼저 완성 |

## 첫 구현 단위 제안

첫 PR 또는 작업 단위는 EOS 플러그인을 바로 붙이는 대신 `ChameleonPainter` 멀티플레이 기반을 먼저 닫는 것이 좋다.

포함 범위:

- `AChameleonPainterGameState` 추가.
- `AChameleonPainterPlayerState` 추가.
- `AChameleonPainterPlayerController` 추가.
- 라운드 enum과 서버 권위 전환.
- 페인트 stroke 서버 RPC와 replicated stroke log의 최소 버전.
- 멀티 PIE 2클라이언트 검증.

이 단계가 끝나면 EOS는 세션 발견과 연결 계층으로만 붙이면 되므로, 온라인 서비스와 게임플레이 버그를 분리해서 디버깅할 수 있다.
