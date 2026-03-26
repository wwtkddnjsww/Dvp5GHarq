import re
from pathlib import Path
import pandas as pd

ROOT = Path("result-tmp")

# ✅ 여러 deadline(ms)을 한 번에 처리
DEADLINE_MS_LIST = [2, 3, 5, 6, 8, 10]  # 예시: 원하는 값들로 바꾸세요
DEADLINE_FUDGE_MS = 0.5         # 기존 +0.5ms 여유 유지


# ---------------------------
# Fast IO helpers
# ---------------------------

def read_columns_fast(path):
    return list(pd.read_csv(path, delim_whitespace=True, nrows=0, engine="c").columns)


def count_data_rows_fast(path):
    """
    TxStats는 행 개수만 필요.
    첫 줄에 알파벳이 있으면 헤더로 보고 1줄 제외.
    """
    with path.open("rb") as f:
        first = f.readline()
        if not first:
            return 0
        rest_lines = sum(1 for _ in f)
        total_lines = 1 + rest_lines

    first_text = first.decode("utf-8", errors="ignore")
    has_alpha = any(("a" <= c.lower() <= "z") for c in first_text)
    return max(0, total_lines - (1 if has_alpha else 0))


def read_one_col_fast(path, col, dtype="float32"):
    return pd.read_csv(
        path,
        delim_whitespace=True,
        usecols=[col],
        dtype={col: dtype},
        engine="c",
    )[col]


# ---------------------------
# Parsing / indexing helpers
# ---------------------------

def get_mcs_from_prefix(prefix: str):
    m = re.search(r"MCS(\d+)", prefix)
    return int(m.group(1)) if m else None


def extract_prefix(name, tokens):
    for t in tokens:
        if t in name:
            return name.split(t)[0]
    return None


def build_file_index(d: Path):
    tx_tokens = ("NrUlPdcpTxStats", "TxStats")
    rx_tokens = ("NrUlPdcpRxStats", "RxStats")
    ul_tokens = ("UlRxPacketTrace",)

    tx_map: dict[str, Path] = {}
    rx_map: dict[str, Path] = {}
    ul_map: dict[str, Path] = {}

    for p in d.glob("*.txt"):
        name = p.name

        pref = extract_prefix(name, tx_tokens)
        if pref is not None:
            tx_map[pref] = p
            continue

        pref = extract_prefix(name, rx_tokens)
        if pref is not None:
            rx_map[pref] = p
            continue

        pref = extract_prefix(name, ul_tokens)
        if pref is not None:
            ul_map[pref] = p
            continue

    return tx_map, rx_map, ul_map


# ---------------------------
# Metric computations
# ---------------------------

def load_rx_delays(rx_path):
    cols = read_columns_fast(rx_path)
    if "delay(s)" not in cols:
        raise ValueError(
            f"Rx 파일에 delay(s) 컬럼이 없습니다: {rx_path.name} / cols={cols}"
        )
    return read_one_col_fast(rx_path, "delay(s)", dtype="float32")


def compute_prb_per_slot_fast(ul_path):
    cols = read_columns_fast(ul_path)
    if "nRB" not in cols:
        raise ValueError(
            f"UlRxPacketTrace에 nRB 컬럼이 없습니다: {ul_path.name} / cols={cols}"
        )
    nrb = read_one_col_fast(ul_path, "nRB", dtype="float32")
    return float(nrb.mean())


def dvp_from_delays(tx_cnt, delays, deadline_s):
    if tx_cnt <= 0:
        return float("nan")
    hit_cnt = (delays < deadline_s).sum()
    return float(1.0 - (hit_cnt / tx_cnt))


# ---------------------------
# Main
# ---------------------------

def main():
    combo_dirs = sorted([p for p in ROOT.glob("sinr*_par*_srs*") if p.is_dir()])
    if not combo_dirs:
        print(f"[WARN] {ROOT} 아래에 sinr*_par*_srs* 폴더를 못 찾았어요.")
        return

    made = 0

    # deadline(ms) -> deadline(s) 변환(+여유)
    deadline_s_list = [
        (ms + DEADLINE_FUDGE_MS) / 1000.0 for ms in DEADLINE_MS_LIST
    ]

    for d in combo_dirs:
        tx_map, rx_map, ul_map = build_file_index(d)
        if not tx_map:
            continue

        # ✅ deadline과 무관한 값은 캐시
        nrb_by_mcs: dict[int, float] = {}
        txcnt_by_mcs: dict[int, int] = {}
        delays_by_mcs: dict[int, pd.Series] = {}

        # 먼저 MCS별 기본 데이터 로딩(한 번만)
        for prefix, tx_path in sorted(tx_map.items(), key=lambda kv: kv[0]):
            mcs = get_mcs_from_prefix(prefix)
            if mcs is None:
                continue

            rx_path = rx_map.get(prefix)
            ul_path = ul_map.get(prefix)
            if rx_path is None or ul_path is None:
                print(f"[SKIP] 파일 매칭 실패: {d} / prefix={prefix} (rx/ul 누락)")
                continue

            try:
                tx_cnt = count_data_rows_fast(tx_path)
                prb_mean = compute_prb_per_slot_fast(ul_path)
                delays = load_rx_delays(rx_path)  # deadline별 재사용
            except Exception as e:
                print(f"[SKIP] 기본 로딩 실패: {d} / MCS{mcs} -> {e}")
                continue

            # 중복 MCS는 마지막 값으로 덮어씀(원 로직 유지)
            txcnt_by_mcs[mcs] = tx_cnt
            nrb_by_mcs[mcs] = prb_mean
            delays_by_mcs[mcs] = delays

        if not nrb_by_mcs:
            continue

        mcs_sorted = sorted(nrb_by_mcs.keys())

        # ✅ deadline별로 DVP 파일 따로 생성
        for ms, deadline_s in zip(DEADLINE_MS_LIST, deadline_s_list):
            dvp_by_mcs: dict[int, float] = {}

            for mcs in mcs_sorted:
                tx_cnt = txcnt_by_mcs[mcs]
                delays = delays_by_mcs[mcs]
                dvp_by_mcs[mcs] = dvp_from_delays(tx_cnt, delays, deadline_s)

            # "있는 MCS만" 1열 저장(정렬)
            nrb_col = [nrb_by_mcs[m] for m in mcs_sorted]
            dvp_col = [dvp_by_mcs[m] for m in mcs_sorted]

            nrb_out = d / f"{ms}NRBListSimulation.csv"
            dvp_out = d / f"{ms}DVPListSimulation.csv"

            pd.DataFrame(nrb_col).to_csv(nrb_out, index=False, header=False)
            pd.DataFrame(dvp_col).to_csv(dvp_out, index=False, header=False)

            print(f"[OK] saved -> {nrb_out}")
            print(f"[OK] saved -> {dvp_out}")

        made += 1

    print(f"Done. CSV generated for {made} folders.")


if __name__ == "__main__":
    main()