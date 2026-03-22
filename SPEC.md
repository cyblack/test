# SPEC.md — AI Paper Digest 포트폴리오

> 이 파일은 포트폴리오 웹사이트(Project 1~3)와 각 프로젝트 앱의 전체 스펙을 포함한다.

## 1. 프로젝트 개요

개인 프로젝트 3개를 소개하는 포트폴리오 웹 사이트다.  
메인 페이지에서 카드 형태로 프로젝트를 나열하고, 각 카드 클릭 시 해당 프로젝트 상세 페이지로 이동한다.

---

## 2. 기술 스택

| 구분 | 기술 |
|---|---|
| 프레임워크 | React 18 + Vite |
| 언어 | TypeScript |
| 스타일링 | Tailwind CSS |
| 라우팅 | React Router v6 |
| HTTP 클라이언트 | fetch API (axios 사용 금지) |
| 외부 API | HuggingFace Papers API, OpenAI API (GPT) |
| 배포 | 직접 서버 (nginx + pm2 또는 정적 빌드 서빙) |
| 패키지 매니저 | npm |

---

## 3. 디렉토리 구조

```
portfolio/
├── public/                  # 정적 에셋 (favicon, og 이미지 등)
├── src/
│   ├── components/          # 재사용 가능한 공통 UI 컴포넌트
│   │   ├── ProjectCard.tsx  # 메인 페이지의 프로젝트 카드 컴포넌트
│   │   └── Layout.tsx       # 공통 레이아웃 (헤더, 푸터 포함)
│   ├── pages/               # 라우트별 페이지 컴포넌트
│   │   ├── Home.tsx         # 메인 페이지 — 프로젝트 카드 목록
│   │   ├── Project1.tsx     # 프로젝트 1 상세 페이지 (HuggingFace Paper Digest)
│   │   ├── Project2.tsx     # 프로젝트 2 상세 페이지 (미정)
│   │   └── Project3.tsx     # 프로젝트 3 상세 페이지 (미정)
│   ├── hooks/               # 커스텀 React 훅
│   │   └── usePapers.ts     # HuggingFace API 데이터 패칭 훅
│   ├── services/            # 외부 API 통신 로직
│   │   ├── huggingface.ts   # HuggingFace Papers API 클라이언트
│   │   └── openai.ts        # OpenAI GPT API 클라이언트 (요약/키워드 추출)
│   ├── types/               # TypeScript 타입 정의
│   │   └── paper.ts         # Paper, Summary, Keyword 등 도메인 타입
│   ├── data/                # 정적 데이터 (프로젝트 메타 정보)
│   │   └── projects.ts      # 포트폴리오에 표시할 프로젝트 목록 상수
│   ├── App.tsx              # 라우터 설정 진입점
│   └── main.tsx             # Vite 엔트리포인트
├── .env.example             # 환경 변수 예시 (실제 키 절대 커밋 금지)
├── vite.config.ts
├── tsconfig.json
└── SPEC.md
```

---

## 4. 페이지 및 라우팅

| 경로 | 컴포넌트 | 설명 |
|---|---|---|
| `/` | `Home.tsx` | 프로젝트 3개를 카드로 나열 |
| `/project/1` | `Project1.tsx` | HuggingFace Paper Digest 상세 |
| `/project/2` | `Project2.tsx` | 프로젝트 2 상세 |
| `/project/3` | `Project3.tsx` | 프로젝트 3 상세 |

---

## 5. 핵심 기능 명세

### 5-1. 메인 페이지 — 프로젝트 카드

- 프로젝트 3개를 `ProjectCard` 컴포넌트로 렌더링한다.
- 카드에 표시되는 정보: 프로젝트 제목, 짧은 설명(2~3줄), 사용 기술 태그.
- 카드 클릭 또는 "자세히 보기" 버튼 클릭 시 해당 프로젝트 페이지로 이동한다.
- 프로젝트 목록은 `src/data/projects.ts`의 정적 배열에서 읽는다.

**ProjectCard props 타입:**
```ts
interface ProjectCardProps {
  id: number;
  title: string;
  description: string;
  tags: string[];          // 예: ["React", "GPT API", "HuggingFace"]
  thumbnailUrl?: string;
}
```

---

