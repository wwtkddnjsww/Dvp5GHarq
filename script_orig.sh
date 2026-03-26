
set -euo pipefail

sinr_list=(10)
par_list=(3)
srs_list=(80)
mcs_list=(5 4 3 2 1)  

for sinr in "${sinr_list[@]}"; do
  for par in "${par_list[@]}"; do
    for srs in "${srs_list[@]}"; do

      baseDir="results/sinr${sinr}_par${par}_srs${srs}"
      mkdir -p "${baseDir}"


      for MCS in "${mcs_list[@]}"; do
        totalTxpower=$((sinr - 32))
        simTag="${baseDir}/MCS${MCS}_"

        ./ns3 run "dvp --simTag=${simTag} --totalTxPower=${totalTxpower} --MCSindex=${MCS} --lambdaUll=${par} --srsPeriodicity=${srs}"
      done

    done
  done
done