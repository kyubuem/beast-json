# Beast JSON — 멀티 아키텍처 지원 전략

> **최종 업데이트**: 2026-03-03
> **현황**: 보유 장비 3종으로 성능 튜닝 집중, 미보유 아키텍처는 GitHub Actions CI로 정확성(correctness) 커버

---

## 1. 설계 철학 — "분리된 두 목표"

Beast JSON의 멀티 아키텍처 전략은 목표를 명확히 분리한다.

| 목표 | 커버 수단 | 설명 |
|:---|:---|:---|
| **성능 튜닝** | 보유 장비 3종 | 실제 하드웨어에서만 유의미한 측정 가능 |
| **정확성 검증** | GitHub Actions CI | 모든 아키텍처, 모든 플랫폼에서 `ctest` 통과 보장 |

> **성능 벤치마크는 CI에서 측정하지 않는다.** QEMU 에뮬레이션 및 공유 클라우드 러너는 타이밍 노이즈가 수십 % 이상이므로, 수치 비교에 의미가 없다.

---

## 2. 보유 장비 현황 및 커버 범위

```
장비                            아키텍처 분기              커버 범위
─────────────────────────────────────────────────────────────────────
Mac (Apple M1 Pro)             BEAST_ARCH_APPLE_SILICON   Apple M1 ~ M4 (동일 ISA)
                               128B 캐시 라인             Apple Silicon 전 라인
                               512B prefetch 거리

Snapdragon 8 Gen 2 (Termux)   BEAST_ARCH_GENERIC_AARCH64  표준 Cortex-X3 계보
                               192B prefetch 거리         ≈ AWS Graviton 2 (Neoverse N1) 동급
                               NO SVE (Android 커널 비활)  표준 AArch64 실 성능 전담

Claude Code 인스턴스 (x86_64) x86_64 경로               Intel/AMD AVX-512/AVX2/SSE2
                               (AWS x86 계열)             Linux x86 전체 전담
─────────────────────────────────────────────────────────────────────
```

### Snapdragon Gen 2 ≈ Graviton 근거

- Cortex-X3는 ARM 표준 Cortex 계보 (NEON + ARMv8.4-a + DotProd + I8MM)
- AWS Graviton 2(Neoverse N1), Graviton 3(Neoverse V1)도 동일 ISA 계보
- simdjson이 전 세계 Graviton 최적화를 단일 `arm64` 구현으로 커버하는 것이 근거
- **SVE 여부만 다름**: Graviton 3은 SVE 지원, Snapdragon은 Android 커널 비활성화
  → SVE 경로는 별도 조건부 컴파일(`BEAST_HAS_SVE`)로 분리, CI에서 검증

---

## 3. Beast JSON의 AArch64 전략 — 경쟁 우위

### 타 라이브러리와의 비교

| | simdjson | yyjson | **Beast JSON** |
|:---|:---:|:---:|:---:|
| Apple Silicon 별도 감지 | ❌ | ❌ | ✅ `BEAST_ARCH_APPLE_SILICON` |
| 캐시 라인 아키텍처별 분리 | ❌ | ❌ | ✅ 128B(M1) / 64B(ARM) / 64B(x86) |
| prefetch 거리 아키텍처별 | ❌ | ❌ | ✅ 512B / 256B / 192B |
| SVE 유무 감지 | ❌ | ❌ | ✅ `BEAST_HAS_SVE` |
| DOTPROD 별도 감지 | ❌ | ❌ | ✅ `BEAST_HAS_DOTPROD` |
| AArch64 구현 수 | 1 (단일) | 1 (단일) | 3 (Apple / Generic / Fallback) |

**핵심 insight**: simdjson은 `bitmask.h` 주석에서 Apple M1/N1 차이를 무시하고 단일 `arm64`로 처리하면서, PMULL이 "apparently slow"라고 주석 달고 소프트웨어 XOR 체인으로 대체함. M1과 Graviton을 구별하지 않았기 때문. Beast JSON은 이 구분을 명시적으로 도입한 최초 구현.

