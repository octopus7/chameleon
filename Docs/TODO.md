# TODO

Meccha Chameleon 스타일 위장 게임 프로토타입의 다음 작업 후보.

## 완료된 기준 작업

| 완료 | 작업 | 구현 위치 |
| --- | --- | --- |
| Done | 테스트 레벨과 페인트 머티리얼 생성 | `/Game/ChameleonPainterTest/Maps/L_ChameleonPainter_Test`, `/Game/ChameleonPainterTest/Materials` |
| Done | 플레이어 캐릭터에 hider body와 color picker 연결 | `AChameleonHiderCharacter`, `BP_ChameleonHiderCharacter` |
| Done | 입력을 캐릭터 BP에 직접 연결하지 않고 DA/GameInstance 경유 | `DA_ChameleonPainterInputConfig`, `BP_ChameleonPainterGameInstance` |
| Done | WASD 이동, Space 점프, Mouse 시점 3인칭 조작 | `IA_Move`, `IA_Look`, `IA_Jump`, `IMC_ChameleonPlayer` |
| Done | 브러시 입력, 수동 색 선택, 배경 색 샘플링 | 좌클릭 페인트, 우클릭/Tab 수동 컬러피커, E 샘플링 |

## 우선순위 기준

- `P0`: 다음 플레이 테스트를 위해 바로 처리해야 하는 항목.
- `P1`: 핵심 게임성 확장에 필요하지만 P0 검증 뒤에 붙이는 항목.
- `P2`: 품질, 최적화, 장기 운영 관점의 항목.

## 작업 후보

| 우선순위 | 작업 | 이유 | 완료 기준 |
| --- | --- | --- | --- |
| P0 | PIE 조작 검증과 입력 모드 조정 | 캐릭터, DA, GameInstance, color picker가 실제 PIE에서 함께 움직이는지 확인해야 한다. | PIE에서 WASD/Space/Mouse, 좌클릭 페인트, 우클릭/Tab 수동 컬러피커, E 샘플링이 충돌 없이 동작한다. |
| P0 | 브러시 UX 튜닝 | 현재 브러시 반경/강도는 C++ 기본값이다. 플레이 감각에 맞는 조절 UI나 기본값이 필요하다. | 반경/강도 값을 조정할 수 있고, 페인트 흔적이 의도한 크기와 속도로 누적된다. |
| P0 | 헌터 판정 최소 프로토타입 | 숨는 쪽만 있으면 게임 루프가 닫히지 않는다. | 헌터 입력 또는 테스트 액터 trace가 hider 적중을 판정하고 로그/상태로 확인된다. |
| P0 | 라운드 흐름 최소 구현 | 숨기 시간, 찾기 시간, 승패가 있어야 테스트가 반복 가능하다. | 준비, 숨기, 탐색, 결과 상태가 GameMode/GameState에서 전환된다. |
| P1 | render target 기반 페인팅 검토 | 현재 stroke는 vertex color 기반이라 세밀한 지속 페인트에는 한계가 있다. | UV/RenderTarget 기반 백엔드 설계와 최소 proof of concept를 추가한다. |
| P1 | 네트워크 복제 설계 | 멀티플레이 숨바꼭질이면 색상과 stroke 상태가 다른 클라이언트에 보여야 한다. | base color와 stroke 또는 paint texture 상태가 서버 권위로 복제된다. |
| P1 | 색상 샘플링 규칙 제한 | 자동 배경 매칭이 너무 강하면 난이도가 무너질 수 있다. | 샘플링 쿨다운, 색상 양자화, 샘플 가능 표면 제한 중 하나 이상을 적용한다. |
| P1 | 헌터 이동/카메라 프리셋 | hider와 hunter 조작감이 같을 필요는 없다. | hunter pawn/controller 입력 프리셋과 카메라 값을 분리한다. |
| P2 | 몸체 메시 최적화/LOD | 현재 절차 메시 해상도는 프로토타입용이다. | 해상도 preset, 캐싱, LOD 또는 static mesh bake 전략을 정한다. |
| P2 | 캐릭터 애니메이션/리깅 방향 결정 | 절차 메시만으로는 걷기/점프 가독성이 부족할 수 있다. | skeletal mesh, 단순 squash/stretch, procedural animation 중 하나를 선택한다. |
| P2 | 색상 UI UX 정리 | 현재 위젯은 기능 중심이다. | 최근 색, 환경 팔레트, 미리보기, 취소/확정 흐름을 정리한다. |
| P2 | 플러그인 모듈 분리 검토 | UMG 의존을 더 낮추려면 Runtime/UMG 분리가 필요할 수 있다. | 실제 재사용 요구가 생기면 Runtime, UMG, Editor 모듈 분리안을 확정한다. |

## 추천 진행 순서

1. PIE에서 현재 테스트 맵을 플레이하고 입력/위젯/페인트 루프를 검증한다.
2. 브러시 반경과 색 샘플링 감도를 조정한다.
3. 헌터 판정과 라운드 흐름을 최소 구현한다.
4. 멀티플레이 복제와 render target 페인팅 필요성을 결정한다.
5. 메시 최적화와 애니메이션 방향을 검토한다.
