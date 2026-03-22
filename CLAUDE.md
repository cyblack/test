# Claude Project Rules

## 우선순위 가이드

| 표시 | 의미 | 설명 |
|------|------|------|
| **MANDATORY** | 절대 규칙 | 반드시 준수. 위반 시 프로젝트 손상 가능 |
| **IMPORTANT** | 중요 규칙 | 가능한 한 준수. 합리적 이유가 있으면 예외 가능 |
| (표시 없음) | 권장 규칙 | 따르면 좋지만 상황에 따라 유연하게 적용 |

## Language
- 모든 답변은 한국어로 작성한다.
- 코드/로그/에러 메시지는 원문 유지, 설명은 한국어로 한다.

## 기술 스택

| 영역 | 기술 | 비고 |
|------|------|------|
| Frontend | React + Vite | 고정 |
| Backend | node | 고정 |
| Database | SQLite (개발) / PostgreSQL (프로덕션) | 고정 |
| 상태관리 | SPEC.md 정의에 따름 | 프로젝트별 선택 |
| HTTP/데이터 | SPEC.md 정의에 따름 | 프로젝트별 선택 |
| Chart | 필요 시 Recharts 권장 | 프로젝트별 선택 |
| Lint | ESLint (Frontend), Ruff (Backend) | 고정 |
| Test | Vitest (Frontend), pytest (Backend) | 고정 |


## Docs / 최신 문서 규칙 (IMPORTANT)

**라이브러리/프레임워크/SDK 사용법이 관련된 작업 시:**

1. **반드시 Context7 MCP를 사용**해 최신 문서 근거를 확인한다.
2. Context7 부족 시 **WebFetch로 공식 문서**에서 추가 확인한다.
3. 위 두 방법 불가 시에만 기존 지식 사용 (버전 주의 명시).

## 개발 환경 사전 점검 및 자동 설치 (MANDATORY)

> **"프로젝트 구현해줘" 등의 요청을 받으면, 코드 작성 전에 반드시 아래 순서를 따른다.**

### 프로젝트 구현 요청 시 전체 실행 순서

```
[Step 1] install.sh 실행 (시스템 도구 + MCP 도구 + 의존성 일괄 설치)
[Step 2] .git 삭제 → git init (새 Git 저장소 생성)
[Step 3] SPEC.md 검토 및 스펙 구체화
[Step 4] 필수 설정값 인터뷰 (프로젝트/배포 관련)
[Step 5] Plan Mode로 구현 계획 수립 (반드시 Phase 단위로 분할)
[Step 6] PROGRESS.md 생성 (Plan의 Phase 목록으로 초기화)
[Step 7] Phase별 구현 (각 Phase 완료 시마다):
        - 프로젝트 파일 생성/수정
        - 구현 중 필요한 의존성은 즉시 설치 (pip install / npm install)
        - DB 모델 작성 후: alembic init + alembic revision --autogenerate + alembic upgrade head
        - 린트/테스트 실행 → FAIL이면 수정 → PASS까지 반복 (테스트 가능한 Phase만)
        - PROGRESS.md 업데이트 + 체크포인트 커밋
[Step 8] 최종 린트/테스트 실행 → 전체 통합 검증
[Step 9] install.sh 재실행 (의존성 파일 최종 동기화 확인)
[Step 10] VERSION 파일 생성 (0.1.0) + PROGRESS.md 삭제 + 최종 커밋 + git tag v0.1.0
```

> **핵심**: `bash scripts/install.sh` 하나로 시스템 도구(git/node/python), MCP CLI 도구, 가상환경, Backend/Frontend 의존성이 모두 설치된다. MCP 환경변수(CONTEXT7_API_KEY, GITHUB_PAT)는 **선택사항**으로, 미설정 시 자동으로 skip된다.

### 주의사항
- install.sh는 **프로젝트 구현 요청 시** 가장 먼저 실행 (기능 추가/수정 시에는 불필요)
- 이미 설치되어 있으면 재설치하지 않고 건너뜀
- `.python-version`(3.11)과 `.nvmrc`(20) 파일의 버전을 기준으로 설치

---

## Git 초기화 규칙 (MANDATORY)

**프로젝트 최초 구현 시에만 .git 디렉토리를 초기화하고, 이후에는 절대 삭제하지 않는다.**

### 판단 기준

