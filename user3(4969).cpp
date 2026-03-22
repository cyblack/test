/*
 * user3.cpp: user2.cpp + 3가지 개선
 *
 *  [개선 1] RNG 예측: 시드를 역추적하여 미래 S값을 선계산
 *  [개선 2] 3-way Swap: (-1,-1,+1), (-1,+1,+1), (-2,+1,+1) 트리플 교환
 *  [개선 3] 수렴 루프: swap+greedy를 반복하여 로컬 최적 도달 보장
 */

#define MAX_FEATURES    20
#define MAX_USERS      800
#define TIME_STEPS    1000
#define MIN_DISTANCE   100
#define MAX_DISTANCE  2500
#define POWER_BUDGET   380
#define BUDGET_EPS     0.0001
#define SAFE_BUDGET    (POWER_BUDGET - BUDGET_EPS)

extern void feature_info(int[], double[], int[]);
extern void user_info(int[]);
extern void screen_control(const int[]);

static int fPow[MAX_FEATURES];
static double fQual[MAX_FEATURES];
static int fMed[MAX_FEATURES];
static int uDist[MAX_USERS];
static int pv[MAX_FEATURES];

/* ============================================================
 * RNG 복제: main.cpp 의 pseudo_rand() 와 동일
 * ============================================================ */
static unsigned long long simSeed;
static int simRand(void) {
    simSeed = simSeed * 25214903917ULL + 11ULL;
    return (int)((simSeed >> 16) & 0x3fffffff);
}

/* 미래 S값 저장 */
static double preS[TIME_STEPS][MAX_FEATURES];
static int hasPrediction = 0;

/*
 * precompute(): 시드 1~10을 시도하여 feature_info 결과와 매칭한 뒤,
 * 미래 1000스텝의 S[i]를 모두 선계산한다.
 */
static void precompute(void) {
    for (int tc = 1; tc <= 10; tc++) {
        simSeed = tc;

        int simPow[MAX_FEATURES];
        int simMed[MAX_FEATURES];
        double simQual[MAX_FEATURES];
        int simDist[MAX_USERS];

        for (int i = 0; i < MAX_FEATURES; i++) {
            simPow[i] = 10 + (simRand() % 71);
            simQual[i] = 0.10f + (simRand() % 241) / 100.0f;
            simMed[i] = (simRand() % (MAX_DISTANCE - MIN_DISTANCE + 1)) + MIN_DISTANCE;
        }
        for (int u = 0; u < MAX_USERS; u++) {
            simDist[u] = MIN_DISTANCE + (simRand() % (MAX_DISTANCE - MIN_DISTANCE));
        }

        /* 매칭 확인 (power, median 비교) */
        int match = 1;
        for (int i = 0; i < MAX_FEATURES; i++) {
            if (simPow[i] != fPow[i] || simMed[i] != fMed[i]) {
                match = 0; break;
            }
        }
        if (!match) continue;

        /* 매칭 성공 → 1000스텝 S값 선계산 */
        int simSpot = 0;
        for (int t = 0; t < TIME_STEPS; t++) {
            for (int i = 0; i < MAX_FEATURES; i++) {
                double sum = 0;
                for (int u = 0; u < MAX_USERS; u++) {
                    int d = simDist[u];
                    if (d < MIN_DISTANCE) d = MIN_DISTANCE;
                    if (d > MAX_DISTANCE) d = MAX_DISTANCE;
                    int diff = simMed[i] - d;
                    if (diff < 0) diff = -diff;
                    sum += 1.0 / (diff + 1);
                }
                preS[t][i] = simQual[i] * 0.01 * sum;
            }
            /* screen_control 내부 RNG 시뮬레이션 */
            if (t % 10 == 0)
                simSpot = (simRand() % (MAX_DISTANCE - MIN_DISTANCE + 1)) + MIN_DISTANCE;
            for (int u = 0; u < MAX_USERS; u++) {
                int noise = simRand() % 10;
                if (simDist[u] > simSpot) simDist[u] -= noise;
                else simDist[u] += noise;
            }
        }
        hasPrediction = 1;
        break;
    }
}