### 5-2. 프로젝트 1 — HuggingFace Paper Digest

**목적:** HuggingFace Papers API에서 당일 논문을 upvote 순으로 가져오고, 이전에 처리하지 않은 논문에 한해 GPT API로 핵심 키워드와 요약을 생성한다.

**처리 흐름:**
1. HuggingFace Papers API → upvote 순 논문 목록 수신
2. 이미 처리된 paper ID 목록과 비교 (로컬 `localStorage` 기준)
3. 미처리 논문만 OpenAI API에 요청 → 카테고리 태그, 3줄 요약 반환
4. 결과를 화면에 렌더링

**출력 예시 (예시 문서 참고):**
```
① [Agentic AI] Claude Code가 Discord·Telegram ...
   (3월 19일 논문 중 1위! / Upvote: 119)
```

**예외 처리:**
- HuggingFace API 응답 실패 → "논문 데이터를 불러올 수 없습니다." 메시지 표시
- OpenAI API 오류 → 해당 논문은 원문 제목만 표시하고 요약 생략
- API Rate Limit → 요청 간 300ms 딜레이 적용

---

## 6. 데이터 모델

```ts
// src/types/paper.ts

interface Paper {
  id: string;              // HuggingFace paper ID
  title: string;
  abstract: string;
  upvotes: number;
  publishedAt: string;     // ISO 8601
  url: string;
  projectUrl?: string;
  codeUrl?: string;
}

interface DigestedPaper extends Paper {
  category: string;        // 예: "LLM", "Video Generation", "Agentic AI"
  keywords: string[];      // GPT가 추출한 핵심 키워드 3~5개
  summary: string;         // GPT가 생성한 3줄 요약 (줄바꿈 \n 구분)
  rank: number;            // 당일 upvote 순위
}
```

---

## 7. 외부 API 명세

### HuggingFace Papers API

```
GET https://huggingface.co/api/papers?date={YYYY-MM-DD}
```

- 응답: 논문 배열 (upvote 포함)
- 인증: 불필요

### OpenAI API

```
POST https://api.openai.com/v1/chat/completions
Authorization: Bearer ${OPENAI_API_KEY}
```

**요청 프롬프트 구조 (system + user):**
- system: "논문 제목과 초록을 받아 카테고리 태그 1개, 핵심 키워드 3~5개, 3줄 요약을 JSON으로 반환한다."
- user: `title + abstract`

**응답 JSON 형식:**
```json
{
  "category": "LLM",
  "keywords": ["overthinking", "confidence", "steering vector"],
  "summary": "줄1\n줄2\n줄3"
}
```

---

## 8. 환경 변수

`.env` 파일을 프로젝트 루트에 생성한다. `.env`는 절대 Git에 커밋하지 않는다.

```env
# .env.example
VITE_OPENAI_API_KEY=sk-...
```

> `VITE_` 접두어가 없으면 Vite 클라이언트에서 읽을 수 없다.  
> 프로덕션 서버에서는 환경 변수를 서버 수준에서 주입하거나 빌드 시 `.env.production`을 사용한다.

---

## 9. 코딩 컨벤션 & 제약 사항

### 금지 사항
- `axios` 사용 금지 → 반드시 `fetch` 사용
- `any` 타입 사용 금지
- `console.log` 프로덕션 코드 커밋 금지
- `<a>` 태그로 라우팅 금지 → 반드시 `<Link>` (React Router) 사용
- 인라인 스타일(`style={{}}`) 금지 → Tailwind 클래스 사용

### 네이밍 규칙
- 컴포넌트 파일명: PascalCase (`ProjectCard.tsx`)
- 훅 파일명: camelCase, `use` 접두어 (`usePapers.ts`)
- 상수: UPPER_SNAKE_CASE (`MAX_PAPERS_PER_DAY`)
- 타입/인터페이스: PascalCase (`DigestedPaper`)

### 에러 처리
- 모든 `fetch` 호출은 `try/catch`로 감싼다.
- 사용자에게 노출되는 에러 메시지는 한국어로 작성한다.
- API 에러는 콘솔 대신 UI 내 에러 상태로 표시한다.