| 상황 | .git 삭제 여부 |
|------|---------------|
| "프로젝트 처음 만들어줘" / "SPEC.md 기반으로 구현해줘" | **O (삭제 후 재생성)** |
| 기능 추가/수정/버그 수정/리팩토링 | **X (절대 삭제 금지)** |
| "git 초기화해줘" (명시적 요청) | 히스토리 삭제 경고 후 재확인 |

### 최초 구현 시 순서
```bash
rm -rf .git          # 1. 기존 템플릿 .git 삭제
git init             # 2. 즉시 새 Git 저장소 생성 (Phase별 체크포인트 커밋을 위해)
# (Phase별 구현 + 체크포인트 커밋 반복)
# (최종 Phase 완료 후)
git tag v0.1.0       # 3. 버전 태그
```

## 필수 설정값 인터뷰

**install.sh가 다루지 않는 프로젝트별 설정값(프로젝트 정보, 외부 API, 배포 설정)은 인터뷰 형식으로 입력받는다.**

- 필수 설정값이 없으면 사용자에게 먼저 질문한다.
- 기본값이 있는 항목은 기본값 제시 후 확인.
- API 키 등 민감 정보는 `.env`에 저장, `.gitignore` 포함 확인.

## SPEC.md 검토 및 스펙 구체화 (IMPORTANT)

- SPEC.md를 먼저 읽고 요구사항을 파악한다.
- 모호하거나 불명확한 부분은 **반드시 사용자에게 질문** 후 구현.
- 가정을 세우기 전에 확인을 먼저 받는다.
- 구현 중 새로 발견된 애매한 요구사항/엣지 케이스도 즉시 인터뷰하고, 답변을 반영한 뒤 진행한다.

## Plan Mode 필수 (IMPORTANT)

- 코드 작성 전에 **항상 Plan Mode를 사용**하여 계획 수립.
- 사용자 승인 후 구현 시작.
- **예외**: 단순 오타 수정, 한 줄 변경, 사용자가 "바로 수정해줘" 요청 시.

### 대규모 작업 판단 기준
- 계획 수립 시 **Phase가 3개 이상**이면 대규모 작업으로 판단한다.
- 대규모 작업에만 Phase 분할 + PROGRESS.md + 체크포인트 커밋을 적용한다.
- 소규모 작업(Phase 2개 이하)은 Plan Mode만 거치고 일반 Workflow를 따른다.

## 대규모 작업 규칙 (IMPORTANT)

> **적용 조건**: Plan Mode에서 계획이 **Phase 3개 이상**으로 나뉘는 경우에만 적용한다.
> 소규모 기능 추가/수정/버그 수정 등은 이 규칙을 적용하지 않는다.

### PROGRESS.md 생성
- Plan Mode 승인 후, **코드 작성 전에 PROGRESS.md를 먼저 생성**한다.
- Plan의 Phase 목록을 그대로 옮겨 초기 상태로 작성한다.
- 형식:
  ```markdown
  # 구현 진행 상황
  ## 완료된 Phase
  (없음)
  ## 현재 진행 중
  - [ ] Phase 1: (내용)
  ## 남은 Phase
  - [ ] Phase 2: (내용)
  - [ ] Phase 3: (내용)
  - [ ] Phase 4: (내용)
  ```

### Phase 완료 시 반복 작업
각 Phase를 완료할 때마다 **반드시** 아래를 수행한다:
1. **린트/테스트 실행** → FAIL이면 해당 Phase 내에서 수정 → PASS까지 반복
   - 테스트 불가능한 Phase(프로젝트 구조 생성, 설정 파일 등)는 린트만 확인 후 건너뜀
   - **다음 Phase로 넘어가기 전에 반드시 현재 Phase가 정상 동작해야 한다**
2. PROGRESS.md 업데이트 (완료 체크 + 커밋 해시 기록 + 다음 Phase를 "현재 진행 중"으로 이동)
3. 체크포인트 커밋: `feat: Phase N - (Phase 설명)`
4. **사용자에게 `/clear` 실행 요청** → 다음 Phase로 넘어가기 전에 컨텍스트를 정리한다.

### 컨텍스트 중단 시 복구
- 대화가 중단된 후 재시작하면, **PROGRESS.md를 읽고** 마지막 완료 Phase 이후부터 이어서 진행한다.
- `git log`와 `git diff`로 현재 상태를 추가 확인한다.

