## 2026-07-09 00:14:47 (소요시간: 00:00:20)

- 질문: 캐릭터가 지금 그냥 스태틱 메시맞지? 메타볼 기반의?
- 답변: 현재 플레이어 캐릭터 Blueprint는 `AChameleonHiderCharacter`를 부모로 쓰고, 몸체는 `UStaticMeshComponent`가 아니라 `UChameleonMetaballBodyComponent`입니다. 이 컴포넌트는 `UProceduralMeshComponent` 기반이며, 코드에서 메타볼/implicit field 값을 샘플링해 절차 메시를 생성합니다. 다만 `SourceAssets/Characters/Hider/SM_HiderMetaball_Body.obj`라는 스태틱 메시용 프로토타입 소스 에셋도 존재합니다.

## 2026-07-09 00:13:55 (소요시간: 00:03:00)

- 질문: 페인팅 기능은 어떻게 진입하며, 우클릭이 인접 환경에서 단색을 뽑는 것처럼 보이는데 수동 브러시 색 선택 후 칠하는 방식이 맞지 않는가?
- 답변: 기존 매핑은 우클릭도 환경 샘플링에 연결되어 있어 의도와 다르게 보였다. 수정 후 기본 조작은 좌클릭 페인트, 우클릭/Tab 수동 컬러피커, E 선택적 환경 샘플링이다. 컬러피커는 브러시 색만 바꾸고 몸 전체 기본색은 바꾸지 않는다.

## 2026-07-09 00:37:09 (소요시간: 00:14:39)

- 질문: Tab을 누르면 커서만 보이고 위젯은 전혀 보이지 않는데, 채색 UI를 WBP로 만들어서 구성한 것이 맞나?
- 답변: 이전 구현은 실제 WBP 애셋이 아니라 C++ UMG 위젯 fallback 중심이었다. 수정 후 `/Game/ChameleonPainterTest/UI/WBP_ChameleonColorPicker` WBP가 생성되고 `BP_ChameleonHiderCharacter.ColorPickerWidgetClass`에 연결된다. 이제 Tab 또는 우클릭으로 컬러피커 WBP를 열고, 좌클릭으로 현재 브러시 색을 칠하며, E 키로만 주변 환경 색을 샘플링한다.
## 2026-07-09 00:41:07 (소요시간: 00:00:15)

- 질문: 에픽에서 제공해주는 네트웍 서비스 뭐였지?
- 답변: Epic Online Services(EOS)입니다. Unreal에서는 Online Subsystem EOS나 EOS SDK를 통해 로그인, 친구, 로비, 매치메이킹, P2P 네트워킹, 세션, 업적 같은 온라인 기능을 붙일 수 있습니다.

## 2026-07-09 00:43:07 (소요시간: 00:00:40)

- 질문: EOS 무료였나? 사용 자격 따로 신청해야하는지도 알고싶어
- 답변: Epic Online Services(EOS)는 게임 및 게임 관련 애플리케이션 용도라면 상업/비상업/교육/개인 사용 모두 무료 라이선스로 사용할 수 있다. 일반적인 PC/모바일/Unreal 연동은 별도 심사 신청보다 Epic Games 계정으로 Developer Portal에 로그인해 제품을 만들고 SDK 자격 증명을 발급받는 절차가 필요하다. 단, 콘솔 SDK는 Developer Portal에서 접근 요청이 필요하고, 추가 Anti-Cheat 엔터프라이즈 지원/기능이나 Kids Web Services 같은 일부 항목은 별도 조건 또는 라이선스가 붙을 수 있다.

## 2026-07-09 00:44:51 (소요시간: 00:03:02)

- 질문: 웨이트를 어떤식으로 먹인건진 모르겠는데 여전히 양 다리가 간섭 일어난다. 버텍스 만들때부터 본 지정하고 접합부를 부드럽게 처리하는건 어때?
- 답변: 그 방식이 맞다. 기존 위치 기반 후처리는 골반/접합부에서 양쪽 다리 가중치가 섞일 수 있으므로, 메타볼 blob마다 담당 본을 지정하고 정점 생성 시점의 blob field contribution으로 skin weight를 산출하도록 바꿨다. 중심선 근처에서는 반대쪽 사지 가중치를 차단하고, 골반/어깨 접합부만 몸통 또는 루트 본과 부드럽게 블렌딩한다.