### 3-레벨 AArch64 분기 구조

```
AArch64 진입
    │
    ├── BEAST_ARCH_APPLE_SILICON  (__APPLE__ && __aarch64__)
    │       캐시 라인: 128B
    │       prefetch 거리: 512B (M1 L2 스트라이드 최적화)
    │       금지: PMULL (GPR-SIMD 전송 레이턴시), SVE
    │       특화: 576-entry OoO 버퍼 친화형 Global NEON Pipeline
    │
    ├── BEAST_ARCH_GENERIC_AARCH64  (__aarch64__ && !__APPLE__)
    │       캐시 라인: 64B
    │       prefetch 거리: 192B (Cortex-X3 / Neoverse N1 최적)
    │       SVE: BEAST_HAS_SVE 있을 때만 조건부 활성화
    │       DotProd: BEAST_HAS_DOTPROD 있을 때만 활성화
    │
    └── Fallback (기타 AArch64 — Raspberry Pi 등)
            캐시 라인: 64B
            prefetch: 비활성화
            순수 NEON 기본 구현
```

---

## 4. GitHub Actions CI 전략 — 미보유 아키텍처 정확성 커버

### 목표: 정확성(ctest 통과) 보장, 성능 측정 없음

#### 4-1. 즉시 활용 가능한 무료 러너 (공개 저장소 기준)

| 러너 | OS | 아키텍처 | 실제 하드웨어 | 비고 |
|:---|:---|:---|:---|:---|
| `ubuntu-24.04` | Linux | x86_64 | ✅ 네이티브 | 현재 주력 환경과 동일 |
| `ubuntu-24.04-arm` | Linux | AArch64 | ✅ Graviton2 네이티브 | Snapdragon과 ISA 동일 |
| `macos-15` | macOS | Apple Silicon | ✅ M1 네이티브 | 보유 Mac과 동일 |
| `windows-2025-arm` | Windows | ARM64 | ✅ 네이티브 | MSVC AArch64 정확성 |

#### 4-2. QEMU 에뮬레이션 (x86 호스트에서 추가 아키텍처)

| 타겟 아키텍처 | QEMU 타겟 | 용도 |
|:---|:---|:---|
| RISC-V 64 | `qemu-riscv64` | 엔디안/정렬 이슈, fallback 경로 |
| PPC64LE | `qemu-ppc64le` | Big/Little endian 혼재 검증 |
| s390x | `qemu-s390x` | Big endian 극단 케이스 |
| ARM 32-bit | `qemu-arm` | 구형 ARM 정렬 동작 |

> ⚠️ **QEMU 에뮬레이션은 성능 측정에 절대 사용하지 않는다.** 오직 `ctest` 통과 여부만 확인.

#### 4-3. 권장 workflow 구성 (초안)

```yaml
# .github/workflows/ci.yml (미구현 — TODO)
name: CI

on: [push, pull_request]

jobs:
  # ── 1. 네이티브 성능 머신 (정확성 + 기능 테스트) ──────────────────
  native-linux-x86:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)
      - run: ctest --test-dir build --output-on-failure

  native-linux-arm64:          # Graviton2 (= Snapdragon ISA 동급)
    runs-on: ubuntu-24.04-arm
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)
      - run: ctest --test-dir build --output-on-failure

  native-macos-apple-silicon:  # M1 (보유 Mac과 동일)
    runs-on: macos-15
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)
      - run: ctest --test-dir build --output-on-failure

  native-windows-arm64:        # Windows ARM64 정확성
    runs-on: windows-2025-arm
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
      - run: ctest --test-dir build --output-on-failure

  # ── 2. QEMU 에뮬레이션 (정확성만, 느린 아키텍처) ─────────────────
  qemu-riscv64:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get install -y qemu-user-static gcc-riscv64-linux-gnu g++-riscv64-linux-gnu
      - run: |
          cmake -B build \
            -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-linux-gnu.cmake \
            -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j$(nproc)
          cd build && ctest --output-on-failure

  qemu-ppc64le:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get install -y qemu-user-static gcc-powerpc64le-linux-gnu g++-powerpc64le-linux-gnu
      - run: |
          cmake -B build \
            -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ppc64le-linux-gnu.cmake \
            -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j$(nproc)
          cd build && ctest --output-on-failure
```

