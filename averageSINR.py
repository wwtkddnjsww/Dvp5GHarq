import pandas as pd

# 파일 경로 (네가 올린 파일이 이 경로에 있음)
path = "_1_UlRxPacketTrace.txt"

# 탭 구분 + 헤더 사용
df = pd.read_csv(path, sep="\t")

# (선택) UL만 / 정상 수신만 평균내고 싶으면 아래 줄을 켜
# df = df[(df["direction"] == "UL") & (df["corrupt"] == 0)]

avg_sinr = df["SINR(dB)"].mean()
print("Average SINR (dB):", avg_sinr)