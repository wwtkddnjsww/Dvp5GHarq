import re
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ================== 설정 ==================
SIM_ROOT = Path("result-tmp")
THY_ROOT = Path("theoryresults_tmp")
TESTBED_ROOT = Path("testbedresults")

TARGET_COMBO = "sinr10_par3_srs80"
DEADLINE_MS_LIST = [2, 3, 5, 8, 10]

OUT_DIR = Path("compare_theory_sim_testbed")
OUT_DIR.mkdir(parents=True, exist_ok=True)

plt.style.use("default")
plt.rcParams["figure.figsize"] = (4, 3)
plt.rcParams["mathtext.fontset"] = "stix"
plt.rcParams["font.family"] = "Times New Roman"

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
    tb_nrb = f"{deadline_ms}NRBListTheory.csv"
    tb_dvp = f"{deadline_ms}DVPListTheory.csv"
    return sim_nrb, sim_dvp, thy_nrb, thy_dvp, tb_nrb, tb_dvp


def load_xy_pair(x_path: Path, y_path: Path, tag: str):
    if not x_path.exists():
        return None, f"[SKIP] missing {tag} x-file: {x_path}"
    if not y_path.exists():
        return None, f"[SKIP] missing {tag} y-file: {y_path}"

    try:
        x = read_vector_csv(x_path)
        y = read_vector_csv(y_path)
    except Exception as e:
        return None, f"[SKIP] CSV read error ({tag}): {e}"

    n = min(len(x), len(y))
    if n == 0:
        return None, f"[SKIP] empty data ({tag})"

    x = x[:n]
    y = y[:n]
    x, y = sort_xy(x, y)

    if Y_LOG:
        y = safe_log_clip(y)

    return {"x": x, "y": y}, None


def load_deadline_series(deadline_ms: int):
    sim_nrb, sim_dvp, thy_nrb, thy_dvp, tb_nrb, tb_dvp = names_for_deadline(deadline_ms)

    sim_dir = SIM_ROOT / TARGET_COMBO
    thy_dir = THY_ROOT / TARGET_COMBO
    tb_dir = TESTBED_ROOT

    sim_data, sim_msg = load_xy_pair(sim_dir / sim_nrb, sim_dir / sim_dvp, f"Simulation D={deadline_ms}ms")
    thy_data, thy_msg = load_xy_pair(thy_dir / thy_nrb, thy_dir / thy_dvp, f"Theory D={deadline_ms}ms")
    tb_data, tb_msg = load_xy_pair(tb_dir / tb_nrb, tb_dir / tb_dvp, f"Testbed D={deadline_ms}ms")

    for msg in [sim_msg, thy_msg, tb_msg]:
        if msg:
            print(msg)

    if sim_data is None and thy_data is None and tb_data is None:
        return None

    return {
        "deadline_ms": deadline_ms,
        "simulation": sim_data,
        "theory": thy_data,
        "testbed": tb_data,
    }


def plot_one_deadline(record, out_path: Path):
    dms = record["deadline_ms"]

    plt.figure()

    # Simulation
    if record["simulation"] is not None:
        plt.plot(
            record["simulation"]["x"],
            record["simulation"]["y"],
            linestyle="-",
            marker="o",
            linewidth=2,
            markersize=5,
            label="Simulation",
        )

    # Theory
    if record["theory"] is not None:
        plt.plot(
            record["theory"]["x"],
            record["theory"]["y"],
            linestyle="--",
            marker="x",
            linewidth=2,
            markersize=5,
            label="Theory",
        )

    # Testbed
    if record["testbed"] is not None:
        plt.plot(
            record["testbed"]["x"],
            record["testbed"]["y"],
            linestyle="-.",
            marker="s",
            linewidth=2,
            markersize=5,
            label="Testbed",
        )

    plt.xlabel("PRB per slot")
    plt.ylabel("Delay Violation Probability")
    if Y_LOG:
        plt.yscale("log")
        plt.ylim(YMIN, YMAX)

    plt.grid(True, which="both", linestyle="--", linewidth=0.5)
    plt.legend(fontsize=9, loc="best", fancybox=False, edgecolor="black")
    plt.tight_layout()
    plt.savefig(out_path, dpi=200)
    plt.close()
    print(f"[OK] saved -> {out_path}")


def plot_all_deadlines(records, out_path: Path):
    """
    deadline별로 subplot 하나씩 생성
    """
    if not records:
        return

    n = len(records)
    ncols = 2
    nrows = int(np.ceil(n / ncols))

    fig, axes = plt.subplots(nrows, ncols, figsize=(4 * ncols, 3 * nrows))
    axes = np.atleast_1d(axes).ravel()

    for ax, record in zip(axes, records):
        dms = record["deadline_ms"]

        if record["simulation"] is not None:
            ax.plot(
                record["simulation"]["x"],
                record["simulation"]["y"],
                linestyle="-",
                marker="o",
                linewidth=2,
                markersize=4,
                label="Simulation",
            )

        if record["theory"] is not None:
            ax.plot(
                record["theory"]["x"],
                record["theory"]["y"],
                linestyle="--",
                marker="x",
                linewidth=2,
                markersize=4,
                label="Theory",
            )

        if record["testbed"] is not None:
            ax.plot(
                record["testbed"]["x"],
                record["testbed"]["y"],
                linestyle="-.",
                marker="s",
                linewidth=2,
                markersize=4,
                label="Testbed",
            )

        ax.set_title(f"Deadline = {dms} slots", fontsize=10)
        ax.set_xlabel("PRB per slot")
        ax.set_ylabel("DVP")

        if Y_LOG:
            ax.set_yscale("log")
            ax.set_ylim(YMIN, YMAX)

        ax.grid(True, which="both", linestyle="--", linewidth=0.5)
        ax.legend(fontsize=8, loc="best", fancybox=False, edgecolor="black")

    # 남는 subplot 숨기기
    for ax in axes[len(records):]:
        ax.axis("off")

    plt.tight_layout()
    plt.savefig(out_path, dpi=200)
    plt.close()
    print(f"[OK] saved -> {out_path}")


def main():
    records = []

    for dms in DEADLINE_MS_LIST:
        rec = load_deadline_series(dms)
        if rec is None:
            print(f"[WARN] no valid data for deadline={dms} slots")
            continue

        records.append(rec)

        out_path = OUT_DIR / f"compare_{TARGET_COMBO}_{dms}ms.png"
        plot_one_deadline(rec, out_path)

    if records:
        out_path_all = OUT_DIR / f"compare_{TARGET_COMBO}_all_deadlines.png"
        plot_all_deadlines(records, out_path_all)
    else:
        print("[WARN] nothing to plot.")


if __name__ == "__main__":
    main()