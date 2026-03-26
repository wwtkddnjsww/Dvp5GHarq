/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

/**
 * \ingroup examples
 * \file siemens-urllc-scenario.cc
 * \brief 
 *
 * Notice: this entire program uses technical terms defined by the 3GPP TS 38.300 [1].
 *
 * This example describes how to setup a simulation using the 3GPP channel model from TR 38.901 [2].
 * This example consists of a simple grid topology, in which you
 * can choose the number of gNbs and UEs. Have a look at the possible parameters
 * to know what you can configure through the command line.
 *
 * With the default configuration, the example will create two flows that will
 * go through two different subband numerologies (or bandwidth parts). For that,
 * specifically, two bands are created, each with a single CC, and each CC containing
 * one bandwidth part.
 *
 * The example will print on-screen the end-to-end result of one (or two) flows,
 * as well as writing them on a file.
 *
 * \code{.unparsed}
$ ./ns3 run "siemens-urllc-scenario --PrintHelp"
    \endcode
 *
 */

// NOLINTBEGIN
// clang-format off

/**
 * Useful references that will be used for this tutorial:
 * [1] <a href="https://portal.3gpp.org/desktopmodules/Specifications/SpecificationDetails.aspx?specificationId=3191">3GPP TS 38.300</a>
 * [2] <a href="https://portal.3gpp.org/desktopmodules/Specifications/SpecificationDetails.aspx?specificationId=3173">3GPP channel model from TR 38.901</a>
 * [3] <a href="https://www.nsnam.org/docs/release/3.38/tutorial/html/tweaking.html#using-the-logging-module">ns-3 documentation</a>
 */

// clang-format on
// NOLINTEND

/*
 * Include part. Often, you will have to include the headers for an entire module;
 * do that by including the name of the module you need with the suffix "-module.h".
 */

#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"


/*
 * Use, always, the namespace ns3. All the NR classes are inside such namespace.
 */
using namespace ns3;

/*
 * With this line, we will be able to see the logs of the file by enabling the
 * component "CttcNrDemo".
 * Further information on how logging works can be found in the ns-3 documentation [3].
 */
NS_LOG_COMPONENT_DEFINE("CttcNrDemo");