### 최종 정리
- 모든 Phase 완료 후 PROGRESS.md를 삭제하고 최종 커밋에 포함한다.

## Frontend 개발 규칙

### 새 페이지는 새 URL로 생성
- 모달이나 조건부 렌더링으로 페이지를 대체하면 안 된다.
- **예외**: 삭제 확인 다이얼로그, 간단한 알림, 빠른 미리보기.

### 디자인 일관성 유지
- 새 페이지 구현 전 기존 페이지 코드를 먼저 분석.
- 기존 컴포넌트 재사용, 스타일(색상/폰트/간격) 통일.

### Writing / UI Guidelines
1. 이모지 금지 - 아이콘 컴포넌트(lucide-react) 사용
2. 중첩 카드뷰 금지 - 레이아웃 깊이 최대 2단계
3. 심플한 디자인 - "Less is more"
4. **반응형 레이아웃** - 모바일 우선, PC 최적화
   - 모바일: 단일 컬럼, 최소 padding (`px-2`)
   - PC: max-width 제한, 다중 컬럼 활용
5. 시간/날짜는 한국 시간(Asia/Seoul) 기준
6. fallback 더미 값 금지 - 에러 메시지를 명확히 노출
7. 사용자 작업에 성공/실패 메시지 항상 노출
8. **디자인 레퍼런스**: `design/reference.png` 이미지 확인 후 적용

### 파일 업로드 UI 패턴 (MANDATORY)
- **`<label htmlFor="file-input">` 방식만 사용** — 브라우저 네이티브 동작으로 모든 환경에서 안정적
- `button.onClick(() => ref.click())` 방식 **금지** — 일부 브라우저에서 파일 선택창이 열리지 않음
```tsx
// 올바른 패턴
<label htmlFor="file-input" className="cursor-pointer ...">
  파일 선택
</label>
<input id="file-input" type="file" className="hidden" onChange={...} />

// 금지 패턴
<button onClick={() => fileRef.current?.click()}>파일 선택</button>
<input ref={fileRef} type="file" className="hidden" />
```

### API 응답 타입 동기화 (IMPORTANT)
- **같은 도메인이라도 엔드포인트별 응답 구조가 다르면 별도 타입으로 정의**
- `types/index.ts`는 SPEC.md 섹션 12.3의 타입 일관성 표와 1:1 대응 유지
- 구현 전 반드시 SPEC.md의 Response 스키마 확인 후 타입 정의
  ```
  예시 — 혼용 금지:
  GET /feed          → items: PostDetail[]  (post 중첩: item.post.id)
  GET /users/*/posts → items: Post[]        (직접: item.id)
  ```

## 의존성 관리 및 환경 구축 (IMPORTANT)

### 스크립트 실행 방법
```bash
bash scripts/install.sh            # 의존성 설치
bash scripts/dev.sh                # 개발 서버 (전체)
bash scripts/dev.sh backend        # Backend만
bash scripts/dev.sh frontend       # Frontend만
bash scripts/test.sh               # 테스트 (전체)
bash scripts/test.sh lint          # 린트만
```

### 새 패키지 설치 워크플로우

| 환경 | 설치 명령 | 반영 파일 |
|------|----------|----------|
| Backend | `pip install 패키지명` | `backend/requirements.txt`에 수동 추가 |
| Frontend | `npm install 패키지명` | `frontend/package.json` 자동 반영 |

### Dockerfile과 패키지 관리

| 패키지 종류 | 수정 파일 | Dockerfile 수정 |
|------------|----------|----------------|
| Python (pip) | `backend/requirements.txt` | 불필요 |
| Node.js (npm) | `frontend/package.json` | 불필요 |
| 시스템 (apt) | `Dockerfile` | **필요 - 자동으로 추가** |

> 시스템 레벨 패키지(libpq-dev, ffmpeg 등)가 필요하면 Dockerfile에 직접 추가한다.

### 의존성 파일 통일
- 환경별 분리 금지 (`requirements-dev.txt` 등)
- `package.json`의 `devDependencies`는 npm 표준이므로 허용
- 환경별 동작 차이는 `.env`로 제어

---

## Workflow (MANDATORY)

코드 수정 후 항상:
1. **Lint 체크** → 에러 수정
2. **테스트 실행** → PASS/FAIL 확인
3. **결과 테이블 출력** (아래 형식)
4. FAIL이면 수정 후 재테스트
5. PASS면 사용자에게 커밋 여부 확인 → 승인 시 `git add` → `git commit`
6. `git push`는 사용자가 명시적으로 요청할 때만

