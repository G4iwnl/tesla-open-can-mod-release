---
sidebar_position: 2
---

# 설치 및 배선

이 가이드는 Tesla Model 3/Y에서 CAN 버스에 보드를 연결하는 방법을 보여줍니다. 가장 쉬운 방법은 Enhance Auto Tesla Gen 2 케이블을 사용하는 것입니다. Enhance Auto S3XY Commander에 사용되는 것과 동일한 케이블로, 단일 커넥터를 통해 CAN 버스 데이터와 12V 전원을 모두 제공합니다.

이 가이드는 보드에 무관하게 적용됩니다.

## 필요한 부품

| 부품 | 링크 |
|---|---|
| Enhance Auto Tesla Gen 2 케이블 | [enhauto.com](https://www.enhauto.com/products/tesla-gen-2-cable?variant=41214470094923) |
| 12V/24V → 5V DC/DC 컨버터 (보드에 따라 USB-C 또는 Micro-USB) | 다양한 공급업체 |

## 1단계: X179 커넥터 접근

X179 커넥터는 **보조석 발판**, 오른쪽 패널 뒤에 있습니다.

1. 오른쪽 발판 패널 트림을 제거하세요 — 도구 없이 살짝 당기면 분리됩니다.
2. 커넥터 클러스터를 찾으세요. X179 커넥터가 아래에 강조 표시된 것입니다.

![X179 커넥터 위치](/img/x179-connector.png)

커넥터에 접근하고 케이블을 연결하는 방법은 [Enhance Auto의 설치 영상](https://youtube.com/watch?v=ifwJNZgykVI)을 참고하세요.

:::note
Legacy Model 3 차량 (2020년 이전)에는 X179 커넥터가 없을 수 있습니다. 이 경우 [**X652 커넥터**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/)를 사용하세요 — 핀 1에 CAN-H, 핀 2에 CAN-L.
:::

## 2단계: 케이블 핀아웃 확인

Enhance Auto Gen 2 케이블은 한쪽 끝에 X179 포트에 연결되는 커넥터가 있고, 다른 쪽 끝에는 일반적으로 S3XY Commander에 연결되는 개별 전선이 있습니다. 개별 전선 끝의 핀아웃:

![Enhance Auto 커넥터 핀아웃](/img/pinout-labeled.png)

| 전선 | 신호 | 용도 |
|---|---|---|
| 빨간색 | 12V+ | 전원 — DC/DC 컨버터 IN+에 연결 |
| 검은색 | GND | 전원 — DC/DC 컨버터 IN-에 연결 |
| 줄무늬 검은색 | CAN-H (Body Bus) | CAN 버스 — 보드의 CAN-H에 연결 |
| 단색 검은색 | CAN-L (Body Bus) | CAN 버스 — 보드의 CAN-L에 연결 |
| 나머지 검은색 쌍 | 기타 버스 | 미사용 — 미연결 상태로 두세요 |

## 3단계: 전원 및 CAN 버스 연결

1. **CAN-H** (줄무늬 검은색)와 **CAN-L** (단색 검은색) 전선을 보드의 CAN 스크루 터미널 또는 CAN 핀에 연결하세요.
2. **12V+** (빨간색)와 **GND** (검은색) 전선을 DC/DC 컨버터의 입력에 연결하세요.
3. DC/DC 컨버터의 출력 (USB-C 또는 Micro-USB)을 보드에 연결하여 전원을 공급하세요.

![연결된 셋업](/img/x170-enhanced.png)
![연결된 셋업 예시](/img/connected-setup.png)

:::important
보드 또는 CAN 모듈에 온보드 120 Ohm 종단 저항이 있는 경우, 연결 전에 **제거하거나 끊으세요**. 차량의 CAN 버스에는 이미 자체 종단이 있으며, 두 번째 저항을 추가하면 통신 오류가 발생합니다.
:::

## 4단계: 차량에 연결

Enhance Auto 케이블을 X179 커넥터에 연결하세요. 딸깍 소리와 함께 고정됩니다 — 점퍼 와이어, 납땜, 분기 배선이 필요 없습니다.

케이블은 단일 커넥터를 통해 CAN 버스 데이터와 12V 전원을 모두 제공하므로, 차량이 켜지면 보드도 바로 작동합니다.

## 직접 배선 (Enhance Auto 케이블 없이)

직접 배선을 선호하는 경우 [**X179 커넥터**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/) 핀에 연결하세요:

| 핀 | 신호 |
|-----|--------|
| 13 | CAN-H |
| 14 | CAN-L |

**레거시 Model 3 (2020년 이전)**의 경우 [**X652 커넥터**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/)를 사용하세요:

| 핀 | 신호 |
|-----|--------|
| 1 | CAN-H |
| 2 | CAN-L |

직접 배선의 경우 전원을 별도로 공급해야 합니다.
