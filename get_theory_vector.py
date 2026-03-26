import re
from pathlib import Path
import pandas as pd

# ================== 설정 ==================
ROOT = Path("result-tmp")
PAR_FIXED = 3
SRS_FIXED = 80
OUT_MFILE = Path("getLenaHarqBler.m")

RV_VALUES = [0, 1, 2, 3]  # rv=재전송횟수
# =========================================


def load_ws(path: Path):
    return pd.read_csv(path, sep=r"\s+", engine="python")


def parse_sinr_par_srs(folder_name):
    m1 = re.search(r"sinr(-?\d+)", folder_name)
    m2 = re.search(r"par(\d+)", folder_name)
    m3 = re.search(r"srs(\d+)", folder_name)
    sinr = int(m1.group(1)) if m1 else None
    par = int(m2.group(1)) if m2 else None
    srs = int(m3.group(1)) if m3 else None
    return sinr, par, srs


def find_ul_trace_files(d):
    return sorted(d.glob("*UlRxPacketTrace*.txt"))


def col_ci(df, wanted,):
    """case-insensitive 컬럼 매칭"""
    wanted_l = wanted.lower()
    for c in df.columns:
        if str(c).lower() == wanted_l:
            return c
    return None


def tbler_cell_mean(x):
    """
    TBler 셀 하나에 여러 숫자가 들어있을 수 있어서:
      - 숫자 1개면 그대로
      - "0.1 0.2", "0.1,0.2", "[0.1,0.2]" 등은 숫자 토큰 뽑아서 평균
    """
    if x is None or (isinstance(x, float) and pd.isna(x)):
        return float("nan")
    if isinstance(x, (int, float)):
        return float(x)

    s = str(x).strip()
    if not s:
        return float("nan")

    # 괄호류 제거
    s = re.sub(r"[\[\]\(\)\{\}]", " ", s)

    # 숫자 토큰 추출 (지수표기 포함)
    tokens = re.findall(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", s)
    if not tokens:
        return float("nan")

    vals = []
    for t in tokens:
        try:
            vals.append(float(t))
        except Exception:
            pass

    return float(sum(vals) / len(vals)) if vals else float("nan")


def matlab_vec(vec):
    return "[" + ", ".join(f"{v:.6f}" for v in vec) + "]"


def compute_table():
    """
    return:
      table[sinr][mcs] = [p0,p1,p2,p3]
    규칙:
      - SINR은 폴더명에서만 가져옴 (파일 내 SINR 컬럼 무시)
      - TBler 셀 내부에 여러 값이면 먼저 평균
      - (sinr, mcs, rv) 별로 평균 -> p_rv
      - 해당 (mcs, rv) 샘플이 없으면 0.0으로 채움 (원하면 NaN으로 바꿔도 됨)
    """
    table = {}

    combo_dirs = sorted([p for p in ROOT.glob("sinr*_par*_srs*") if p.is_dir()])
    for d in combo_dirs:
        sinr, par, srs = parse_sinr_par_srs(d.name)
        if sinr is None or par is None or srs is None:
            continue
        if par != PAR_FIXED or srs != SRS_FIXED:
            continue

        ul_files = find_ul_trace_files(d)
        if not ul_files:
            continue

        all_rows = []
        for f in ul_files:
            try:
                df = load_ws(f)
            except Exception as e:
                print(f"[SKIP] 읽기 실패: {f} -> {e}")
                continue

            rv_col = col_ci(df, "rv")
            mcs_col = col_ci(df, "mcs")
            tbler_col = col_ci(df, "TBler") or col_ci(df, "tbler")

            if rv_col is None:
                print(f"[SKIP] rv 컬럼 없음: {f.name} / cols={list(df.columns)}")
                continue
            if mcs_col is None:
                print(f"[SKIP] mcs 컬럼 없음: {f.name} / cols={list(df.columns)}")
                continue
            if tbler_col is None:
                print(f"[SKIP] TBler 컬럼 없음: {f.name} / cols={list(df.columns)}")
                continue

            tmp = pd.DataFrame({
                "mcs": pd.to_numeric(df[mcs_col], errors="coerce"),
                "rv": pd.to_numeric(df[rv_col], errors="coerce"),
                "tbler_mean_in_cell": df[tbler_col].apply(tbler_cell_mean),
            }).dropna(subset=["mcs", "rv", "tbler_mean_in_cell"])

            tmp = tmp[tmp["rv"].isin(RV_VALUES)]
            if not tmp.empty:
                all_rows.append(tmp)

        if not all_rows:
            continue

        big = pd.concat(all_rows, ignore_index=True)

        # (mcs, rv)별로 평균 TBler
        g = big.groupby(["mcs", "rv"])["tbler_mean_in_cell"].mean()

        table.setdefault(sinr, {})
        mcs_list = sorted(set(int(x) for x in big["mcs"].unique()))
        for mcs in mcs_list:
            vec = []
            for rv in RV_VALUES:
                v = g.get((mcs, rv), None)
                vec.append(float(v) if v is not None else 0.0)
            table[sinr][mcs] = vec

        print(f"[OK] {d.name} -> sinr={sinr}, mcs_count={len(table[sinr])}")

    return table


def write_matlab_file(table, out_path):
    lines = []
    lines.append("function [expectedBlerVector] = getLenaHarqBler( ...")
    lines.append("        iterLen, mcsIndex, snrRcvd_avg, hvar, M_Harq, ...")
    lines.append("        bgType, cbsTarget, Nrb)")
    lines.append("%GETLENAHARQBLER Auto-generated from UlRxPacketTrace (par=3, srs=80 fixed)")
    lines.append("% expectedBlerVector = [p0, p1, p2, p3] where p_k is mean TBLER for rv=k")
    lines.append("% NOTE: SINR is taken from folder name sinr*_par3_srs80 (file SINR(dB) ignored).")
    lines.append("")
    lines.append("%#ok<*INUSD>")
    lines.append("sinrDb = round(snrRcvd_avg); % assume caller passes ~folder SINR(dB)")
    lines.append("expectedBlerVector = [NaN, NaN, NaN, NaN];")
    lines.append("")
    lines.append("switch sinrDb")

    for sinr in sorted(table.keys()):
        lines.append(f"    case {sinr}")
        lines.append("        switch mcsIndex")
        for mcs in sorted(table[sinr].keys()):
            vec = table[sinr][mcs]
            lines.append(f"            case {mcs}")
            lines.append(f"                expectedBlerVector = {matlab_vec(vec)};")
        lines.append("            otherwise")
        lines.append("                error('SINR=%d dB, MCS %d에 대한 테이블이 없습니다.', sinrDb, mcsIndex);")
        lines.append("        end")
        lines.append("")

    lines.append("    otherwise")
    lines.append("        error('SINR=%d dB에 대한 테이블이 없습니다.', sinrDb);")
    lines.append("end")
    lines.append("end")

    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"[OK] wrote MATLAB file -> {out_path.resolve()}")


def main():
    table = compute_table()
    if not table:
        print("[WARN] 테이블이 비어있습니다. (컬럼명/폴더패턴 확인 필요)")
        return
    write_matlab_file(table, OUT_MFILE)


if __name__ == "__main__":
    main()