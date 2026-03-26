import re
from pathlib import Path
from collections import defaultdict

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ================== 설정 ==================
SIM_ROOT = Path("result-tmp")           # simulation 결과 루트
THY_ROOT = Path("theoryresults_tmp")    # theory 결과 루트

# ✅ 여러 deadline(ms)을 한 번에
DEADLINE_MS_LIST = [2, 3, 5, 8, 10]  # 원하는 값들로 변경

plt.style.use("default")
plt.rcParams["figure.figsize"] = (4, 3)
plt.rcParams["mathtext.fontset"] = "stix"
plt.rcParams["font.family"] = "Times New Roman"

# y축 로그
Y_LOG = True
YMIN, YMAX = 1e-5, 1.0
EPS = 1e-12
# =========================================


def read_vector_csv(p: Path) -> np.ndarray:
    """1행/1열 어떤 형태든 숫자만 flatten해서 읽기"""
    df = pd.read_csv(p, header=None)
    vals = pd.to_numeric(df.to_numpy().ravel(), errors="coerce")
    vals = vals[~np.isnan(vals)]
    return vals.astype(float)


def parse_combo(folder_name: str):
    """
    폴더명: sinr*_par*_srs*
      - sinr: SINR(dB)
      - par : packet arrival rate
      - srs : SRS
    """
    ms = re.search(r"sinr(-?\d+)", folder_name)
    mp = re.search(r"par(\d+)", folder_name)
    mr = re.search(r"srs(\d+)", folder_name)
    sinr = int(ms.group(1)) if ms else None
    arrival = int(mp.group(1)) if mp else None
    srs = int(mr.group(1)) if mr else None
    return sinr, arrival, srs


def sort_xy(x, y):
    idx = np.argsort(x)
    return x[idx], y[idx]


def safe_log_clip(y):
    y = np.asarray(y, dtype=float)
    y = np.where(np.isfinite(y), y, np.nan)
    return np.clip(y, EPS, None)


def names_for_deadline(deadline_ms: int):
    sim_nrb = f"{deadline_ms}NRBListSimulation.csv"
    sim_dvp = f"{deadline_ms}DVPListSimulation.csv"
    thy_nrb = f"{deadline_ms}NRBListTheory.csv"
    thy_dvp = f"{deadline_ms}DVPListTheory.csv"
    out_dir = SIM_ROOT / f"compare_plots_{deadline_ms}ms"
    return sim_nrb, sim_dvp, thy_nrb, thy_dvp, out_dir


def load_folder_series(sim_dir: Path, thy_dir: Path, deadline_ms: int):
    """
    deadline_ms에 해당하는 sim/theory 곡선 로드.
    하나라도 없으면 None
    """
    SIM_NRB_NAME, SIM_DVP_NAME, THY_NRB_NAME, THY_DVP_NAME, _ = names_for_deadline(deadline_ms)

    sim_nrb_p = sim_dir / SIM_NRB_NAME
    sim_dvp_p = sim_dir / SIM_DVP_NAME
    thy_nrb_p = thy_dir / THY_NRB_NAME
    thy_dvp_p = thy_dir / THY_DVP_NAME

    missing = []
    if not sim_nrb_p.exists(): missing.append(f"SIM:{SIM_NRB_NAME}")
    if not sim_dvp_p.exists(): missing.append(f"SIM:{SIM_DVP_NAME}")
    if not thy_nrb_p.exists(): missing.append(f"THY:{THY_NRB_NAME}")
    if not thy_dvp_p.exists(): missing.append(f"THY:{THY_DVP_NAME}")

    if missing:
        return None, f"[SKIP] {sim_dir.name} (D={deadline_ms}ms) -> missing: {', '.join(missing)}"

    try:
        x_sim = read_vector_csv(sim_nrb_p)
        y_sim = read_vector_csv(sim_dvp_p)
        x_thy = read_vector_csv(thy_nrb_p)
        y_thy = read_vector_csv(thy_dvp_p)
    except Exception as e:
        return None, f"[SKIP] {sim_dir.name} (D={deadline_ms}ms) -> CSV read error: {e}"

    n_sim = min(len(x_sim), len(y_sim))
    n_thy = min(len(x_thy), len(y_thy))
    x_sim, y_sim = x_sim[:n_sim], y_sim[:n_sim]
    x_thy, y_thy = x_thy[:n_thy], y_thy[:n_thy]

    if n_sim == 0 or n_thy == 0:
        return None, f"[SKIP] {sim_dir.name} (D={deadline_ms}ms) -> empty data (sim={n_sim}, thy={n_thy})"

    x_sim, y_sim = sort_xy(x_sim, y_sim)
    x_thy, y_thy = sort_xy(x_thy, y_thy)

    if Y_LOG:
        y_sim = safe_log_clip(y_sim)
        y_thy = safe_log_clip(y_thy)

    sinr, arrival, srs = parse_combo(sim_dir.name)
    if sinr is None or arrival is None or srs is None:
        return None, f"[SKIP] {sim_dir.name} (D={deadline_ms}ms) -> cannot parse sinr/par(arrival)/srs"

    return {
        "folder": sim_dir.name,
        "sinr": sinr,
        "arrival": arrival,
        "srs": srs,
        "x_sim": x_sim,
        "y_sim": y_sim,
        "x_thy": x_thy,
        "y_thy": y_thy,
        "deadline_ms": deadline_ms,
    }, None