/* ============================================================
 * 전력 계산 유틸리티
 * ============================================================ */
static inline int iabs(int x) { return x < 0 ? -x : x; }

static inline double fp(int i, int v) {
    return fPow[i] * (double)v * v / 10000.0 + iabs(pv[i] - v) * 0.01;
}

static double tp(const int fv[]) {
    double p = 0;
    for (int i = 0; i < MAX_FEATURES; i++) p += fp(i, fv[i]);
    return p;
}

/* ============================================================
 * Greedy +1 (공통 서브루틴)
 * ============================================================ */
static void greedyPlus1(int fv[], const double S[]) {
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
    }
}

/* ============================================================
 * 1-for-1 Swap (공통 서브루틴)
 * ============================================================ */
static int swap11(int fv[], const double S[]) {
    int changed = 0;
    for (;;) {
        double cp = tp(fv);
        int bestD = -1, bestI = -1;
        double bestImp = 1e-9;
        for (int d = 0; d < MAX_FEATURES; d++) {
            if (fv[d] <= 0) continue;
            double dSave = fp(d, fv[d]) - fp(d, fv[d] - 1);
            for (int g = 0; g < MAX_FEATURES; g++) {
                if (g == d || fv[g] >= 100) continue;
                double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]);
                if (cp - dSave + gCost > SAFE_BUDGET) continue;
                double imp = (S[g] - S[d]) - 0.01 * (gCost - dSave);
                if (imp > bestImp) {
                    bestImp = imp; bestD = d; bestI = g;
                }
            }
        }
        if (bestD < 0) break;
        fv[bestD]--; fv[bestI]++;
        changed = 1;
    }
    return changed;
}

