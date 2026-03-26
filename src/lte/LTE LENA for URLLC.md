<!--
Copyright of Description(c) 2024 KTH Royal Institute of Technology and Siemens

SPDX-License-Identifier: GPL-2.0-only
-->
# Description of LTE LENA for URLLC Simulation

## The folder structure

In our 5G URLLC simulation, mainly our implementation is located in [lte-pdcp.cc](model/lte-pdcp.cc).


## Description

Function 
- LtePdcp::DoTransmitPdcpSdu(LtePdcpSapProvider::TransmitPdcpSduParameters params) 
sends from UDP layer to RLC layer. 

The part of the code 
```
    for(uint8_t i = 0; i < m_maxDup; i++){
        Ptr<Packet> p_dup = Copy(params.pdcpSdu);
        PdcpTag pdcpTag_dup(Simulator::Now());
        // pdcpTag_dup.SetSTforDup(Simulator::Now());
        // std::cout << "Sender: cur time: " << Simulator::Now() << " pdcp dup Tag: "<< pdcpTag_dup.GetSenderTimestamp() <<std::endl;
        // PdcpTag pdcpTag_dup(Simulator::Now());
        LtePdcpHeader pdcpHeader_dup;
        pdcpHeader_dup.SetSequenceNumber(m_txSequenceNumber);
        pdcpHeader_dup.SetDcBit(LtePdcpHeader::DATA_PDU);
        p_dup->AddHeader(pdcpHeader_dup);
        p_dup->AddByteTag(pdcpTag_dup, 1, pdcpHeader_dup.GetSerializedSize());
        LteRlcSapProvider::TransmitPdcpPduParameters txParams_dup;
        txParams_dup.rnti = m_rnti;
        txParams_dup.lcid = m_lcid;
        txParams_dup.pdcpPdu = p_dup;
        NS_LOG_INFO("Transmitting Duplicated PDCP PDU with data: " << txParams_dup.pdcpPdu);
        // m_txPdu(m_rnti, m_lcid, p_dup->GetSize());
        // Simulator::Schedule(MilliSeconds(dup_delay),&LtePdcp::m_txPdu,m_rnti, m_lcid, p_dup->GetSize());
        Simulator::Schedule(MilliSeconds(m_dupDelay*(i+1)),&LteRlcSapProvider::TransmitPdcpPdu,m_rlcSapProvider,txParams_dup);// changed by sangwon
    }
```
is generating duplicated packet according to the parameter defined in [siemens-urllc-scenario.cc](../../contrib/nr/examples/siemens-urllc-scenario.cc).

The function 
- LtePdcp::DoReceivePdu(Ptr<Packet> p)
is activated in Rx node (i.e., gNB when UL, UE when DL).
The part of the code
```
    NS_LOG_LOGIC("PDCP header: " << pdcpHeader);
    if (pdcpSequencNumbers[pdcpHeader.GetSequenceNumber()] == 1){
        NS_LOG_INFO("Duplicated packet Sequence Number: "<<pdcpHeader.GetSequenceNumber());
        // accumulated_dup_vals++;
        // NS_LOG_INFO("# of duplicated pakcets: "<<accumulated_dup_vals);
    }
    else if ((m_maxSeqNotifier == true) && (pdcpHeader.GetSequenceNumber()+1 >= m_maxPdcpSn)) {
        // This else if is for the last part of duplicated packet sequences.
    }
    else{
        NS_LOG_INFO("First arrived packet sequence number: "<<pdcpHeader.GetSequenceNumber());
        m_rxPdu(m_rnti, m_lcid, p->GetSize(), delay.GetNanoSeconds()); // temporal move to check result.
        std::cout<< "Time: "<< Simulator::Now() << " Delay: " << delay << " Timestamp: " << pdcpTag.GetSenderTimestamp() << " SeqNum: " << pdcpHeader.GetSequenceNumber() << std::endl; 
    
        pdcpSequencNumbers[pdcpHeader.GetSequenceNumber()] = 1;
        m_rxSequenceNumber = pdcpHeader.GetSequenceNumber()+1;
        // accumulated_seq_vals++;
        // if (accumulated_seq_vals == m_max_dup*MAX_PDCP_SN){
        // }
        if (m_rxSequenceNumber >= m_maxPdcpSn)
        {
            m_maxSeqNotifier = true;
            m_rxSequenceNumber = 0;
            memset(pdcpSequencNumbers,0,sizeof(uint16_t)*5000);
            // accumulated_seq_vals = 0;
        }
        if ((m_rxSequenceNumber > 2000) && (m_rxSequenceNumber < 3000) && (m_maxSeqNotifier == true)){
            m_maxSeqNotifier = false; // initialize the Max seq notifier when the enough number of sequence number is reached.
        }
        LtePdcpSapUser::ReceivePdcpSduParameters params;
        params.pdcpSdu = p;
        params.rnti = m_rnti;
        params.lcid = m_lcid;
        m_pdcpSapUser->ReceivePdcpSdu(params);
    }
```
is worked based on the sequence number.
In NS-3, the max sequence number of PDCP layer is 4095.
Therefore, we fixed this value and accorindg to the sequence number we have 4096 array space that store the history of sequence number in the variable
- pdcpSequencNumbers
There is one algorithm which will prevent initializing multiple times with same sequence number.


## Author ##
Sangwon Seo

## License ##

This software is licensed under the terms of the GNU GPLv2, as like as ns-3.
See the LICENSE file for more details.