### 테스트 결과 출력 형식 (필수)
```
| 구분 | 결과 | 상세 |
|------|------|------|
| Frontend Lint | PASS/FAIL | 에러 수 또는 "OK" |
| Backend Lint | PASS/FAIL | 에러 수 또는 "OK" |
| Frontend Test | PASS/FAIL | 통과/전체 (예: 10/10) |
| Backend Test | PASS/FAIL | 통과/전체 (예: 15/15) |
| **최종 결과** | **PASS/FAIL** | - |
```

### Commit 규칙
- 커밋 메시지는 한국어, Conventional Commits 사용
- "자동으로 커밋해" 요청 시 확인 없이 자동 커밋 가능

## 테스트 (IMPORTANT)

- 새 기능 추가 시 반드시 테스트 코드 함께 작성
- 테스트 없이 기능만 추가하는 것은 미완성 작업
- 파일 명명: `test_*.py` (Backend), `*.test.ts(x)` (Frontend)
- AAA 패턴: Arrange → Act → Assert

### SPEC 기반 테스트 자동 생성 (프로젝트 구현 시)

> **프로젝트 구현 완료 후 반드시 아래 순서를 따른다:**

1. **SPEC.md 섹션 12 (Testing Requirements)를 읽고** 테스트 케이스 목록 파악
2. **Backend**: `12.1 API Contract Tests` 표의 모든 행을 pytest로 작성
   - 각 엔드포인트 × 테스트 케이스 = 1개의 테스트 함수
   - 상태코드 + 응답 구조 모두 검증
3. **Frontend**: `12.2 Component Render Tests` 표의 모든 행을 Vitest로 작성
   - 컴포넌트 렌더링 + 핵심 UI 요소 존재 여부 검증
4. **타입 일관성**: `12.3 타입 일관성 검증` 표 기준으로 응답 구조 오용 없는지 확인
5. 테스트 실행 → **FAIL이면 구현 버그로 간주하고 수정** → 전체 PASS까지 반복

### TEST.md 기반 검증 워크플로우 (MANDATORY)

> **목적**: SPEC.md 요구사항을 테스트 가능한 시나리오로 고정하고, 구현 완료 조건을 명확히 한다.

1. 프로젝트 구현 시작 시 `SPEC.md`를 기준으로 루트 `TEST.md`를 생성/갱신한다.
2. `TEST.md` 초안 작성 직후 사용자에게 아래를 **반드시 인터뷰**한다:
   - 시나리오가 SPEC.md 요구사항을 정확히 반영하는지
   - 추가하고 싶은 사용자 시나리오가 있는지
3. 인터뷰 답변을 반영해 `TEST.md`를 확정하고, 확정본을 **검증 단일 기준(Single Source of Truth for validation)** 으로 사용한다.
4. 구현 완료 후 반드시 `TEST.md`의 모든 시나리오를 실제 테스트 코드/명령으로 검증한다.
5. 하나라도 FAIL이면 구현 미완료로 간주하고 수정 후 재검증한다.
6. `TEST.md` 전 시나리오가 PASS 될 때까지 **구현 → 테스트 실행 → 실패 원인 수정 → 재테스트** 사이클을 반복한다.
7. 최종 보고 시 `TEST.md` 기준 결과 테이블(PASS/FAIL, 상세)을 포함한다.
8. `SPEC.md`의 기능/API/타입이 변경되면 `TEST.md`를 즉시 동기화하고 필요한 인터뷰를 다시 수행한다.
9. 권장 실행 순서:
   - `bash scripts/test.sh lint`
   - `bash scripts/test.sh`
   - 필요 시 `bash scripts/test.sh --coverage`

### TDD 기반 구현 규칙 (MANDATORY)

> **목표**: SPEC.md 요구사항을 TEST.md 시나리오로 변환하고, TEST.md PASS를 목표로 TDD(RED → GREEN → REFACTOR) 방식으로 구현한다.