---

## 5. 미보유 아키텍처 대응 방향

### 현재 계획 (Phase 76 이후 여유 있을 때)

| 아키텍처 | 조달 방법 | 우선순위 | 메모 |
|:---|:---|:---:|:---|
| AWS Graviton 3 (SVE) | Oracle Cloud Always Free (Ampere A1, 4코어 영구 무료) | 🟠 중기 | SVE 코드패스 실 성능 측정 필요 시 |
| Windows ARM64 | CI only (정확성) → 실 측정 필요 시 별도 조달 | 🟡 낮음 | MSVC 컴파일 오류 조기 발견이 주목적 |
| RISC-V | CI only (QEMU, 정확성) | 🟡 낮음 | 엔디안/정렬 버그 조기 발견 |
| PPC64LE | CI only (QEMU, 정확성) | 🟡 낮음 | Big endian 코너케이스 |

### 현재 하지 않는 것 (의도적 결정)

- **CI에서 성능 수치 측정**: 공유 러너 타이밍 노이즈 → 의미 없음
- **Graviton 특화 최적화**: Snapdragon(Cortex-X3)로 충분히 커버됨
- **Windows ARM64 성능 튜닝**: 사용자 기반 우선순위 낮음
- **크로스 컴파일 페르포먼스 벡터 분석**: 에뮬레이션 결과는 신뢰 불가

---

## 6. 현재 아키텍처 감지 매크로 정의 위치

```
include/beast_json/arch.h  (또는 config.h)
    BEAST_ARCH_APPLE_SILICON   → __APPLE__ && __aarch64__
    BEAST_ARCH_GENERIC_AARCH64 → __aarch64__ && !__APPLE__
    BEAST_HAS_SVE              → __ARM_FEATURE_SVE (+ Android 커널 검사)
    BEAST_HAS_DOTPROD          → __ARM_FEATURE_DOTPROD
    BEAST_CACHE_LINE_SIZE      → 128 (Apple) / 64 (기타)
```

---

## 7. 참고 — 타 라이브러리의 접근법 요약

### simdjson

- **런타임 dispatch**: CPU 기능을 런타임에 감지 → 가장 좋은 구현 선택
- **구현 분기**: icelake → haswell → westmere → arm64 → fallback (x86), ppc64, lsx/lasx (LoongArch)
- **AArch64**: 단일 `arm64` 구현. Apple Silicon 구별 없음. PMULL 주석: *"apparently slow"* → 소프트웨어 XOR 체인으로 대체
- **결론**: Beast JSON의 3-level AArch64 분기는 simdjson에는 없는 차별화

### yyjson

- **컴파일 타임 only**: 런타임 dispatch 없음. 빌드 플래그 기반
- **AArch64**: `__aarch64__` 하나의 분기. Apple Silicon 구별 없음. `__ARM_FEATURE_UNALIGNED` 정도만 체크
- **결론**: yyjson보다 Beast JSON의 아키텍처 세분화가 월등히 앞섬

### Beast JSON의 전략적 위치

```
정교함 (아키텍처 세분화)
        ▲
        │  ★ Beast JSON  ← Apple Silicon / Generic AArch64 / SVE 3분할
        │
        │  simdjson      ← 런타임 dispatch (x86 강점), AArch64는 단일
        │
        │  yyjson        ← 컴파일 타임, 단일 AArch64
        │
        └─────────────────────────────────────────────────
                         대응 아키텍처 수
```
