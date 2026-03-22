/*
 * =============================================================================
 *  화면 기능 최적 제어 알고리즘 (user2.cpp)
 * =============================================================================
 *
 *  [문제 요약]
 *  - 20개 화면 기능(feature)의 값(0~100)을 1000 time step 동안 조절
 *  - 800명 사용자의 만족도를 최대화하되, 매 step 전력 예산 380 이내
 *  - 최종 점수 = Σ (satisfaction - powerUsed × 0.01)
 *
 *  [핵심 수학]
 *  - 만족도: Σ_u Σ_i  v[i] × quality[i] × 0.01 / (|median[i] - dist[u]| + 1)
 *           = Σ_i  v[i] × S[i]      ← v[i]에 "선형"
 *  - 전력:   Σ_i  power[i] × v[i]² / 10000 + |prev[i] - v[i]| × 0.01
 *                                      ← v[i]에 "이차(제곱)"
 *  → 한계수익체감 구조이므로, Lagrange multiplier로 예산 내 최적 배분 가능
 *
 *  [알고리즘 흐름]  (11단계 파이프라인)
 *   1) S[i] 계산           - feature별 한계 만족도
 *   2) Lagrange 이진탐색   - 예산 제약 하 연속 최적해 탐색
 *   3) 연속 최적해 확정
 *   4) 스마트 정수 반올림   - floor 시작 → ceil 효율순 배정 (배낭 그리디)
 *   5) 예산 초과 안전 감소
 *   6) Greedy +1           - 남은 예산 최대 활용
 *   7) 1-for-1 Swap        - 비효율 feature ↔ 고효율 feature 교환
 *   8) Swap 후 Greedy +1
 *   9) 2-for-1 Swap        - 비효율 feature -2 → 고효율 feature +1
 *  10) 1-for-2 Swap        - 비효율 feature -1 → 고효율 feature +2
 *  11) 최종 Greedy +1      - 빈틈 없이 예산 소진
 * =============================================================================
 */

#define MAX_FEATURES    20
#define MAX_USERS      800
#define TIME_STEPS    1000
#define MIN_DISTANCE   100
#define MAX_DISTANCE  2500
#define POWER_BUDGET   380

/*
 * SAFE_BUDGET: 부동소수점 오차로 인한 예산 초과 방지용 마진
 * main.cpp의 screen_control()에서 powerUsed > 380 이면 즉시 실격(-1)되므로,
 * 우리 쪽에서는 약간 보수적인 예산(379.9999)을 사용한다.
 */
#define BUDGET_EPS     0.0001
#define SAFE_BUDGET    (POWER_BUDGET - BUDGET_EPS)

extern void feature_info(int[], double[], int[]);
extern void user_info(int[]);
extern void screen_control(const int[]);

static int fPow[MAX_FEATURES];     // feature별 전력 계수 (10~80)
static double fQual[MAX_FEATURES]; // feature별 품질 계수 (0.10~2.50)
static int fMed[MAX_FEATURES];     // feature별 최적 거리 (100~2500)
static int uDist[MAX_USERS];       // 사용자별 현재 거리
static int pv[MAX_FEATURES];       // 직전 step의 feature 값 (전환비용 계산용)

static inline int iabs(int x) { return x < 0 ? -x : x; }

/*
 * fp(i, v): feature i를 value v로 설정할 때의 전력 소모량
 *
 * = (기능 전력) + (전환 비용)
 * = power[i] × v² / 10000  +  |prev[i] - v| × 0.01
 *
 * 전환비용: 이전 값에서 변경할수록 추가 전력 소모 → 값을 안정적으로 유지하는 것이 유리
 */
static inline double fp(int i, int v) {
    return fPow[i] * (double)v * v / 10000.0 + iabs(pv[i] - v) * 0.01;
}

/*
 * tp(fv): 전체 feature 배열의 총 전력 소모량
 */
static double tp(const int fv[]) {
    double p = 0;
    for (int i = 0; i < MAX_FEATURES; i++) p += fp(i, fv[i]);
    return p;
}