1. 구현 시작 전 `SPEC.md`를 읽고 `TEST.md`를 최신화한다.
2. `TEST.md` 시나리오는 **항상 SPEC.md에서 추출**한다. 특정 도메인(예: 인스타그램 기능) 시나리오를 고정 문구로 강제하지 않는다.
   - 추출 기준: `Core Features & User Stories`, `API Design`, `Business Rules`, `Frontend Routes`
   - 작성 원칙: "사용자 행동 → 기대 결과" 형태로 기능별 시나리오를 정의
   - SPEC.md에 없는 기능은 TEST.md에 임의로 추가하지 않는다.
3. 각 사용자 시나리오는 테스트 코드로 분해한다:
   - Unit Test (핵심 로직/유틸)
   - Integration Test (API/DB/상태 연동)
   - 필요 시 E2E 또는 사용자 흐름 테스트
4. 구현 순서는 반드시 TDD를 따른다:
   - RED: 실패하는 테스트를 먼저 작성
   - GREEN: 테스트를 통과시키는 최소 코드 구현
   - REFACTOR: 중복 제거/구조 개선 후 전체 테스트 재통과
5. 구현 중 판단이 갈리는 애매한 사항(요구사항 해석, 예외 처리, UX 흐름, 데이터 규칙)은 임의 결정하지 말고 사용자에게 인터뷰 후 진행한다.
6. 테스트 없이 기능 코드만 먼저 작성하는 방식은 금지한다.
7. 완료 전까지 `TEST.md` 시나리오를 기준으로 RED/GREEN 사이클을 반복하며 미통과 항목을 0으로 만든다.
8. 완료 조건은 `TEST.md` 전 항목 PASS + 린트 PASS + 타입 일관성 PASS다.
9. 최종 보고에는 아래를 포함한다:
   - TEST.md 시나리오별 PASS/FAIL
   - 새로 추가된 unit/integration 테스트 목록
   - 미통과 항목(있는 경우)과 원인/후속 조치

## Database 규칙 (IMPORTANT)

### ORM 필수
- Raw SQL 직접 작성 금지 - SQLAlchemy ORM만 사용
- 복잡한 쿼리도 ORM으로 작성

### 마이그레이션 필수
- 모델 변경 후 반드시 `alembic revision --autogenerate` → `alembic upgrade head`
- 마이그레이션 파일은 Git에 커밋

### DB & API 동기화
- 스키마 변경 시 관련 API(Pydantic 모델 포함)도 함께 업데이트

## Backend Configuration (IMPORTANT)

### CORS 설정
```python
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,  # allow_origins=["*"] 시 반드시 False
    allow_methods=["*"],
    allow_headers=["*"],
)
```
> **주의**: `allow_origins=["*"]`와 `allow_credentials=True`는 CORS 스펙상 동시 사용 불가. 프로덕션에서는 특정 origin을 지정하고 `allow_credentials=True`로 변경.

### Trailing Slash 통일
- **Trailing Slash 없이 통일**: `/api/v1/users` (O) / `/api/v1/users/` (X)
- Backend: `app.router.redirect_slashes = False`

## Backend 설계 원칙

### 12-Factor App
- Config: 환경 변수로 관리 - 하드코딩 금지
- Dependencies: `requirements.txt`에 명시적 선언
- Port Binding: `PORT` 환경 변수 사용

### Unix Philosophy (리팩토링 시)
- 각 함수/클래스는 한 가지 일만
- 비즈니스 로직(services/)과 인프라 코드(repositories/) 분리

### 백엔드 모듈 구조
- SPEC.md의 프로젝트 구조 정의를 따른다.

## Git
- commit message: 한국어, Conventional Commits
- 환경 제약으로 git 실패 시 원인/대안 안내

## 버전 관리 (SemVer) (IMPORTANT)

- `VERSION` 파일이 단일 버전 소스 (Single Source of Truth)
- 최초 `0.1.0`으로 시작
- **매 커밋이 아닌 릴리스/배포 시점에 버전 업데이트**
- `fix:` → PATCH, `feat:` → MINOR, `BREAKING CHANGE` → MAJOR

## 컨텍스트 경량화
- 파일 탐색/조사는 Task(Explore)로 위임한다.
- 테스트/린트 실행은 Task(Bash)로 위임한다.
- 큰 파일은 필요한 부분만 Read(offset/limit)한다.

## Debugging & Logging

- **개발**: stdout + `backend/debug.log` / `frontend/debug.log`
- **프로덕션**: stdout만 (12-Factor App 준수)
- Backend: Python `logging` 모듈
- Frontend: console.log 기반 로거

