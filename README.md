# 5G URLLC Simulation


##How to start!

#### 0. prerequisites
Please refer to the following links
[https://www.nsnam.org/wiki/Installation#Prerequisites](https://www.nsnam.org/wiki/Installation#Prerequisites)

#### 1. go to the path
For example
```
cd ~/To/Your/Path/Dvp5GHarq
```

#### 2. configure for simulations
```
./ns3 configure --build-profile=optimized --enable-module=nr --enable-examples
```
--build-profile=optimized makes logging be not available in the terminal.


#### 3. build files
```
./ns3 build
```

#### 4. run our simulation scenario
- Single Scenario
```
./ns3 run dvp
```

- For all simulations
```
sh script_orig.sh
```

Once you build in your local repository, you don't need to configure 


## 5G DVP Implmenetations

For detailed simulation setup, please refer to ``scratch/dvp/dvp.cc`` and ``script_orig.sh`` 