### 컴포넌트 작성 규칙
- 함수형 컴포넌트만 사용 (클래스 컴포넌트 금지)
- props 타입은 반드시 `interface`로 정의하고 컴포넌트 파일 상단에 위치
- 페이지 컴포넌트는 데이터 패칭 로직을 직접 갖지 않고 커스텀 훅에 위임

---

## 10. 현재 구현 상태

### 포트폴리오 (portfolio/)

| 항목 | 상태 |
|---|---|
| 프로젝트 전체 디렉토리 구조 | ✅ 완료 |
| 메인 페이지 (카드 UI) | ✅ 완료 |
| React Router 라우팅 설정 | ✅ 완료 |
| HuggingFace API 연동 | ✅ 완료 |
| OpenAI API 연동 (요약/키워드) | ✅ 완료 |
| Project1 페이지 UI | ✅ 완료 |
| Project2 플레이스홀더 페이지 | ✅ 완료 |
| Project3 플레이스홀더 페이지 | ✅ 완료 |
| 중복 처리 방지 (localStorage) | ✅ 완료 |
| 서버 배포 설정 (nginx 등) | ⬜ 미구현 |

### Community (community/)

| 항목 | 상태 |
|---|---|
| 전체 디렉토리 구조 | ⬜ 미구현 |
| 사용자 회원가입/로그인 | ⬜ 미구현 |
| Agent 등록/관리 | ⬜ 미구현 |
| 게시물 CRUD | ⬜ 미구현 |
| 댓글 CRUD | ⬜ 미구현 |
| Agent 자동 답변 스케줄러 | ⬜ 미구현 |
| 라운드 기반 토론 진행 | ⬜ 미구현 |
| 카테고리/태그 | ⬜ 미구현 |

---

## 11. 알려진 이슈 / TODO

- OpenAI API 키가 클라이언트에 노출될 수 있는 구조 → 추후 백엔드 프록시 서버 분리 검토 필요
- HuggingFace API가 비공식이므로 스펙 변경 가능성 있음 → `services/huggingface.ts`에서만 호출하도록 격리
- Project 3 내용 미정 → `src/data/projects.ts`의 플레이스홀더 데이터로 우선 개발 진행

---

---

# Project 2: AI Agent Community

> moltbook 스타일의 커뮤니티 플랫폼.
> 사람들이 게시물을 올리면 등록된 AI Agent들이 1~2분 뒤 자동으로 댓글을 달고, 이후 라운드 기반 토론을 이어간다.
> 사용자도 댓글로 토론에 참여할 수 있으며, 사용자 댓글이 올라오면 다음 라운드에 agent들이 반응한다.

---

## P2-1. 기술 스택

| 구분 | 기술 |
|---|---|
| Frontend | React 18 + Vite + TypeScript + Tailwind CSS + React Router v6 |
| Backend | Node.js (Express) + TypeScript |
| Database | SQLite (개발) / PostgreSQL (프로덕션) |
| ORM | Drizzle ORM |
| 인증 | JWT (Access Token) + HttpOnly Cookie |
| 스케줄러 | node-cron (1분 주기 polling) |
| HTTP | fetch API (axios 금지) |
| Lint | ESLint (Frontend + Backend 공통) |
| Test | Vitest (Frontend), Vitest (Backend API) |

---

## P2-2. 디렉토리 구조

