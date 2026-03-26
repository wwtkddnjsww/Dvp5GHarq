import re
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

ROOT = Path("results")
DEADLINE_S = 0.002

OUTNAME = "dvp_vs_prb_log.png"
YMIN, YMAX = 1e-5, 1.0
EPS = YMIN  # log(0) 방지

def load_ws(path: Path) -> pd.DataFrame:
    return pd.read_csv(path, sep=r"\s+", engine="python")

def get_mcs_from_prefix(prefix: str):
    m = re.search(r"MCS(\d+)", prefix)
    return int(m.group(1)) if m else None

def find_files_for_prefix(d: Path, prefix: str):
    tx = d / f"{prefix}NrUlPdcpTxStats.txt"
    rx = d / f"{prefix}NrUlPdcpRxStats.txt"
    ul = d / f"{prefix}UlRxPacketTrace.txt"

    # 완화 검색
    if not tx.exists():
        cand = sorted(d.glob(f"{prefix}*TxStats*.txt"))
        tx = cand[0] if cand else tx
    if not rx.exists():
        cand = sorted(d.glob(f"{prefix}*RxStats*.txt"))
        rx = cand[0] if cand else rx
    if not ul.exists():
        cand = sorted(d.glob(f"{prefix}*RxPacketTrace*.txt"))
        ul = cand[0] if cand else ul

    return (tx if tx.exists() else None,
            rx if rx.exists() else None,
            ul if ul.exists() else None)

def compute_dvp(tx_path: Path, rx_path: Path, deadline_s: float) -> float:
    tx = load_ws(tx_path)
    rx = load_ws(rx_path)
    deadline_s_add = deadline_s + 0.0005
    tx_cnt = len(tx)
    if tx_cnt == 0:
        return float("nan")

    if "delay(s)" not in rx.columns:
        raise ValueError(f"Rx 파일에 delay(s) 컬럼이 없습니다: {rx_path.name} / cols={list(rx.columns)}")

    hit_cnt = (rx["delay(s)"] < deadline_s_add).sum()
    dvp = 1.0 - (hit_cnt / tx_cnt)

    # log 범위용 클리핑
    dvp = max(min(dvp, YMAX), EPS)
    return float(dvp)

def compute_prb_per_slot(ul_path: Path) -> float:
    ul = load_ws(ul_path)
    # 컬럼명 표준화
    if "Time" in ul.columns:
        ul = ul.rename(columns={"Time": "time"})
    if "nRB" not in ul.columns:
        raise ValueError(f"UlRxPacketTrace에 nRB 컬럼이 없습니다: {ul_path.name} / cols={list(ul.columns)}")
    # 대표값: 평균 PRB
    return float(ul["nRB"].mean())

def main():
    combo_dirs = sorted([p for p in ROOT.glob("sinr*_par*_srs*") if p.is_dir()])
    if not combo_dirs:
        print(f"[WARN] {ROOT} 아래에 sinr*_par*_srs* 폴더를 못 찾았어요.")
        return

    made = 0
    for d in combo_dirs:
        tx_files = sorted(d.glob("*TxStats*.txt"))
        if not tx_files:
            continue
        ##Sangwon add theory part
        theory_points = []
        plt.figure()
        try:
            dvp_theory = pd.read_csv(d/f"3msDVP{DEADLINE_S}.csv", header=None)
            nrb_theory = pd.read_csv(d/f"3msNRBList{DEADLINE_S}.csv", header=None)
            # x = nrb_theory.iloc[0].to_numpy()   # shape (27,)
            # y = dvp_theory.iloc[0].to_numpy()   # shape (27,)
            x = pd.to_numeric(nrb_theory.iloc[0], errors="coerce").to_numpy()
            y = pd.to_numeric(dvp_theory.iloc[0], errors="coerce").to_numpy()
            print(nrb_theory.dtypes)
            print(dvp_theory.dtypes)
            print(x)
            print(y)
            plt.plot(x, y, marker="x", linestyle="--", label="Theory")
            
        except:
            print(f"[INFO] 이론값 파일 누락: {d} / 3msDVP{DEADLINE_S}.csv or 3msNRBList{DEADLINE_S}.csv")
        points = []  # (x_prb_mean, y_dvp, mcs)
        for tx_file in tx_files:
            name = tx_file.name
            prefix = name.split("NrUlPdcpTxStats")[0] if "NrUlPdcpTxStats" in name else name.split("TxStats")[0]

            mcs = get_mcs_from_prefix(prefix)
            if mcs is None:
                continue

            tx_path, rx_path, ul_path = find_files_for_prefix(d, prefix)
            if tx_path is None or rx_path is None or ul_path is None:
                print(f"[SKIP] 파일 매칭 실패: {d} / prefix={prefix} (tx/rx/ul 중 누락)")
                continue

            try:
                dvp = compute_dvp(tx_path, rx_path, DEADLINE_S)
                prb_mean = compute_prb_per_slot(ul_path)
            except Exception as e:
                print(f"[SKIP] 처리 실패: {d} / MCS{mcs} -> {e}")
                continue

            points.append((prb_mean, dvp, mcs))

        if not points:
            continue

        # x축(PRB) 기준 정렬해서 “꺾은선” 형태로
        points.sort(key=lambda x: x[0])
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        mcs_labels = [p[2] for p in points]


        plt.plot(xs, ys, marker="o", label="Simulation")

        # 점 옆에 MCS 표시 (원하면 주석처리)
        # for x, y, mcs in points:
            # plt.annotate(f"MCS{mcs}", (x, y), textcoords="offset points", xytext=(5, 5), fontsize=8)

        plt.yscale("log")
        plt.ylim(YMIN, YMAX)
        plt.legend()
        plt.xlabel("PRB per slot (mean nRB from UlRxPacketTrace)")
        plt.ylabel(f"DVP = 1 - #(Rx delay<{DEADLINE_S}s)/#Tx   (log, {YMIN}~{YMAX})")
        plt.title(d.name)
        plt.grid(True, which="both", linestyle="dotted", linewidth=0.1)
        plt.tight_layout()

        out_path = d / OUTNAME
        plt.savefig(out_path, dpi=200)
        plt.close()

        made += 1
        print(f"[OK] saved -> {out_path}")

    print(f"Done. Plots generated for {made} folders.")

if __name__ == "__main__":
    main()