def plot_sweep(records, title, out_path: Path, color_key, label_key, sort_key=None):
    """
    같은 조건(theory/sim)은 색 동일.
    - simulation: solid
    - theory: dashed
    """
    if not records:
        return

    uniq = sorted({r[color_key] for r in records})
    color_map = {v: i for i, v in enumerate(uniq)}

    if sort_key is not None:
        records = sorted(records, key=lambda r: r[sort_key])

    prop_cycle = plt.rcParams["axes.prop_cycle"].by_key().get("color", [])
    if not prop_cycle:
        prop_cycle = [None]

    plt.figure()

    for r in records:
        ck = r[color_key]
        color = prop_cycle[color_map[ck] % len(prop_cycle)]

        if label_key == "sinr":
            labkey_str = r"$\gamma$"
        elif label_key == "arrival":
            labkey_str = r"$f$"
        elif label_key == "srs":
            labkey_str = r"$\xi$"
        else:
            labkey_str = label_key

        lab = f"{labkey_str}={r[label_key]}"

        plt.plot(r["x_sim"], r["y_sim"], linestyle="-", marker="o",
                 color=color, label=f"{lab} (Sim)")
        plt.plot(r["x_thy"], r["y_thy"], linestyle="--", marker="x",
                 color=color, label=f"{lab} (Theory)")

    plt.xlabel("PRB per slot")
    plt.ylabel("Delay Violation Probability")
    if Y_LOG:
        plt.yscale("log")
        plt.ylim(YMIN, YMAX)
    plt.grid(True, which="both", linestyle="--", linewidth=0.5)
    plt.legend(fontsize=8, loc="best", fancybox=False, edgecolor="black", ncol=2)
    plt.tight_layout()
    plt.savefig(out_path, dpi=200)
    plt.close()
    print(f"[OK] saved -> {out_path}")


def run_for_deadline(deadline_ms: int):
    SIM_NRB_NAME, SIM_DVP_NAME, THY_NRB_NAME, THY_DVP_NAME, OUT_DIR = names_for_deadline(deadline_ms)
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # sim 기준으로 combo 폴더를 찾고, theory에도 같은 폴더가 있는지 확인
    sim_combo_dirs = sorted([p for p in SIM_ROOT.glob("sinr*_par*_srs*") if p.is_dir()])
    if not sim_combo_dirs:
        print(f"[WARN] {SIM_ROOT} 아래에 sinr*_par*_srs* 폴더를 못 찾았어요.")
        return

    records = []
    for sim_d in sim_combo_dirs:
        thy_d = THY_ROOT / sim_d.name
        if not thy_d.is_dir():
            print(f"[SKIP] {sim_d.name} (D={deadline_ms}ms) -> theory folder missing: {thy_d}")
            continue

        rec, msg = load_folder_series(sim_d, thy_d, deadline_ms)
        if msg:
            print(msg)
        if rec:
            records.append(rec)

    if not records:
        print(f"[WARN] (D={deadline_ms}ms) 유효한 폴더 데이터가 없습니다.")
        return

    # 1) arrival, srs 고정 -> sinr sweep
    grp_sinr = defaultdict(list)
    for r in records:
        grp_sinr[(r["arrival"], r["srs"])].append(r)

    for (arrival, srs), rs in grp_sinr.items():
        title = f"Effect of SINR (arrival={arrival}, srs={srs}) | Deadline={deadline_ms}ms"
        out = OUT_DIR / f"sweep_sinr_arr{arrival}_srs{srs}_{deadline_ms}ms.png"
        plot_sweep(rs, title, out_path=out, color_key="sinr", label_key="sinr", sort_key="sinr")

    # 2) sinr, arrival 고정 -> srs sweep
    grp_srs = defaultdict(list)
    for r in records:
        grp_srs[(r["sinr"], r["arrival"])].append(r)

    for (sinr, arrival), rs in grp_srs.items():
        title = f"Effect of SRS (sinr={sinr}, arrival={arrival}) | Deadline={deadline_ms}ms"
        out = OUT_DIR / f"sweep_srs_sinr{sinr}_arr{arrival}_{deadline_ms}ms.png"
        plot_sweep(rs, title, out_path=out, color_key="srs", label_key="srs", sort_key="srs")

    # 3) sinr, srs 고정 -> arrival sweep
    grp_arr = defaultdict(list)
    for r in records:
        grp_arr[(r["sinr"], r["srs"])].append(r)

    for (sinr, srs), rs in grp_arr.items():
        title = f"Effect of Arrival Rate (sinr={sinr}, srs={srs}) | Deadline={deadline_ms}ms"
        out = OUT_DIR / f"sweep_arr_sinr{sinr}_srs{srs}_{deadline_ms}ms.png"
        plot_sweep(rs, title, out_path=out, color_key="arrival", label_key="arrival", sort_key="arrival")

    print(f"Done. (D={deadline_ms}ms) Plots saved under: {OUT_DIR}")


def main():
    for dms in DEADLINE_MS_LIST:
        run_for_deadline(dms)


if __name__ == "__main__":
    main()