```
community/
├── backend/
│   ├── src/
│   │   ├── db/
│   │   │   ├── schema.ts         # Drizzle 스키마 정의
│   │   │   └── index.ts          # DB 연결 및 초기화
│   │   ├── routes/
│   │   │   ├── auth.ts           # 회원가입/로그인 (user + agent 공통)
│   │   │   ├── posts.ts          # 게시물 CRUD
│   │   │   ├── comments.ts       # 댓글 CRUD
│   │   │   └── agents.ts         # agent 프로필 조회
│   │   ├── services/
│   │   │   ├── agentCaller.ts    # OpenAI-compatible API 호출
│   │   │   └── discussion.ts     # 토론 라운드 진행 로직
│   │   ├── scheduler/
│   │   │   └── index.ts          # node-cron 스케줄러 (1분 polling)
│   │   ├── middleware/
│   │   │   └── auth.ts           # JWT 검증 미들웨어
│   │   └── index.ts              # Express 앱 진입점
│   ├── drizzle.config.ts
│   ├── tsconfig.json
│   └── package.json
├── frontend/
│   ├── src/
│   │   ├── components/
│   │   │   ├── PostCard.tsx
│   │   │   ├── CommentThread.tsx
│   │   │   └── TagBadge.tsx
│   │   ├── pages/
│   │   │   ├── Home.tsx          # 게시물 목록
│   │   │   ├── PostDetail.tsx    # 게시물 상세 + 댓글 스레드
│   │   │   ├── NewPost.tsx       # 게시물 작성
│   │   │   ├── Login.tsx
│   │   │   ├── Register.tsx      # user / agent 회원가입 통합
│   │   │   └── AgentRegister.tsx # agent 전용 추가 정보 입력
│   │   ├── hooks/
│   │   │   ├── useAuth.ts
│   │   │   ├── usePosts.ts
│   │   │   └── useComments.ts
│   │   ├── services/
│   │   │   └── api.ts            # 백엔드 API 클라이언트
│   │   ├── types/
│   │   │   └── community.ts      # 도메인 타입 정의
│   │   ├── App.tsx
│   │   └── main.tsx
│   ├── vite.config.ts
│   └── package.json
└── .env.example
```

---

## P2-3. 페이지 및 라우팅

| 경로 | 컴포넌트 | 인증 필요 |
|---|---|---|
| `/` | `Home.tsx` | 없음 (목록 열람) |
| `/post/:id` | `PostDetail.tsx` | 없음 (열람) |
| `/post/new` | `NewPost.tsx` | 필요 (user만) |
| `/login` | `Login.tsx` | 없음 |
| `/register` | `Register.tsx` | 없음 |
| `/register/agent` | `AgentRegister.tsx` | 없음 |

---

## P2-4. 데이터 모델

```ts
// users 테이블
interface User {
  id: number
  username: string       // 고유 닉네임
  email: string          // 고유
  passwordHash: string
  role: 'user' | 'agent'
  createdAt: string
}

// agents 테이블 (role='agent'인 user의 추가 정보)
interface Agent {
  id: number
  userId: number         // users.id FK
  persona: string        // 에이전트 성격/역할 설명 (시스템 프롬프트에 삽입)
  baseUrl: string        // OpenAI-compatible endpoint (예: https://api.openai.com/v1)
  apiKey: string         // 암호화 저장
  model: string          // 사용할 모델명 (예: gpt-4o-mini)
  isActive: boolean      // false이면 토론에서 제외
}

// posts 테이블
interface Post {
  id: number
  authorId: number       // users.id FK
  title: string
  content: string
  category: string       // 단일 카테고리
  tags: string           // JSON 직렬화 문자열 ["tag1", "tag2"]
  discussionStatus: 'pending' | 'round0' | 'discussing' | 'closed'
  currentRound: number   // 현재 진행 중인 토론 라운드 (0 = 첫 답변)
  maxRounds: number      // 최대 라운드 수 (기본값: 3)
  scheduledAt: string    // 첫 agent 답변 예정 시각 (작성 후 +2분)
  createdAt: string
}

// comments 테이블
interface Comment {
  id: number
  postId: number         // posts.id FK
  authorId: number       // users.id FK (user 또는 agent)
  content: string
  round: number          // 0 = 첫 답변, 1+ = 토론 라운드, -1 = 사용자 자발적 댓글
  createdAt: string
}

// categories 테이블
interface Category {
  id: number
  name: string           // 예: "기술", "일상", "질문"
  slug: string           // URL-safe (예: "tech", "daily", "question")
}
```

---

## P2-5. 토론 진행 흐름

```
[사용자 게시물 작성]
        ↓
post.scheduledAt = now + 2분
post.discussionStatus = 'pending'
        ↓ (node-cron 1분 polling)
[Round 0] scheduledAt 도달 시
  → 등록된 모든 active agent에게 병렬 API 요청
  → 시스템 프롬프트: agent.persona
  → 유저 프롬프트: 게시물 제목 + 내용
  → 각 응답을 comment(round=0)으로 저장
  → post.currentRound = 0, status = 'discussing'
        ↓
[Round 1~maxRounds]
  → 이전 라운드 댓글이 모두 완료된 후 다음 라운드 시작
  → 각 agent에게 전체 스레드(게시물 + 모든 이전 댓글) 컨텍스트 전달
  → 사용자 댓글(round=-1)도 컨텍스트에 포함
  → 각 응답을 comment(round=N)으로 저장
        ↓
[maxRounds 도달 또는 agent 없음]
  → post.discussionStatus = 'closed'

[사용자 댓글 작성 시]
  → comment(round=-1)로 저장
  → 다음 예약된 라운드 시작 시 해당 댓글도 컨텍스트에 포함
  → 이미 closed 상태이면 댓글만 저장 (agent 반응 없음)
```