int
main(int argc, char* argv[])
{

    // LogComponentEnable("NrGnbMac", LOG_LEVEL_INFO);
    // LogComponentEnableAll(LOG_LEVEL_ALL);
    /*
     * Variables that represent the parameters we will accept as input by the
     * command line. Each of them is initialized with a default value, and
     * possibly overridden below when command-line arguments are parsed.
     */
    // Scenario parameters (that we will use inside this script):
    
    uint16_t gNbNum = 1;
    uint16_t ueNum = 40;
    uint16_t positionResetTime = 500; // Reset time for 8ms case = (8000ms), 2ms case = 2000ms.

    bool logging = true;
    bool isHarqEnabled = false;
    bool isRandomPosition = true;
    bool isFadingEnabled = true;
    uint8_t maxHarqNum = 5;
    std::string schedulerType = "ns3::NrMacSchedulerOfdmaRR"; // RR, MR, PF, Edf, Qos
    //CA duplication only use this value
    bool caDuplication = false;
    
    // Traffic parameters (that we will use inside this script):
    uint32_t udpPacketSizeULL = 100; //orig 100
    uint32_t lambdaULL = 500; // The number of packets in one second 8ms: 125, 2ms=500

    // Simulation parameters. Please don't use double to indicate seconds; use
    // ns-3 Time values which use integers to avoid portability issues.
    Time simTime = MilliSeconds(50400);// simtime for 8ms case = 800 second. 2ms case = 200 second
    Time udpAppStartTime = MilliSeconds(400);
    uint16_t channelCoherence = 10;
    std::string pattern = "DL|DL|DL|UL|UL|DL|DL|DL|UL|UL|"; 
    // std::string pattern = "F|F|F|F|F|F|F|F|F|F|"; 

    // MCS parameters
    uint8_t fixedMcs = 15;
    bool useFixedMcs = true;

    // NR parameters. We will take the input from the command line, and then we
    // will pass them inside the NR module.
    uint16_t numerologyBwp1 = 1;
    double centralFrequencyBand1 = 3.725e9; // 3.8GHz -> from Siemens webpage
    double bandwidthBand1 = 80e6; // 100MHz for single and 20MHz for 10 UEs
    double totalTxPower = 24; //orig 4, for single transmission -10dbm

    uint16_t numerologyBwp2 = 1;
    double centralFrequencyBand2 = 20.0e9; // 
    double bandwidthBand2 = 20e6; // 100MHz for single and 20MHz for 10 UEs
    
    double targetErrorRate = 0.0001;  // should be set to value in range 0 to 1

    bool isFixedDuplication = true;
    uint8_t maxPacketDup = 0; // the number of packet duplications
    uint16_t duplicationDelay = 500; //in microseconds


    // bool discardingRepetitionPacket = false;
    bool independentChannelInstanceForSinglePaket = true;
    uint16_t schedulingDiscardTimerMs = 0; 
    // Where we will store the output files.
    std::string simTag = "_1_";
    std::string outputDir = "./Result";

    uint16_t ReorderingTimerVal = 10;
    //For MAC Related Parameters                                                          Given setting
    uint32_t n0Delay = 0; // Minimum decoding delay of Dl DCI                               0 (slots)
    uint32_t n1Delay = 2; // Minimum processing delay to generate HARQ (UE side)            2 (slots)
    uint32_t n2Delay = 2; // Minimum processing delay of UL_DCI (UE side)                   2 (slots)
    uint8_t ulCtrlSymbols = 2;

    /*
     * From here, we instruct the ns3::CommandLine class of all the input parameters
     * that we may accept as input, as well as their description, and the storage
     * variable.
     */
    CommandLine cmd(__FILE__);

    cmd.AddValue("gNbNum", "The number of gNbs in multiple-ue topology", gNbNum);
    cmd.AddValue("ueNum", "The number of UEs in multiple-ue topology", ueNum);
    cmd.AddValue("logging", "Enable logging", logging);
    cmd.AddValue("packetSizeUll",
                 "packet size in bytes to be used by ultra low latency traffic",
                 udpPacketSizeULL);
    cmd.AddValue("lambdaUll",
                 "Number of UDP packets in one second for ultra low latency traffic",
                 lambdaULL);
    cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.AddValue("numerologyBwp1", "The numerology to be used in bandwidth part 1", numerologyBwp1);
    cmd.AddValue("centralFrequencyBand1",
                 "The system frequency to be used in band 1",
                 centralFrequencyBand1);
    cmd.AddValue("bandwidthBand1", "The system bandwidth to be used in band 1", bandwidthBand1);
    cmd.AddValue("numerologyBwp2", "The numerology to be used in bandwidth part 2", numerologyBwp2);
    cmd.AddValue("centralFrequencyBand2",
                 "The system frequency to be used in band 2",
                 centralFrequencyBand2);
    cmd.AddValue("bandwidthBand2", "The system bandwidth to be used in band 2", bandwidthBand2);

    cmd.AddValue("totalTxPower",
                 "total tx power that will be proportionally assigned to"
                 " bands, CCs and bandwidth parts depending on each BWP bandwidth ",
                 totalTxPower);
    cmd.AddValue("simTag",
                 "tag to be appended to output filenames to distinguish simulation campaigns",
                 simTag);
    cmd.AddValue("outputDir", "directory where to store simulation results", outputDir);
    cmd.AddValue("tddPattern",
                "LTE TDD pattern to use (e.g. --tddPattern=DL|S|UL|UL|UL|DL|S|UL|UL|UL|)",
                pattern);
    cmd.AddValue("caDup",
                "CA Duplication, (true or false)",
                caDuplication);
    cmd.AddValue("HARQ",
                "HARQ",
                isHarqEnabled);
    cmd.AddValue("maxPacketDup",
                "The number of duplicated packets",
                maxPacketDup);
    cmd.AddValue("duplicationDelay",
                "The interval between duplicated packets",
                duplicationDelay);
    cmd.AddValue("channelCoherence",
                "Channel coherence time in millisecond",
                channelCoherence);
    cmd.AddValue("isFixedMCS",
                "use fixed MCS or EESM'",
                useFixedMcs);
    cmd.AddValue("MCSindex",
                "Fixed MCS index, please check 'bool fixedMCS is true'",
                fixedMcs);
    cmd.AddValue("cyclePeriod",
                "The number of packet sent in one second, e.g., 8ms=125, 2ms=500",
                lambdaULL);
    cmd.AddValue("Scheduler",
                "SchedulerType",
                schedulerType);
    cmd.AddValue("ScheduleingDiscardTimer",
                "Scheduler will discard the packets in the RLC buffer if timer is over (setting in milliseconds)",
                schedulingDiscardTimerMs);
    cmd.AddValue("IndependentPacketInstance",
                "Independently send duplicated packet or original packet. If this value is true, in one slot, every UE can send only one packet",
                independentChannelInstanceForSinglePaket);
    cmd.AddValue("ReorderingTimer",
                "Reordering timer in RLC layer",
                ReorderingTimerVal);

    // Parse the command line
    cmd.Parse(argc, argv);

    /*
     * Check if the frequency is in the allowed range.
     * If you need to add other checks, here is the best position to put them.
     */
    NS_ABORT_IF(centralFrequencyBand1 < 0.5e9 && centralFrequencyBand1 > 100e9);
    // NS_ABORT_IF(centralFrequencyBand2 < 0.5e9 && centralFrequencyBand2 > 100e9);

    /*
     * If the logging variable is set to true, enable the log of some components
     * through the code. The same effect can be obtained through the use
     * of the NS_LOG environment variable:
     *
     * export NS_LOG="UdpClient=level_info|prefix_time|prefix_func|prefix_node:UdpServer=..."
     *
     * Usually, the environment variable way is preferred, as it is more customizable,
     * and more expressive.
     */
    if (logging)
    {
        LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
        LogComponentEnable("LtePdcp", LOG_LEVEL_INFO);
    }

    /*
     * In general, attributes for the NR module are typically configured in NrHelper.  However, some
     * attributes need to be configured globally through the Config::SetDefault() method. Below is
     * an example: if you want to make the RLC buffer very large, you can pass a very large integer
     * here.
     */
    Config::SetDefault("ns3::UeManager::CaDuplication", BooleanValue(caDuplication));
    Config::SetDefault("ns3::NrBearerStatsSimple::DlRlcTxOutputFilename", StringValue(simTag+"NrDlTxRlcStat.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::DlRlcRxOutputFilename", StringValue(simTag+"NrDlRxRlcStats.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::UlRlcTxOutputFilename", StringValue(simTag+"NrUlRlcTxStats.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::UlRlcRxOutputFilename", StringValue(simTag+"NrUlRlcRxStats.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::DlPdcpTxOutputFilename", StringValue(simTag+"NrDlPdcpTxStats.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::DlPdcpRxOutputFilename", StringValue(simTag+"NrDlPdcpRxStats.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::UlPdcpTxOutputFilename", StringValue(simTag+"NrUlPdcpTxStats.txt"));
    Config::SetDefault("ns3::NrBearerStatsSimple::UlPdcpRxOutputFilename", StringValue(simTag+"NrUlPdcpRxStats.txt"));
    
    Config::SetDefault("ns3::NrPhyRxTrace::SimTag", StringValue(simTag));
    Config::SetDefault("ns3::NrMacRxTrace::SimTag", StringValue(simTag));
    /*
     * In general, attributes for the NR module are typically configured in NrHelper.  However, some
     * attributes need to be configured globally through the Config::SetDefault() method. Below is
     * an example: if you want to make the RLC buffer very large, you can pass a very large integer
     * here.
     */
    Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping",EnumValue(LteEnbRrc::RLC_TM_ALWAYS));
    Config::SetDefault("ns3::NrGnbPhy::Pattern", StringValue(pattern));
    Config::SetDefault("ns3::NrMacSchedulerNs3::UlCtrlSymbols", UintegerValue(ulCtrlSymbols));
    Config::SetDefault("ns3::NrGnbPhy::N0Delay", UintegerValue(n0Delay));
    Config::SetDefault("ns3::NrGnbPhy::N1Delay", UintegerValue(n1Delay));
    Config::SetDefault("ns3::NrGnbPhy::N2Delay", UintegerValue(n2Delay));
    Config::SetDefault("ns3::LteRlcTm::MaxTxBufferSize", UintegerValue(999999999));
    // Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    // Config::SetDefault("ns3::LteRlcUm::ReorderingTimer", TimeValue(MilliSeconds(ReorderingTimerVal)));
    // Config::SetDefault("ns3::LteRlcUm::EnablePdcpDiscarding", BooleanValue(false));
    // Config::SetDefault("ns3::LteRlcUm::DiscardingRepetitionPacket", BooleanValue(discardingRepetitionPacket));
    // Config::SetDefault("ns3::LteRlcUm::IndependentChannelInstanceForSinglePaket", BooleanValue(independentChannelInstanceForSinglePaket));
    // Config::SetDefault("ns3::LteRlcUm::DiscardTimerMs", UintegerValue(0));
    // Config::SetDefault("ns3::LteRlcUm::SchedulingDiscardTimerMs", UintegerValue(schedulingDiscardTimerMs));
    Config::SetDefault("ns3::LtePdcp::MaxDuplication", UintegerValue(maxPacketDup));
    Config::SetDefault("ns3::LtePdcp::FixedDuplication", BooleanValue(isFixedDuplication));
    Config::SetDefault("ns3::LtePdcp::DuplicatedPacketDelay", UintegerValue(duplicationDelay));
    if (isHarqEnabled == true){
        Config::SetDefault("ns3::NrUeMac::NumHarqProcess", UintegerValue(maxHarqNum));
        Config::SetDefault("ns3::NrGnbMac::NumHarqProcess", UintegerValue(maxHarqNum));
    }

    int64_t randomStream = 1;
    double ueHeight = 1.5;
    GridScenarioHelper gridScenario;
    gridScenario.SetRows(1);
    gridScenario.SetColumns(gNbNum);
    // All units below are in meters
    gridScenario.SetHorizontalBsDistance(5.0);
    gridScenario.SetVerticalBsDistance(5.0);
    gridScenario.SetBsHeight(3.0);
    gridScenario.SetUtHeight(ueHeight);
    // must be set before BS number
    gridScenario.SetSectorization(GridScenarioHelper::SINGLE);
    gridScenario.SetBsNumber(gNbNum);
    gridScenario.SetUtNumber(ueNum);
    gridScenario.SetScenarioHeight(100); // Create a 3x3 scenario where the UE will
    gridScenario.SetScenarioLength(100); // be distribuited.
    randomStream += gridScenario.AssignStreams(randomStream);
    

    /*
     * Create two different NodeContainer for the different traffic type.
     * In ueLowLat we will put the UEs that will receive low-latency traffic,
     * while in ueVoice we will put the UEs that will receive the voice traffic.
     */


    if (isRandomPosition == true && positionResetTime == 0)
    {
        gridScenario.CreateScenario();
    }
    else if (isRandomPosition == true && positionResetTime != 0)
    {
        std::cout<< "CALL Random Reset" << std::endl;
        gridScenario.SetPositionResetTime(positionResetTime);
        gridScenario.CreateRandomResetScenario();
    }
    else
    {
        Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator>();
         // for initialize 
        for (uint32_t kk = 0; kk <= ueNum/10; kk++){
            for (uint32_t k = 0; k < ueNum; k++)
            {
                uePositionAlloc->Add(Vector(20.0-k*1.0, 20.0-kk*1.0,ueHeight));
            }
        }
        gridScenario.CreateFixedPositionScenario(uePositionAlloc);
    }

    NodeContainer ueLowLatContainer;

    for (uint32_t j = 0; j < gridScenario.GetUserTerminals().GetN(); ++j)
    {
        Ptr<Node> ue = gridScenario.GetUserTerminals().Get(j);
        ueLowLatContainer.Add(ue);
    }

    /*
     * TODO: Add a print, or a plot, that shows the scenario.
     */
    NS_LOG_INFO("Creating " << gridScenario.GetUserTerminals().GetN() << " user terminals and "
                            << gridScenario.GetBaseStations().GetN() << " gNBs");
    /*
     * Setup the NR module. We create the various helpers needed for the
     * NR simulation:
     * - EpcHelper, which will setup the core network
     * - NrHelper, which takes care of creating and connecting the various
     * part of the NR stack
     */
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    // if (isHarqEnabled == false){
    //     nrHelper->SetDuplicationEnabled(true);
    // }
    nrHelper->SetEpcHelper(epcHelper);

    /*
     * Spectrum division. We create two operational bands, each of them containing
     * one component carrier, and each CC containing a single bandwidth part
     * centered at the frequency specified by the input parameters.
     * Each spectrum part length is, as well, specified by the input parameters.
     * Both operational bands will use the StreetCanyon channel modeling.
     * "InH_OfficeMixed"
     */
    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // in this example, both bands have a single CC

    // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
    // a single BWP per CC
    CcBwpCreator::SimpleOperationBandConf bandConf1(centralFrequencyBand1,
                                                    bandwidthBand1,
                                                    numCcPerBand,
                                                    BandwidthPartInfo::InH_OfficeMixed_LoS);


    CcBwpCreator::SimpleOperationBandConf bandConf2(centralFrequencyBand2,
                                                bandwidthBand2,
                                                numCcPerBand,
                                                BandwidthPartInfo::InH_OfficeMixed_LoS);

    // By using the configuration created, it is time to make the operation bands
    OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf1);
    OperationBandInfo band2 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf2);
    
    



    /*
     * The configured spectrum division is:
     * ------------Band1--------------|------------Band2--------------
     * ------------CC1----------------|------------CC2----------------
     * ------------BWP1---------------|------------BWP1---------------
     */

    /*
     * Attributes of ThreeGppChannelModel still cannot be set in our way.
     * TODO: Coordinate with Tommaso
     */
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(channelCoherence)));
    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(channelCoherence)));
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));



    nrHelper->SetUlErrorModel("ns3::NrEesmIrT1");
    nrHelper->SetDlErrorModel("ns3::NrEesmIrT1");
    nrHelper->SetGnbDlAmcAttribute(
    "AmcModel",
    EnumValue(NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbDlAmcAttribute(
    "TargetErrorRate",
    DoubleValue(targetErrorRate));
    nrHelper->SetGnbUlAmcAttribute(
    "AmcModel",
    EnumValue(NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute(
    "TargetErrorRate",
    DoubleValue(targetErrorRate));
    

    // // Both DL and UL AMC will have the same model behind.


    nrHelper->SetSchedulerTypeId(TypeId::LookupByName(schedulerType));
    // if this value is false, no retransmission is allowed
    nrHelper->SetSchedulerAttribute("EnableHarqReTx", BooleanValue(isHarqEnabled)); 
    

    /*
     * Initialize channel and pathloss, plus other things inside band1. If needed,
     * the band configuration can be done manually, but we leave it for more
     * sophisticated examples. For the moment, this method will take care
     * of all the spectrum initialization needs.
     */
    auto bandMask = NrHelper::INIT_PROPAGATION | NrHelper::INIT_CHANNEL;
    if (isFadingEnabled == true)
    {
        bandMask |= NrHelper::INIT_FADING;
    }
    nrHelper->InitializeOperationBand(&band1, bandMask);
    if (caDuplication == true){
        nrHelper->InitializeOperationBand(&band2, bandMask);
    }
    /*
     * Start to account for the bandwidth used by the example, as well as
     * the total power that has to be divided among the BWPs.
     */
    double x = pow(10, totalTxPower / 10);
    double totalBandwidth = bandwidthBand1;

    // Initialize channel and pathloss, plus other things inside band2
    totalBandwidth += bandwidthBand2;
    if (caDuplication== true){
        allBwps = CcBwpCreator::GetAllBwps({band1, band2});
    }
    else{
        allBwps = CcBwpCreator::GetAllBwps({band1});
    }

    

    /*
     * allBwps contains all the spectrum configuration needed for the nrHelper.
     *
     * Now, we can setup the attributes. We can have three kind of attributes:
     * (i) parameters that are valid for all the bandwidth parts and applies to
     * all nodes, (ii) parameters that are valid for all the bandwidth parts
     * and applies to some node only, and (iii) parameters that are different for
     * every bandwidth parts. The approach is:
     *
     * - for (i): Configure the attribute through the helper, and then install;
     * - for (ii): Configure the attribute through the helper, and then install
     * for the first set of nodes. Then, change the attribute through the helper,
     * and install again;
     * - for (iii): Install, and then configure the attributes by retrieving
     * the pointer needed, and calling "SetAttribute" on top of such pointer.
     *
     */

    Packet::EnableChecking();
    Packet::EnablePrinting();

    /*
     *  Case (i): Attributes valid for all the nodes
     */
    
    // Core latency
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));

    // Antennas for all the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(1));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));

    uint32_t bwpIdForLowLat = 0;

    uint32_t bwpIdForLowLatPacketDup = 1;

    if (useFixedMcs == true)
    {
        nrHelper->SetSchedulerAttribute("FixedMcsDl", BooleanValue(useFixedMcs));
        nrHelper->SetSchedulerAttribute("FixedMcsUl", BooleanValue(useFixedMcs));
        nrHelper->SetSchedulerAttribute("StartingMcsDl", UintegerValue(fixedMcs));
        nrHelper->SetSchedulerAttribute("StartingMcsUl", UintegerValue(fixedMcs));
    }


    // gNb routing between Bearer and bandwidh part
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("NGBR_LOW_LAT_EMBB",
                                                 UintegerValue(bwpIdForLowLat));

    // Ue routing between Bearer and bandwidth part
    nrHelper->SetUeBwpManagerAlgorithmAttribute("NGBR_LOW_LAT_EMBB", UintegerValue(bwpIdForLowLat));

    if (caDuplication == true){
        // gNb routing between Bearer and bandwidh part
        nrHelper->SetGnbBwpManagerAlgorithmAttribute("DGBR_DISCRETE_AUT_SMALL",
                                                    UintegerValue(bwpIdForLowLatPacketDup));

        // Ue routing between Bearer and bandwidth part
        nrHelper->SetUeBwpManagerAlgorithmAttribute("DGBR_DISCRETE_AUT_SMALL", UintegerValue(bwpIdForLowLatPacketDup));
    }
    /*
     * We miss many other parameters. By default, not configuring them is equivalent
     * to use the default values. Please, have a look at the documentation to see
     * what are the default values for all the attributes you are not seeing here.
     */

    /*
     * Case (ii): Attributes valid for a subset of the nodes
     */

    // NOT PRESENT IN THIS SIMPLE EXAMPLE

    /*
     * We have configured the attributes we needed. Now, install and get the pointers
     * to the NetDevices, which contains all the NR stack:
     */

    NetDeviceContainer enbNetDev =
        nrHelper->InstallGnbDevice(gridScenario.GetBaseStations(), allBwps);
    NetDeviceContainer ueLowLatNetDev = nrHelper->InstallUeDevice(ueLowLatContainer, allBwps);

    randomStream += nrHelper->AssignStreams(enbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueLowLatNetDev, randomStream);
    // nrHelper->AssignStreams(enbNetDev, randomStream);
    // nrHelper->AssignStreams(ueLowLatNetDev, randomStream);
    /*
     * Case (iii): Go node for node and change the attributes we have to setup
     * per-node.
     */

    // Get the first netdevice (enbNetDev.Get (0)) and the first bandwidth part (0)
    // and set the attribute.
    
    if (caDuplication == true){
        nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
            ->SetAttribute("Numerology", UintegerValue(numerologyBwp1));
        nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
            ->SetAttribute("TxPower", DoubleValue(10 * log10(x)));
            // ->SetAttribute("TxPower", DoubleValue(10 * log10(x*(centralFrequencyBand1/(centralFrequencyBand1+centralFrequencyBand2)))));

        nrHelper->GetGnbPhy(enbNetDev.Get(0), 1)
            ->SetAttribute("Numerology", UintegerValue(numerologyBwp2));
        nrHelper->GetGnbPhy(enbNetDev.Get(0), 1)
            ->SetAttribute("TxPower", DoubleValue(10 * log10(x)));
            // ->SetAttribute("TxPower", DoubleValue(10 * log10(x*(centralFrequencyBand2/(centralFrequencyBand1+centralFrequencyBand2)))));
    }
    else{
        nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
        ->SetAttribute("Numerology", UintegerValue(numerologyBwp1));
        nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
        ->SetAttribute("TxPower", DoubleValue(10 * log10(x)));

    }

    // When all the configuration is done, explicitly call UpdateConfig ()

    for (auto it = enbNetDev.Begin(); it != enbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = ueLowLatNetDev.Begin(); it != ueLowLatNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }


    // From here, it is standard NS3. In the future, we will create helpers
    // for this part as well.

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.000)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    internet.Install(gridScenario.GetUserTerminals());

    Ipv4InterfaceContainer ueLowLatIpIface =
        epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLowLatNetDev));

    // Set the default gateway for the UEs
    for (uint32_t j = 0; j < gridScenario.GetUserTerminals().GetN(); ++j)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(
            gridScenario.GetUserTerminals().Get(j)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // attach UEs to the closest eNB
    nrHelper->AttachToClosestEnb(ueLowLatNetDev, enbNetDev);

    /*
     * Traffic part. Install traffic: low-latency
     * identified by a particular source port.
     */
    uint16_t dlPortLowLat = 1234;

    ApplicationContainer serverApps;

    // The sink will always listen to the specified ports
    UdpServerHelper dlPacketSinkLowLat(dlPortLowLat);
    // dlPacketSinkLowLat.SetAttribute("ResultFilename", StringValue("UDPresult.txt"));

    // The server, that is the application which is listening, is installed in the UE
    serverApps.Add(dlPacketSinkLowLat.Install(ueLowLatContainer));

    /*
     * Configure attributes for the different generators, using user-provided
     * parameters for generating a CBR traffic
     *
     * Low-Latency configuration and object creation:
     */
    UdpClientHelper dlClientLowLat;
    dlClientLowLat.SetAttribute("RemotePort", UintegerValue(dlPortLowLat));
    dlClientLowLat.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientLowLat.SetAttribute("PacketSize", UintegerValue(udpPacketSizeULL));
    dlClientLowLat.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambdaULL)));

    // The bearer that will carry low latency traffic
    EpsBearer lowLatBearer(EpsBearer::NGBR_LOW_LAT_EMBB);
    EpsBearer lowLatBearerPacketDup(EpsBearer::DGBR_DISCRETE_AUT_SMALL);

    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat;
    dlpfLowLat.localPortStart = dlPortLowLat;
    dlpfLowLat.localPortEnd = dlPortLowLat;
    lowLatTft->Add(dlpfLowLat);
    Ptr<EpcTft> lowLatTftPacketDup = Create<EpcTft>();
    lowLatTftPacketDup->Add(dlpfLowLat);


    /*
     * Let's install the applications!
     */
    ApplicationContainer clientApps;

    for (uint32_t i = 0; i < ueLowLatContainer.GetN(); ++i)
    {
        Ptr<Node> ue = ueLowLatContainer.Get(i);
        Ptr<NetDevice> ueDevice = ueLowLatNetDev.Get(i);
        Address ueAddress = ueLowLatIpIface.GetAddress(i);

        // The client, who is transmitting, is installed in the remote host,
        // with destination address set to the address of the UE
        dlClientLowLat.SetAttribute("RemoteAddress", AddressValue(ueAddress));
        clientApps.Add(dlClientLowLat.Install(remoteHost));

        // Activate a dedicated bearer for the traffic type
        nrHelper->ActivateDedicatedEpsBearer(ueDevice, lowLatBearer, lowLatTft);
        nrHelper->ActivateDedicatedEpsBearer(ueDevice, lowLatBearerPacketDup, lowLatTftPacketDup);
    }


    // start UDP server and client apps
    NS_LOG_INFO("user plane packet delivery started");
    serverApps.Start(udpAppStartTime);
    clientApps.Start(udpAppStartTime);
    serverApps.Stop(simTime);
    clientApps.Stop(simTime);

    // enable the traces provided by the nr module
    nrHelper->EnableTraces();

    // nrHelper->EnableRlcSimpleTraces();
    // nrHelper->EnablePdcpSimpleTraces();
    // nrHelper->EnableDlDataPhyTraces();


    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    endpointNodes.Add(remoteHost);
    endpointNodes.Add(gridScenario.GetUserTerminals());

    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));

    Time additional_sim_time = MilliSeconds(100); // additional time for the rest of packet
    Simulator::Stop(simTime+additional_sim_time);
    Simulator::Run();

    /*
     * To check what was installed in the memory, i.e., BWPs of eNb Device, and its configuration.
     * Example is: Node 1 -> Device 0 -> BandwidthPartMap -> {0,1} BWPs -> NrGnbPhy -> Numerology,
    GtkConfigStore config;
    config.ConfigureAttributes ();
    */

    // Print per-flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::ofstream outFile;
    std::string filename = outputDir + "/" + simTag;
    outFile.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "Can't open file " << filename << std::endl;
        return 1;
    }

    outFile.setf(std::ios_base::fixed);

    double flowDuration = (simTime - udpAppStartTime).GetSeconds();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
        {
            protoStream.str("TCP");
        }
        if (t.protocol == 17)
        {
            protoStream.str("UDP");
        }
        outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> "
                << t.destinationAddress << ":" << t.destinationPort << ") proto "
                << protoStream.str() << "\n";
        outFile << "  Tx Packets: " << i->second.txPackets << "\n";
        outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
        outFile << "  TxOffered:  " << i->second.txBytes * 8.0 / flowDuration / 1000.0 / 1000.0
                << " Mbps\n";
        outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        if (i->second.rxPackets > 0)
        {
            // Measure the duration of the flow from receiver's perspective
            averageFlowThroughput += i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

            outFile << "  Throughput: " << i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000
                    << " Mbps\n";
            outFile << "  Mean delay:  "
                    << 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets << " ms\n";
            // outFile << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << "
            // Mbps \n";
            outFile << "  Mean jitter:  "
                    << 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets << " ms\n";
        }
        else
        {
            outFile << "  Throughput:  0 Mbps\n";
            outFile << "  Mean delay:  0 ms\n";
            outFile << "  Mean jitter: 0 ms\n";
        }
        outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
    }

    double meanFlowThroughput = averageFlowThroughput / stats.size();
    double meanFlowDelay = averageFlowDelay / stats.size();

    outFile << "\n\n  Mean flow throughput: " << meanFlowThroughput << "\n";
    outFile << "  Mean flow delay: " << meanFlowDelay << "\n";

    outFile.close();

    std::ifstream f(filename.c_str());

    if (f.is_open())
    {
        std::cout << f.rdbuf();
    }

    Simulator::Destroy();

    if (argc == 0)
    {
        double toleranceMeanFlowThroughput = 0.0001 * 56.258560;
        double toleranceMeanFlowDelay = 0.0001 * 0.553292;

        if (meanFlowThroughput >= 56.258560 - toleranceMeanFlowThroughput &&
            meanFlowThroughput <= 56.258560 + toleranceMeanFlowThroughput &&
            meanFlowDelay >= 0.553292 - toleranceMeanFlowDelay &&
            meanFlowDelay <= 0.553292 + toleranceMeanFlowDelay)
        {
            return EXIT_SUCCESS;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }
    else if (argc == 1 and ueNum == 9) // called from examples-to-run.py with these parameters
    {
        double toleranceMeanFlowThroughput = 0.0001 * 47.858536;
        double toleranceMeanFlowDelay = 0.0001 * 10.504189;

        if (meanFlowThroughput >= 47.858536 - toleranceMeanFlowThroughput &&
            meanFlowThroughput <= 47.858536 + toleranceMeanFlowThroughput &&
            meanFlowDelay >= 10.504189 - toleranceMeanFlowDelay &&
            meanFlowDelay <= 10.504189 + toleranceMeanFlowDelay)
        {
            return EXIT_SUCCESS;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }
    else
    {
        return EXIT_SUCCESS; // we dont check other parameters configurations at the moment
    }
}