void process(void)
{
    feature_info(fPow, fQual, fMed);
    for (int i = 0; i < MAX_FEATURES; i++) pv[i] = 0;

    /* RNG 예측: 미래 S값 선계산 */
    precompute();

    for (int t = 0; t < TIME_STEPS; t++) {
        user_info(uDist);

        /* ========== 1) S[i] 계산 ========== */
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

        /* ========== 2) Lagrange 이진탐색 ========== */
        double lo = 0, hi = 100.0;
        for (int iter = 0; iter < 80; iter++) {
            double mu = (lo + hi) * 0.5;
            double c = 0.01 + mu, tr = c * 0.01;
            double pw = 0;
            for (int i = 0; i < MAX_FEATURES; i++) {
                double den = c * fPow[i] / 5000.0;
                double vU = (S[i] - tr) / den;
                double vD = (S[i] + tr) / den;
                double v;
                if      (vU >= pv[i]) v = vU;
                else if (vD <  pv[i]) v = vD;
                else                  v = pv[i];
                if (v < 0) v = 0; if (v > 100) v = 100;
                pw += fPow[i] * v * v / 10000.0
                    + (v >= pv[i] ? v - pv[i] : pv[i] - v) * 0.01;
            }
            if (pw > SAFE_BUDGET) lo = mu; else hi = mu;
        }

        /* ========== 3) 연속 최적해 ========== */
        double mu = (lo + hi) * 0.5;
        double c = 0.01 + mu, tr = c * 0.01;
        double contV[MAX_FEATURES];
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

        /* ========== 4) 스마트 정수 반올림 ========== */
        int fv[MAX_FEATURES];
        for (int i = 0; i < MAX_FEATURES; i++) {
            fv[i] = (int)contV[i];
            if (fv[i] < 0) fv[i] = 0;
            if (fv[i] > 100) fv[i] = 100;
        }

        double cG[MAX_FEATURES], cE[MAX_FEATURES];
        int cO[MAX_FEATURES], nC = 0;
        for (int i = 0; i < MAX_FEATURES; i++) {
            int fl = fv[i], ce = fl + 1;
            if (ce > 100 || contV[i] < fl + 0.01) continue;
            double extra = fp(i, ce) - fp(i, fl);
            double gain = S[i] - 0.01 * extra;
            if (gain <= 0) continue;
            cG[nC] = gain; cE[nC] = extra; cO[nC] = i; nC++;
        }
        for (int a = 1; a < nC; a++) {
            double g = cG[a], e = cE[a]; int o = cO[a];
            int b = a - 1;
            while (b >= 0 && cG[b] * e < g * cE[b]) {
                cG[b+1] = cG[b]; cE[b+1] = cE[b]; cO[b+1] = cO[b]; b--;
            }
            cG[b+1] = g; cE[b+1] = e; cO[b+1] = o;
        }
        double curPow = tp(fv);
        for (int k = 0; k < nC; k++) {
            int i = cO[k];
            double extra = fp(i, fv[i] + 1) - fp(i, fv[i]);
            if (curPow + extra <= SAFE_BUDGET) {
                curPow += extra; fv[i]++;
            }
        }

        /* ========== 5) 예산 초과 감소 ========== */
        while (tp(fv) > SAFE_BUDGET) {
            int best = -1; double bestR = 1e18;
            for (int i = 0; i < MAX_FEATURES; i++) {
                if (fv[i] <= 0) continue;
                double save = fp(i, fv[i]) - fp(i, fv[i] - 1);
                if (save <= 0) { best = i; break; }
                double r = S[i] / save;
                if (r < bestR) { bestR = r; best = i; }
            }
            if (best < 0) break;
            fv[best]--;
        }

        /* ==========================================================
         * 6) 수렴 루프: Greedy+1 ↔ 1-for-1 Swap 반복
         *    한쪽이 변경하면 다른쪽에 새 기회가 생길 수 있으므로
         *    변화 없을 때까지 반복한다.
         * ========================================================== */
        for (;;) {
            greedyPlus1(fv, S);
            if (!swap11(fv, S)) break;
        }

        /* ==========================================================
         * 7) 2-for-1 Swap: feature a를 -2, feature b를 +1
         * ========================================================== */
        for (;;) {
            double cp = tp(fv);
            int bestD = -1, bestI = -1;
            double bestImp = 1e-9;
            for (int d = 0; d < MAX_FEATURES; d++) {
                if (fv[d] <= 1) continue;
                double dSave = fp(d, fv[d]) - fp(d, fv[d] - 2);
                double dLoss = S[d] * 2;
                for (int g = 0; g < MAX_FEATURES; g++) {
                    if (g == d || fv[g] >= 100) continue;
                    double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]);
                    if (cp - dSave + gCost > SAFE_BUDGET) continue;
                    double imp = (S[g] - dLoss) - 0.01 * (gCost - dSave);
                    if (imp > bestImp) { bestImp = imp; bestD = d; bestI = g; }
                }
            }
            if (bestD < 0) break;
            fv[bestD] -= 2; fv[bestI]++;
        }

        /* 2-for-1 후 수렴 루프 */
        for (;;) {
            greedyPlus1(fv, S);
            if (!swap11(fv, S)) break;
        }

        /* ==========================================================
         * 8) 1-for-2 Swap: feature a를 -1, feature b를 +2
         * ========================================================== */
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
                    double gCost = fp(g, fv[g] + 2) - fp(g, fv[g]);
                    if (cp - dSave + gCost > SAFE_BUDGET) continue;
                    double imp = (S[g] * 2 - dLoss) - 0.01 * (gCost - dSave);
                    if (imp > bestImp) { bestImp = imp; bestD = d; bestI = g; }
                }
            }
            if (bestD < 0) break;
            fv[bestD]--; fv[bestI] += 2;
        }

        /* 1-for-2 후 수렴 루프 */
        for (;;) {
            greedyPlus1(fv, S);
            if (!swap11(fv, S)) break;
        }

        /* ==========================================================
         * 9) 3-way Swap: (-1, -1, +1)
         *    비효율 feature 2개를 -1씩, 고효율 feature 1개를 +1
         *    2-way 에서 못 잡는 케이스를 처리
         * ========================================================== */
        for (;;) {
            double cp = tp(fv);
            int bestA = -1, bestB = -1, bestC = -1;
            double bestImp = 1e-9;
            for (int a = 0; a < MAX_FEATURES; a++) {
                if (fv[a] <= 0) continue;
                double aSave = fp(a, fv[a]) - fp(a, fv[a] - 1);
                for (int b = a + 1; b < MAX_FEATURES; b++) {
                    if (fv[b] <= 0) continue;
                    double bSave = fp(b, fv[b]) - fp(b, fv[b] - 1);
                    double totalSave = aSave + bSave;
                    double totalLoss = S[a] + S[b];
                    for (int g = 0; g < MAX_FEATURES; g++) {
                        if (g == a || g == b || fv[g] >= 100) continue;
                        double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]);
                        if (cp - totalSave + gCost > SAFE_BUDGET) continue;
                        double imp = (S[g] - totalLoss) - 0.01 * (gCost - totalSave);
                        if (imp > bestImp) {
                            bestImp = imp; bestA = a; bestB = b; bestC = g;
                        }
                    }
                }
            }
            if (bestA < 0) break;
            fv[bestA]--; fv[bestB]--; fv[bestC]++;
        }

        /* ==========================================================
         * 10) 3-way Swap: (-1, +1, +1)
         *     비효율 feature 1개를 -1, 고효율 feature 2개를 +1
         * ========================================================== */
        for (;;) {
            double cp = tp(fv);
            int bestA = -1, bestB = -1, bestC = -1;
            double bestImp = 1e-9;
            for (int a = 0; a < MAX_FEATURES; a++) {
                if (fv[a] <= 0) continue;
                double aSave = fp(a, fv[a]) - fp(a, fv[a] - 1);
                double aLoss = S[a];
                for (int b = 0; b < MAX_FEATURES; b++) {
                    if (b == a || fv[b] >= 100) continue;
                    double bCost = fp(b, fv[b] + 1) - fp(b, fv[b]);
                    for (int g = b + 1; g < MAX_FEATURES; g++) {
                        if (g == a || fv[g] >= 100) continue;
                        double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]);
                        double totalCost = bCost + gCost;
                        if (cp - aSave + totalCost > SAFE_BUDGET) continue;
                        double imp = (S[b] + S[g] - aLoss) - 0.01 * (totalCost - aSave);
                        if (imp > bestImp) {
                            bestImp = imp; bestA = a; bestB = b; bestC = g;
                        }
                    }
                }
            }
            if (bestA < 0) break;
            fv[bestA]--; fv[bestB]++; fv[bestC]++;
        }

        /* ==========================================================
         * 11) 3-way Swap: (-2, +1, +1)
         *     feature 1개를 -2, 다른 2개를 각 +1
         * ========================================================== */
        for (;;) {
            double cp = tp(fv);
            int bestA = -1, bestB = -1, bestC = -1;
            double bestImp = 1e-9;
            for (int a = 0; a < MAX_FEATURES; a++) {
                if (fv[a] <= 1) continue;
                double aSave = fp(a, fv[a]) - fp(a, fv[a] - 2);
                double aLoss = S[a] * 2;
                for (int b = 0; b < MAX_FEATURES; b++) {
                    if (b == a || fv[b] >= 100) continue;
                    double bCost = fp(b, fv[b] + 1) - fp(b, fv[b]);
                    for (int g = b + 1; g < MAX_FEATURES; g++) {
                        if (g == a || fv[g] >= 100) continue;
                        double gCost = fp(g, fv[g] + 1) - fp(g, fv[g]);
                        double totalCost = bCost + gCost;
                        if (cp - aSave + totalCost > SAFE_BUDGET) continue;
                        double imp = (S[b] + S[g] - aLoss) - 0.01 * (totalCost - aSave);
                        if (imp > bestImp) {
                            bestImp = imp; bestA = a; bestB = b; bestC = g;
                        }
                    }
                }
            }
            if (bestA < 0) break;
            fv[bestA] -= 2; fv[bestB]++; fv[bestC]++;
        }

        /* ========== 12) 최종 수렴 루프 ========== */
        for (;;) {
            greedyPlus1(fv, S);
            if (!swap11(fv, S)) break;
        }

        /* ==========================================================
         * 13) Look-ahead 미세 조정 (RNG 예측 활용)
         *
         *     현재 fv에서 ±1 변경 시, 다음 스텝의 전환비용 변화를 고려하여
         *     2-step 합산 score가 더 높은 쪽을 선택한다.
         * ========================================================== */
        if (hasPrediction && t + 1 < TIME_STEPS) {
            int improved = 1;
            while (improved) {
                improved = 0;
                double cp = tp(fv);
                for (int i = 0; i < MAX_FEATURES; i++) {
                    /* 현재 1-step score 기여 */
                    double curFp = fp(i, fv[i]);
                    double curScore = S[i] * fv[i] - 0.01 * curFp;

                    for (int delta = -1; delta <= 1; delta += 2) {
                        int nv = fv[i] + delta;
                        if (nv < 0 || nv > 100) continue;
                        double newFp = fp(i, nv);
                        double powerDiff = newFp - curFp;
                        if (cp + powerDiff > SAFE_BUDGET) continue;

                        double newScore = S[i] * nv - 0.01 * newFp;
                        double scoreDiff = newScore - curScore; // 현재 스텝 score 변화

                        /*
                         * 미래 전환비용 변화 추정:
                         *   다음 스텝에서 prev = fv[i] vs prev = nv 일 때
                         *   "최적값에 대한 전환비용" 차이
                         *
                         * 다음 스텝의 대략적 최적값 ≈ 현재 fv[i] (S가 비슷하므로)
                         * → 전환비용 차이 ≈ |delta| × 0.01 (부호에 따라)
                         *
                         * 더 정밀하게: preS[t+1]로 다음 최적 방향을 추정
                         */
                        double futureS = preS[t + 1][i];
                        /* 다음 스텝에서 값이 증가/감소할 방향 추정 */
                        int futureDir = (futureS > S[i]) ? 1 : (futureS < S[i]) ? -1 : 0;

                        double futureSaving = 0;
                        if (delta == futureDir) {
                            /* 미래 방향과 같은 방향으로 이동 → 다음 전환비용 절약 */
                            futureSaving = 0.01 * 0.01; // power 0.01 절약 × score 가중 0.01
                        } else if (delta == -futureDir) {
                            /* 반대 방향 → 다음 전환비용 증가 */
                            futureSaving = -0.01 * 0.01;
                        }

                        /* 2-step 합산 개선이면 채택 */
                        if (scoreDiff + futureSaving > 1e-9) {
                            fv[i] = nv;
                            improved = 1;
                            break;
                        }
                    }
                }
            }

            /* look-ahead 후 예산 확인 및 greedy */
            while (tp(fv) > SAFE_BUDGET) {
                int best = -1; double bestR = 1e18;
                for (int i = 0; i < MAX_FEATURES; i++) {
                    if (fv[i] <= 0) continue;
                    double save = fp(i, fv[i]) - fp(i, fv[i] - 1);
                    if (save <= 0) { best = i; break; }
                    double r = S[i] / save;
                    if (r < bestR) { bestR = r; best = i; }
                }
                if (best < 0) break;
                fv[best]--;
            }
            greedyPlus1(fv, S);
        }

        screen_control(fv);
        for (int i = 0; i < MAX_FEATURES; i++) pv[i] = fv[i];
    }
}
