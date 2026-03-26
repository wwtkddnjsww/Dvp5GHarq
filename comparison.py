import re
from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ================== 설정 ==================
ROOT = Path("results")
Deadline = 3
SIM_NRB_NAME = f"{Deadline}NRBListSimulation.csv"
SIM_DVP_NAME = f"{Deadline}DVPListSimulation.csv"
THY_NRB_NAME = f"{Deadline}NRBListTheory.csv"
THY_DVP_NAME = f"{Deadline}DVPListTheory.csv"

OUT_PNG = f"DVP_vs_NRB_Sim_vs_Theory_{Deadline}ms.png"
OUT_SUMMARY = ROOT / "compare_summary.csv"

Y_LOG = True
YMIN, YMAX = 1e-5, 1.0
EPS = 1e-12
# =========================================


def read_vector_csv(p: Path) -> np.ndarray:
    """
    - 1열(세로) 또는 1행(가로)로 저장된 벡터를 모두 읽음
    - CSV 전체를 flatten 해서 숫자만 추출
    """
    df = pd.read_csv(p, header=None)

    # 전체 셀을 1차원으로 펼친 뒤 숫자로 변환
    vals = pd.to_numeric(df.to_numpy().ravel(), errors="coerce")
    vals = vals[~np.isnan(vals)]
    return vals.astype(float)

def read_1col_csv(p: Path) -> np.ndarray:
    df = pd.read_csv(p, header=None)
    s = pd.to_numeric(df.iloc[:, 0], errors="coerce").dropna()
    return s.to_numpy(dtype=float)


def parse_combo(folder_name: str):
    ms = re.search(r"sinr(-?\d+)", folder_name)
    mp = re.search(r"par(\d+)", folder_name)
    mr = re.search(r"srs(\d+)", folder_name)
    sinr = int(ms.group(1)) if ms else None
    par = int(mp.group(1)) if mp else None
    srs = int(mr.group(1)) if mr else None
    return sinr, par, srs


def sort_xy(x, y):
    idx = np.argsort(x)
    return x[idx], y[idx]


def interp_to(x_src, y_src, x_tgt):
    x_src, y_src = sort_xy(np.asarray(x_src), np.asarray(y_src))
    x_tgt = np.asarray(x_tgt)
    return np.interp(x_tgt, x_src, y_src, left=np.nan, right=np.nan)


def safe_log_clip(y):
    y = np.asarray(y, dtype=float)
    y = np.where(np.isfinite(y), y, np.nan)
    return np.clip(y, EPS, None)


def main():
    combo_dirs = sorted([p for p in ROOT.glob("sinr*_par*_srs*") if p.is_dir()])
    if not combo_dirs:
        print(f"[WARN] {ROOT} 아래에 sinr*_par*_srs* 폴더를 못 찾았어요.")
        return

    summary_rows = []
    made = 0

    for d in combo_dirs:
        sim_nrb_p = d / SIM_NRB_NAME
        sim_dvp_p = d / SIM_DVP_NAME
        thy_nrb_p = d / THY_NRB_NAME
        thy_dvp_p = d / THY_DVP_NAME

        # ✅ fallback 없이: 하나라도 없으면 스킵
        missing = []
        if not sim_nrb_p.exists(): missing.append(SIM_NRB_NAME)
        if not sim_dvp_p.exists(): missing.append(SIM_DVP_NAME)
        if not thy_nrb_p.exists(): missing.append(THY_NRB_NAME)
        if not thy_dvp_p.exists(): missing.append(THY_DVP_NAME)

        if missing:
            print(f"[SKIP] {d.name} -> missing: {', '.join(missing)}")
            continue

        try:
            x_sim = read_vector_csv(sim_nrb_p)
            y_sim = read_vector_csv(sim_dvp_p)
            x_thy = read_vector_csv(thy_nrb_p)
            y_thy = read_vector_csv(thy_dvp_p)
        except Exception as e:
            print(f"[SKIP] {d.name} -> CSV read error: {e}")
            continue

        n_sim = min(len(x_sim), len(y_sim))
        n_thy = min(len(x_thy), len(y_thy))
        x_sim, y_sim = x_sim[:n_sim], y_sim[:n_sim]
        x_thy, y_thy = x_thy[:n_thy], y_thy[:n_thy]

        if n_sim == 0 or n_thy == 0:
            print(f"[SKIP] {d.name} -> empty data (sim={n_sim}, thy={n_thy})")
            continue

        x_sim, y_sim = sort_xy(x_sim, y_sim)
        x_thy, y_thy = sort_xy(x_thy, y_thy)

        y_sim_c = safe_log_clip(y_sim) if Y_LOG else y_sim
        y_thy_c = safe_log_clip(y_thy) if Y_LOG else y_thy

        # 오차: theory를 sim x로 보간해 비교
        y_thy_on_simx = interp_to(x_thy, y_thy, x_sim)
        if Y_LOG:
            y_thy_on_simx = safe_log_clip(y_thy_on_simx)

        valid = np.isfinite(y_thy_on_simx) & np.isfinite(y_sim_c)
        if valid.any():
            diff = y_sim_c[valid] - y_thy_on_simx[valid]
            mae = float(np.mean(np.abs(diff)))
            rmse = float(np.sqrt(np.mean(diff ** 2)))
            rel = np.abs(diff) / np.maximum(np.abs(y_thy_on_simx[valid]), EPS)
            mape = float(np.mean(rel))
        else:
            mae = rmse = mape = float("nan")

        sinr, par, srs = parse_combo(d.name)
        summary_rows.append({
            "folder": d.name,
            "sinr": sinr,
            "par": par,
            "srs": srs,
            "n_sim": n_sim,
            "n_thy": n_thy,
            "mae": mae,
            "rmse": rmse,
            "mape": mape,
        })

        # 플롯 저장
        plt.figure()
        plt.plot(x_sim, y_sim_c, marker="o", linestyle="-", label="Simulation")
        plt.plot(x_thy, y_thy_c, marker="x", linestyle="--", label="Theory")

        plt.xlabel("NRB (PRB per slot)")
        plt.ylabel("DVP" + (" (log)" if Y_LOG else ""))
        plt.title(f"{d.name} | MAE={mae:.3e}, RMSE={rmse:.3e}, MAPE={mape:.3e}")
        if Y_LOG:
            plt.yscale("log")
            plt.ylim(YMIN, YMAX)
        plt.grid(True, which="both")
        plt.legend()
        plt.tight_layout()

        out_png = d / OUT_PNG
        plt.savefig(out_png, dpi=200)
        plt.close()

        made += 1
        print(f"[OK] {d.name} -> saved plot: {out_png}")

    if summary_rows:
        pd.DataFrame(summary_rows).to_csv(OUT_SUMMARY, index=False)
        print(f"[OK] saved summary -> {OUT_SUMMARY}")
    else:
        print("[WARN] 비교 결과가 없습니다. (필요 CSV가 폴더에 있는지 확인)")

    print(f"Done. Compared {made} folders.")


if __name__ == "__main__":
    main()