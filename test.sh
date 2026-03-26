# ./ns3 run "siemens-la-scenario-sched-low --simTag=_test_Fixed5_5_ --totalTxPower=18 --channelCoherence=8 --Scheduler=ns3::NrMacSchedulerOfdmaRR --maxPacketDup=2 --MCSindex=5"
# ./ns3 run "siemens-la-scenario-sched1 --simTag=_EDF2DUP_Fixed5_5_ --totalTxPower=-17 --channelCoherence=8 --Scheduler=ns3::NrMacSchedulerOfdmaEdf --ScheduleingDiscardTimer=10 --maxPacketDup=2 --duplicationDelay=2500 --MCSindex=5"
./ns3 run "siemens-la-scenario-low --simTag=_2REP_Fixed5_5_ --totalTxPower=-17 --channelCoherence=8 --Scheduler=ns3::NrMacSchedulerOfdmaRR --maxPacketDup=2 --MCSindex=5"
