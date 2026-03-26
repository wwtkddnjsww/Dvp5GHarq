import re
from pathlib import Path
import pandas as pd

ROOT = Path("result-tmp")
DEADLINE_S = 10 # ms

NRB_OUT = f"{DEADLINE_S}NRBListSimulation.csv"
DVP_OUT = f"{DEADLINE_S}DVPListSimulation.csv"



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
        cand = sorted(d.glob(f"{prefix}*UlRxPacketTrace*.txt"))
        ul = cand[0] if cand else ul

    return (tx if tx.exists() else None,
            rx if rx.exists() else None,
            ul if ul.exists() else None)

def compute_dvp(tx_path: Path, rx_path: Path, deadline_s: float) -> float:
    tx = load_ws(tx_path)
    rx = load_ws(rx_path)

    tx_cnt = len(tx)
    if tx_cnt == 0:
        return float("nan")

    if "delay(s)" not in rx.columns:
        raise ValueError(
            f"Rx 파일에 delay(s) 컬럼이 없습니다: {rx_path.name} / cols={list(rx.columns)}"
        )

    hit_cnt = (rx["delay(s)"] < deadline_s).sum()
    dvp = 1.0 - (hit_cnt / tx_cnt)
    return float(dvp)

def compute_prb_per_slot(ul_path: Path) -> float:
    ul = load_ws(ul_path)
    if "Time" in ul.columns:
        ul = ul.rename(columns={"Time": "time"})
    if "nRB" not in ul.columns:
        raise ValueError(
            f"UlRxPacketTrace에 nRB 컬럼이 없습니다: {ul_path.name} / cols={list(ul.columns)}"
        )
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

        # 중복 MCS는 마지막 값으로 덮어씀
        nrb_by_mcs = {}
        dvp_by_mcs = {}

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
                deadline_alpha = (DEADLINE_S+0.5)/1000 # + 0.5ms (약간의 여유)
                dvp = compute_dvp(tx_path, rx_path, deadline_alpha)
                prb_mean = compute_prb_per_slot(ul_path)
            except Exception as e:
                print(f"[SKIP] 처리 실패: {d} / MCS{mcs} -> {e}")
                continue

            nrb_by_mcs[mcs] = prb_mean
            dvp_by_mcs[mcs] = dvp

        if not nrb_by_mcs and not dvp_by_mcs:
            continue

        # ✅ "있는 MCS만" 정렬해서 1열(세로)로 저장
        nrb_col = [nrb_by_mcs[m] for m in sorted(nrb_by_mcs.keys())]
        dvp_col = [dvp_by_mcs[m] for m in sorted(dvp_by_mcs.keys())]

        nrb_path = d / NRB_OUT
        dvp_path = d / DVP_OUT

        # header=False, index=False => 값만 한 열로 저장
        pd.DataFrame(nrb_col).to_csv(nrb_path, index=False, header=False)
        pd.DataFrame(dvp_col).to_csv(dvp_path, index=False, header=False)

        made += 1
        print(f"[OK] saved -> {nrb_path}")
        print(f"[OK] saved -> {dvp_path}")

    print(f"Done. CSV generated for {made} folders.")

if __name__ == "__main__":
    main()