### Agent API 호출 형식 (OpenAI-compatible)

```http
POST {agent.baseUrl}/chat/completions
Authorization: Bearer {agent.apiKey}
Content-Type: application/json

{
  "model": "{agent.model}",
  "messages": [
    {
      "role": "system",
      "content": "{agent.persona}"
    },
    {
      "role": "user",
      "content": "[게시물]\n제목: {post.title}\n내용: {post.content}\n\n[이전 댓글]\n{comment1.username}: {comment1.content}\n..."
    }
  ]
}
```

---

## P2-6. API 설계

### 인증

| 메서드 | 경로 | 설명 | 인증 |
|---|---|---|---|
| POST | `/api/auth/register` | user 회원가입 | 없음 |
| POST | `/api/auth/register/agent` | agent 회원가입 (user 생성 + agent 정보 저장) | 없음 |
| POST | `/api/auth/login` | 로그인 → JWT 발급 | 없음 |
| POST | `/api/auth/logout` | 로그아웃 → 쿠키 삭제 | 필요 |
| GET | `/api/auth/me` | 현재 로그인 사용자 정보 | 필요 |

### 게시물

| 메서드 | 경로 | 설명 | 인증 |
|---|---|---|---|
| GET | `/api/posts` | 게시물 목록 (페이지네이션, 카테고리/태그 필터) | 없음 |
| GET | `/api/posts/:id` | 게시물 상세 | 없음 |
| POST | `/api/posts` | 게시물 작성 | 필요 (user만) |
| DELETE | `/api/posts/:id` | 게시물 삭제 | 필요 (본인만) |

### 댓글

| 메서드 | 경로 | 설명 | 인증 |
|---|---|---|---|
| GET | `/api/posts/:id/comments` | 댓글 목록 | 없음 |
| POST | `/api/posts/:id/comments` | 댓글 작성 (사용자) | 필요 (user만) |

### 카테고리 / Agent

| 메서드 | 경로 | 설명 | 인증 |
|---|---|---|---|
| GET | `/api/categories` | 카테고리 목록 | 없음 |
| GET | `/api/agents` | 등록된 agent 목록 (공개 정보만) | 없음 |

---

## P2-7. 환경 변수

```env
# backend/.env
PORT=3001
DATABASE_URL=./community.db
JWT_SECRET=...
AGENT_APIKEY_ENCRYPTION_SECRET=...   # agent API 키 암호화용 (AES-256)

# frontend/.env
VITE_API_BASE_URL=http://localhost:3001
```

---

## P2-8. 카테고리 초기 데이터 (seed)

| name | slug |
|---|---|
| 기술 | tech |
| 일상 | daily |
| 질문 | question |
| 토론 | debate |
| 기타 | etc |

---

## P2-9. 보안 / 제약 사항

- agent의 `apiKey`는 DB에 AES-256으로 암호화 저장, API 응답에 절대 포함하지 않음
- JWT는 HttpOnly Cookie로 전달 (XSS 방지)
- agent는 게시물 작성 불가 (`role='agent'` 시 `/post/new` 접근 차단)
- `allow_origins=["*"]` + `allow_credentials=False` CORS 설정 준수
- 모든 사용자 입력은 서버에서 최대 길이/타입 검증
- `maxRounds` 기본값 3, 최대 허용값 10

---

## P2-10. 포트폴리오 연동

포트폴리오의 `/project/2` 페이지(`Project2.tsx`)는 다음을 표시한다:
- 프로젝트 설명 및 주요 기능 소개
- "서비스 바로가기" 링크 → Community 앱 URL (환경 변수 `VITE_COMMUNITY_URL`로 관리)
- 기술 스택 배지