void process(void)
{
    feature_info(fPow, fQual, fMed);
    for (int i = 0; i < MAX_FEATURES; i++) pv[i] = 0;

    for (int t = 0; t < TIME_STEPS; t++) {
        user_info(uDist);

        // ============================================================
        // [1단계] S[i] 계산: feature i의 value를 1 올렸을 때 얻는 만족도 증가량
        //
        // S[i] = quality[i] × 0.01 × Σ_u ( 1 / (|median[i] - dist[u]| + 1) )
        //
        // - user가 median에 가까울수록 기여가 큼 (분모가 작아짐)
        // - quality가 높을수록 기여가 큼
        // - 이 값이 클수록 해당 feature에 전력을 투자할 가치가 높음
        // ============================================================
        double S[MAX_FEATURES];
        for (int i = 0; i < MAX_FEATURES; i++) {
            double sum = 0;
            for (int u = 0; u < MAX_USERS; u++) {
                int d = uDist[u];
                if (d < MIN_DISTANCE) d = MIN_DISTANCE;
                if (d > MAX_DISTANCE) d = MAX_DISTANCE;
                int diff = fMed[i] - d;
                if (diff < 0) diff = -diff;
                sum += 1.0 / (diff + 1);
            }
            S[i] = fQual[i] * 0.01 * sum;
        }

        // ============================================================
        // [2단계] Lagrange 이진탐색: 예산 제약 하 연속 최적해 탐색
        //
        // 목적: maximize Σ S[i]·v[i] - 0.01 × totalPower
        // 제약: totalPower ≤ 380
        //
        // Lagrange multiplier μ를 도입하면, 각 feature별로 독립적인 해를 구할 수 있다:
        //   c = 0.01 + μ  (score 패널티 0.01 + 예산 제약 μ)
        //
        // 전환비용의 방향성 때문에 3가지 케이스로 분리:
        //   v ≥ prev: v_up = (S[i] - c×0.01) / (c×P[i]/5000)  ← 증가 시 전환비용 추가
        //   v < prev: v_dn = (S[i] + c×0.01) / (c×P[i]/5000)  ← 감소 시 전환비용 절약
        //   그 외:    v = prev[i]                                ← 변경 안 하는 게 최적
        //
        // μ를 이진탐색으로 조절하여 총 전력 ≈ 예산이 되는 지점을 찾는다.
        // ============================================================
        double lo = 0, hi = 100.0;
        for (int iter = 0; iter < 80; iter++) {
            double mu = (lo + hi) * 0.5;
            double c = 0.01 + mu, tr = c * 0.01;
            double pw = 0;
            for (int i = 0; i < MAX_FEATURES; i++) {
                double den = c * fPow[i] / 5000.0;
                double vU = (S[i] - tr) / den;  // v >= prev 케이스의 최적해
                double vD = (S[i] + tr) / den;  // v <  prev 케이스의 최적해
                double v;
                if      (vU >= pv[i]) v = vU;    // 증가 방향이 최적
                else if (vD <  pv[i]) v = vD;    // 감소 방향이 최적
                else                  v = pv[i]; // 유지가 최적
                if (v < 0) v = 0; if (v > 100) v = 100;
                pw += fPow[i] * v * v / 10000.0
                    + (v >= pv[i] ? v - pv[i] : pv[i] - v) * 0.01;
            }
            if (pw > SAFE_BUDGET) lo = mu; else hi = mu; // 예산초과라면 벌금을 더쎼게 ..lo=hi
        }

        // ============================================================
        // [3단계] 찾은 μ로 연속 최적해 확정
        // ============================================================
        double mu = (lo + hi) * 0.5;
        double c = 0.01 + mu, tr = c * 0.01;
        double contV[MAX_FEATURES];  // 연속 최적해 (실수)
        for (int i = 0; i < MAX_FEATURES; i++) {
            double den = c * fPow[i] / 5000.0;
            double vU = (S[i] - tr) / den;
            double vD = (S[i] + tr) / den;
            double v;
            if      (vU >= pv[i]) v = vU;
            else if (vD <  pv[i]) v = vD;
            else                  v = pv[i];
            if (v < 0) v = 0; if (v > 100) v = 100;
            contV[i] = v;
        }

        // ============================================================
        // [4단계] 스마트 정수 반올림 (분수 배낭 그리디)
        //
        // 연속 최적해는 실수이지만 실제로는 정수가 필요하다.
        //
        // 방법:
        //  (a) 모든 feature를 floor(연속해)로 시작 → 예산 여유 확보
        //  (b) 각 feature의 "ceil 후보"에 대해 효율을 계산:
        //      효율 = (만족도 이득 S[i]) / (추가 전력 extra)
        //  (c) 효율 내림차순으로 정렬
        //  (d) 예산 내에서 효율 높은 순으로 ceil 배정
        //
        // 이것은 0-1 배낭 문제의 그리디 근사 해법과 동일하다.
        // ============================================================
        int fv[MAX_FEATURES];
        for (int i = 0; i < MAX_FEATURES; i++) {
            fv[i] = (int)contV[i];  // floor
            if (fv[i] < 0) fv[i] = 0;
            if (fv[i] > 100) fv[i] = 100;
        }

        // ceil 후보 수집 및 효율순 정렬
        double cG[MAX_FEATURES], cE[MAX_FEATURES]; // cG: gain, cE: extra power
        int cO[MAX_FEATURES], nC = 0;               // cO: feature index
        for (int i = 0; i < MAX_FEATURES; i++) {
            int fl = fv[i], ce = fl + 1;
            if (ce > 100 || contV[i] < fl + 0.01) continue; // 연속해가 floor에 가까우면 skip
            double extra = fp(i, ce) - fp(i, fl);            // ceil 시 추가 전력
            double gain = S[i] - 0.01 * extra;               // net score 이득
            if (gain <= 0) continue;                          // 이득 없으면 skip
            cG[nC] = gain; cE[nC] = extra; cO[nC] = i; nC++;
        }
        // 효율 = gain/extra 내림차순 삽입정렬 (최대 20개라 충분)
        for (int a = 1; a < nC; a++) {
            double g = cG[a], e = cE[a]; int o = cO[a];
            int b = a - 1;
            while (b >= 0 && cG[b] * e < g * cE[b]) { // cG[b]/cE[b] < g/e → 교환
                cG[b+1] = cG[b]; cE[b+1] = cE[b]; cO[b+1] = cO[b]; b--;
            }
            cG[b+1] = g; cE[b+1] = e; cO[b+1] = o;
        }
        // 효율 높은 순으로 예산 내에서 ceil 배정
        double curPow = tp(fv);
        for (int k = 0; k < nC; k++) {
            int i = cO[k];
            double extra = fp(i, fv[i] + 1) - fp(i, fv[i]);
            if (curPow + extra <= SAFE_BUDGET) {
                curPow += extra; fv[i]++;
            }
        }

        // ============================================================
        // [5단계] 예산 초과 시 안전 감소
        //
        // 부동소수점 누적 오차로 예산을 넘을 수 있으므로,
        // "만족도/전력" 효율이 가장 낮은 feature부터 -1씩 감소시킨다.
        // ============================================================
        while (tp(fv) > SAFE_BUDGET) {
            int best = -1; double bestR = 1e18;
            for (int i = 0; i < MAX_FEATURES; i++) {
                if (fv[i] <= 0) continue;
                double save = fp(i, fv[i]) - fp(i, fv[i] - 1); // -1 시 절약 전력
                if (save <= 0) { best = i; break; }             // 전력이 안 줄면 우선 처리
                double r = S[i] / save;                          // 효율 = 만족도 손실 / 전력 절약
                if (r < bestR) { bestR = r; best = i; }          // 효율 최저 선택
            }
            if (best < 0) break;
            fv[best]--;
        }

        // ============================================================
        // [6단계] Greedy +1: 남은 예산으로 최고 효율 feature 증가
        //
        // 매 반복마다 "net score gain = S[i] - 0.01 × 추가전력"이
        // 가장 큰 feature를 +1 한다. 더 이상 예산 내에서 증가 불가능하면 종료.
        // ============================================================
        for (;;) {
            double cp = tp(fv);
            int best = -1; double bestG = 0;
            for (int i = 0; i < MAX_FEATURES; i++) {
                if (fv[i] >= 100) continue;
                double extra = fp(i, fv[i] + 1) - fp(i, fv[i]);
                if (cp + extra > SAFE_BUDGET) continue;
                double g = S[i] - 0.01 * extra;  // 실제 점수 기여분
                if (g > bestG) { bestG = g; best = i; }
            }
            if (best < 0) break;
            fv[best]++;
            printf("[6단계] 증가");
        }

        // ============================================================
        // [7단계] 1-for-1 Swap: 비효율 feature -1 ↔ 고효율 feature +1
        //
        // 정수 반올림 때문에 최적이 아닐 수 있다.
        // 모든 (dec, inc) 쌍을 탐색하여:
        //   net score 변화 = (S[inc] - S[dec]) - 0.01 × (추가전력 - 절약전력)
        // 가 양수이면 교환. 개선이 없을 때까지 반복.
        // 20×20 = 400쌍이므로 충분히 빠르다.
        // ============================================================
        for (;;) {
            double cp = tp(fv);
            int bestD = -1, bestI = -1;
            double bestImp = 1e-9; // 개선 임계값 (노이즈 방지)

            for (int d = 0; d < MAX_FEATURES; d++) {
                if (fv[d] <= 0) continue;
                double dSave = fp(d, fv[d]) - fp(d, fv[d] - 1); // dec 시 절약 전력
                double dLoss = S[d];                              // dec 시 만족도 손실

                for (int g = 0; g < MAX_FEATURES; g++) {
                    if (g == d || fv[g] >= 100) continue;
                    double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]); // inc 시 추가 전력
                    if (cp - dSave + gCost > SAFE_BUDGET) continue;  // 예산 확인

                    // net score 개선량
                    double imp = (S[g] - dLoss) - 0.01 * (gCost - dSave);
                    if (imp > bestImp) {
                        bestImp = imp; bestD = d; bestI = g;
                    }
                }
            }
            if (bestD < 0) break;
            fv[bestD]--; fv[bestI]++;
            printf("[7단계] 1감소 1증가");
        }

        // ============================================================
        // [8단계] Swap 후 다시 Greedy +1
        //
        // Swap으로 전력이 남을 수 있으므로 빈틈없이 채운다.
        // ============================================================
        for (;;) {
            double cp = tp(fv);
            int best = -1; double bestG = 0;
            for (int i = 0; i < MAX_FEATURES; i++) {
                if (fv[i] >= 100) continue;
                double extra = fp(i, fv[i] + 1) - fp(i, fv[i]);
                if (cp + extra > SAFE_BUDGET) continue;
                double g = S[i] - 0.01 * extra;
                if (g > bestG) { bestG = g; best = i; }
            }
            if (best < 0) break;
            fv[best]++;
            printf("[8단계] 증가");
        }

        // ============================================================
        // [9단계] 2-for-1 Swap: 비효율 feature -2, 고효율 feature +1
        //
        // 1:1 Swap으로 못 잡는 경우를 처리한다.
        // 예: feature A가 전력을 많이 먹지만 S가 낮은 경우,
        //     A를 2 줄여서 확보한 전력으로 B를 1 올리는 게 이득일 수 있다.
        //
        // net score 변화 = S[inc] - 2×S[dec] - 0.01 × (추가전력 - 절약전력)
        // ============================================================
        for (;;) {
            double cp = tp(fv);
            int bestD = -1, bestI = -1;
            double bestImp = 1e-9;

            for (int d = 0; d < MAX_FEATURES; d++) {
                if (fv[d] <= 1) continue;
                double dSave = fp(d, fv[d]) - fp(d, fv[d] - 2); // -2 시 절약 전력
                double dLoss = S[d] * 2;                          // -2 시 만족도 손실

                for (int g = 0; g < MAX_FEATURES; g++) {
                    if (g == d || fv[g] >= 100) continue;
                    double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]);
                    if (cp - dSave + gCost > SAFE_BUDGET) continue;

                    double imp = (S[g] - dLoss) - 0.01 * (gCost - dSave);
                    if (imp > bestImp) {
                        bestImp = imp; bestD = d; bestI = g;
                    }
                }
            }
            if (bestD < 0) break;
            fv[bestD] -= 2; fv[bestI]++;
            printf("[9단계] 2감소 1증가");
        }

        // ============================================================
        // [10단계] 1-for-2 Swap: 비효율 feature -1, 고효율 feature +2
        //
        // 반대 방향의 비대칭 교환.
        // 예: feature A에서 1만 빼도 B를 2 올릴 수 있는 경우.
        //     (A의 전력계수가 크고, B의 전력계수가 작을 때)
        //
        // net score 변화 = 2×S[inc] - S[dec] - 0.01 × (추가전력 - 절약전력)
        // ============================================================
        for (;;) {
            double cp = tp(fv);
            int bestD = -1, bestI = -1;
            double bestImp = 1e-9;

            for (int d = 0; d < MAX_FEATURES; d++) {
                if (fv[d] <= 0) continue;
                double dSave = fp(d, fv[d]) - fp(d, fv[d] - 1);
                double dLoss = S[d];

                for (int g = 0; g < MAX_FEATURES; g++) {
                    if (g == d || fv[g] >= 99) continue;
                    double gCost = fp(g, fv[g] + 2) - fp(g, fv[g]); // +2 시 추가 전력
                    if (cp - dSave + gCost > SAFE_BUDGET) continue;

                    double imp = (S[g] * 2 - dLoss) - 0.01 * (gCost - dSave);
                    if (imp > bestImp) {
                        bestImp = imp; bestD = d; bestI = g;
                    }
                }
            }
            if (bestD < 0) break;
            fv[bestD]--; fv[bestI] += 2;
            printf("[10단계] 1감소 2 증가");
        }

        // ============================================================
        // [11단계] 최종 Greedy +1
        //
        // 모든 Swap이 끝난 후 마지막으로 남은 예산을 빈틈없이 활용한다.
        // ============================================================
        for (;;) {
            double cp = tp(fv);
            int best = -1; double bestG = 0;
            for (int i = 0; i < MAX_FEATURES; i++) {
                if (fv[i] >= 100) continue;
                double extra = fp(i, fv[i] + 1) - fp(i, fv[i]);
                if (cp + extra > SAFE_BUDGET) continue;
                double g = S[i] - 0.01 * extra;
                if (g > bestG) { bestG = g; best = i; }
            }
            if (best < 0) break;
            fv[best]++;
            printf("[11단계] 증가");
        }

        // 결과 제출 및 이전 값 갱신
        screen_control(fv);
        for (int i = 0; i < MAX_FEATURES; i++) pv[i] = fv[i];
    }
}
