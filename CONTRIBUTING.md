# 기여 가이드

Tesla Open CAN Mod에 기여해 주셔서 감사합니다!

## 빠른 시작

```bash
# 클론
git clone https://github.com/ylovex75/tesla-open-can-mod-release.git
cd tesla-open-can-mod-release

# 두 빌드 타겟 모두 빌드
pio run -e waveshare_esp32s3_rs485_can -e esp32_twai

# 테스트 실행 (네이티브 테스트 114개)
pio test -e native
```

## 개발 환경 설정

- **IDE:** VS Code + PlatformIO 확장 (권장)
- **툴체인:** PlatformIO CLI (`pio`)
- **빌드 타겟:** `waveshare_esp32s3_rs485_can` (ESP32-S3) 및 `esp32_twai` (ESP32)
- **테스트 환경:** `native` (호스트에서 실행, 하드웨어 불필요)

## 코드 스타일

- C++17 (`-std=gnu++17`)
- 들여쓰기: 스페이스 4칸, 탭 금지
- 중괄호: 함수 및 제어문과 같은 줄에
- 헤더 전용 아키텍처: 모든 로직은 `include/` 헤더에 위치
- 커밋 전 `clang-format --style=file`로 포맷 적용

## 풀 리퀘스트 절차

1. 저장소를 **Fork**하고 `main`에서 브랜치를 생성합니다
2. 두 타겟을 **빌드**하여 오류가 없는지 확인합니다: `pio run -e waveshare_esp32s3_rs485_can -e esp32_twai`
3. `pio test -e native`로 **테스트**합니다 — 114개 이상의 테스트가 모두 통과해야 합니다
4. 사용자 관련 변경 사항이 있으면 `CHANGELOG.md`의 `[Unreleased]` 항목을 **업데이트**합니다
5. 기능이나 동작에 변화가 있으면 `docs-site/docs/`의 문서를 **업데이트**합니다
6. 변경 내용과 이유를 명확히 설명하여 PR을 **제출**합니다

### PR 체크리스트

- [ ] ESP32 및 ESP32-S3 두 타겟 모두 컴파일 통과
- [ ] `pio test -e native` 통과
- [ ] `CHANGELOG.md` 업데이트 (사용자 관련 변경 시)
- [ ] 문서 업데이트 (기능/동작 변경 시)
- [ ] 새로운 컴파일러 경고 없음

## 기여할 수 있는 항목

### 초보자 이슈

- i18n 번역 개선 (새 언어 추가 또는 기존 번역 수정)
- CAN 신호 디코딩 단위 테스트 추가
- 문서 개선

### 기능 기여

- 새로운 CAN 신호 디코더 (읽기 전용 모니터링)
- Web UI 개선
- 새로운 보드 지원 (TWAI 탑재 ESP32 기반)

### 안전 관련 기여

CAN TX 동작을 수정하는 변경(프레임 주입 또는 수정)은 추가 검토가 필요합니다:

- 영향을 받는 CAN ID 및 비트를 명확하게 문서화
- `test/`에 해당 단위 테스트 추가
- `SECURITY.md`의 TX 표면 테이블 업데이트
- 더 긴 리뷰 주기 예상

## 프로젝트 구조

```
include/
  app.h              — 메인 루프, 핸들러 디스패치, OTA 보호
  handlers.h         — Legacy/HW3/HW4/Nag CAN 프레임 핸들러
  can_live.h         — 실시간 신호 디코딩 & O(1) 조회
  can_helpers.h      — Tesla 체크섬, 비트 조작
  can_monitor.h      — PSRAM 기반 CAN 프레임 로거
  log_buffer.h       — 시스템 로그 메시지용 링 버퍼
  shared_types.h     — 공유 열거형 및 구조체
  drivers/
    twai_driver.h    — ESP32 TWAI CAN 드라이버
  web/
    web_server.h     — HTTP 서버, NVS, WiFi, API 엔드포인트
    web_ui.h         — 단일 페이지 웹 대시보드 (HTML/CSS/JS)
src/
  main.cpp           — 진입점 (앱 루프 호출)
test/
  test_native_*/     — 네이티브 단위 테스트 (하드웨어 불필요)
```

## 버전 관리

- 버전은 `VERSION` (시맨틱 버전) 및 `include/version.h`에서 관리됩니다
- 버전 업 시 두 파일을 함께 업데이트해야 합니다
- `CHANGELOG.md`는 [Keep a Changelog](https://keepachangelog.com/) 형식을 따릅니다

## 버그 신고

[이슈 트래커](https://github.com/ylovex75/tesla-open-can-mod-release/issues)를 이용해 주세요. 보안 관련 이슈는 [SECURITY.md](SECURITY.md)를 참고하세요.

## 라이선스

기여함으로써, 귀하의 기여물이 GPL-3.0 라이선스 하에 배포됨에 동의하는 것으로 간주됩니다.
