<!--
Copyright of Description(c) 2024 KTH Royal Institute of Technology and Siemens

SPDX-License-Identifier: GPL-2.0-only
-->
# Description of the examples

The current [5G URLLC Simulation Scenario](/examples/siemens-urllc-scenario.cc) is focused on single link, currently.
We would develop the multiple links among gNB and UEs. 


## The folder structure
We develop [5G URLLC Simulation Scenario](/examples/siemens-urllc-scenario.cc) based on "cttc-nr-demo".
The other scenarios are implemented by CTTC.
## 5G URLLC Simulation Scenario

The simulation scenario is composed of 5 psteps of the codes.

1. Parameter declaration

2. Make scenario space size

3. Installing NR and EPC helper

4. Setting UDP connection

5. Running Simulation

### Parameter Settings

The parameter for URLLC simulation is defined in the delaration part.



#### Packet duplication
If you would like to simulate packet duplication, please turning off HARQ by using 
- bool isHarqEnabled = False;

After that, RLC reordering timer is activated in LTE LENA, we can turning off the reordering timer by
- Config::SetDefault("ns3::LteRlcUm::ReorderingTimer", TimeValue(MilliSeconds(0)));
If not turning off the RLC reordering timer, the long tail of delay will be observed due to reordering try in RLC layer when packet is missed.
For turning off reordering we use RLC unacknowledge mode (UM).

To adjust the number of packet duplication, please use the variable
- uint8_t maxPacketDup = N; 
which will send N+1 packets.

To set the delay period between duplicated packets,
- uint8_t duplicationDelay = 2;
in which unit is the millisecond.
(Currently, we focus on when the subframe numerology is 0 and the slot length is 1 ms)


In addition, when HARQ is turning on, you can adjust the maximum HARQ feedback (N times) by
- uint8_t maxHarqNum = N;
which will send N packets.


#### Random position
We offer the random position of UE and fixed position (that the position can be set) by
- bool isRandomPosition = false;
This fixed position for GridScenarioHelper is developed ourselves, therefore, it may not contained in NS-3 5G LENA.
(Mar. 13th, 2024)

#### TDD pattern
TDD Pattern can be defined by 
- std::string pattern = "DL|DL|DL|DL|DL|DL|DL|UL|UL|UL|"; 
If simulating the flexible slots, it can be defined by
- std::string pattern = "F|F|F|F|F|F|F|F|F|F|"; 

#### Packet Transmission Period
The packet transmission period can be set by
- uint32_t udpPacketSizeULL = 100; 
in which unit is 1 ms.

The packet size is defined by
- uint32_t lambdaULL = 100;
in which unit is bytes.

#### Simulation time
The simulation time can be defined by 
- Time simTime = MilliSeconds(1000400);
- Time udpAppStartTime = MilliSeconds(400);
where the simTime will be added 100 ms more, due to the Rx node need time to receive the packet.
If the simulation ends at the same time when UDP packet ends, few packets will be lost due to the stopped simulation immediately.


#### Link adaptation Algorithm
Target TBLER can be adjusted by 
- double targetErrorRate = 0.0001;
.
*** shannon capacity adaptive modulation and coding scheme (AMC) is not affected by the target TBLER variable.





## Author ##
Sangwon Seo

## License ##

This software is licensed under the terms of the GNU GPLv2, as like as ns-3.
See the LICENSE file for more details.
