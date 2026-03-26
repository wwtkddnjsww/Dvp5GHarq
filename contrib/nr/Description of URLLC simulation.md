<!--
Copyright of Description(c) 2024 KTH Royal Institute of Technology and Siemens

SPDX-License-Identifier: GPL-2.0-only
-->
# Description of 5G URLLC Simulation

The link to our main repository direction [README.md](../../README.md).

## The folder structure

In our 5G URLLC simulation, the scenario is included in 
[5G URLLC Simulation Scenario](/examples/siemens-urllc-scenario.cc).

For 5G architecture, 5G LENA is non standard alone (NSA), therefore, physical layer and MAC layer protocols are included in this module.
Otherwise, RLC, PDCP, and core network is defined in [LTE LENA](../../src/lte)
Therefore, if the 5G feature upper than MAC layer, please refer to LTE LENA module.
Moreover, if you find regarding antenna, propagation, and fading model, it would be highly recommneded to review the implementation in [Antenna Module](../../src/antenna), [Propagation Module](../../src/propagation/), and [Spectrum Module](../../src/spectrum/).
In these modules, you may find phy properties in the files where the names include "three-gpp-**".




## Simulation Scenario

Please refer to the document in [Scenario.md](/examples/SCENARIO.md)


## LTE LENA (PDCP and RLC Layers)

PDCP duplication implementation is located in LTE LENA module.
Please refer to the document in [LTE-LENA-for-URLLC](../../src/lte/LTE LENA for URLLC.md)


## NR (5G LENA) (PHY and MAC Layers)

PHY and MAC layers are located in here which include scheduling algorithms, adaptive modulation and coding schemes, and structures except antenna, fading, and propagation loss.


## Spectrum



## Propagation



## Antenna Module





## Author ##
Sangwon Seo

## License ##

This software is licensed under the terms of the GNU GPLv2, as like as ns-3.
See the LICENSE file for more details.
