/*
 * Copyright (c) 2011-2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "lte-pdcp.h"

#include "lte-pdcp-header.h"
#include "lte-pdcp-sap.h"
#include "lte-pdcp-tag.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include <iostream>
#include <algorithm>

namespace ns3
{
// This variable is used for classifying same sequence number in different carriers.
// Therefore, it is difficult to use more than 1 stream per UE.
static uint8_t pdcpSequenceNumbers[100][5000] = {0}; //changed by Sangwon [RNTI][PACKET SN]
NS_LOG_COMPONENT_DEFINE("LtePdcp");

/// LtePdcpSpecificLteRlcSapUser class
class LtePdcpSpecificLteRlcSapUser : public LteRlcSapUser
{
  public:
    /**
     * Constructor
     *
     * \param pdcp PDCP
     */
    LtePdcpSpecificLteRlcSapUser(LtePdcp* pdcp);

    // Interface provided to lower RLC entity (implemented from LteRlcSapUser)
    void ReceivePdcpPdu(Ptr<Packet> p) override;

  private:
    LtePdcpSpecificLteRlcSapUser();
    LtePdcp* m_pdcp; ///< the PDCP
};

LtePdcpSpecificLteRlcSapUser::LtePdcpSpecificLteRlcSapUser(LtePdcp* pdcp)
    : m_pdcp(pdcp)
{
}

LtePdcpSpecificLteRlcSapUser::LtePdcpSpecificLteRlcSapUser()
{
}

void
LtePdcpSpecificLteRlcSapUser::ReceivePdcpPdu(Ptr<Packet> p)
{
    m_pdcp->DoReceivePdu(p);
}

///////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(LtePdcp);

LtePdcp::LtePdcp()
    : m_pdcpSapUser(nullptr),
      m_rlcSapProvider(nullptr),
      m_rnti(0),
      m_lcid(0),
      m_txSequenceNumber(0),
      m_rxSequenceNumber(0)
{
    NS_LOG_FUNCTION(this);
    m_pdcpSapProvider = new LtePdcpSpecificLtePdcpSapProvider<LtePdcp>(this);
    m_rlcSapUser = new LtePdcpSpecificLteRlcSapUser(this);
}

LtePdcp::~LtePdcp()
{
    NS_LOG_FUNCTION(this);
}

TypeId
LtePdcp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LtePdcp")
                            .SetParent<Object>()
                            .SetGroupName("Lte")
                            .AddAttribute("MaxDuplication",
                                          "The number of duplicated packets",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&LtePdcp::m_maxDup),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("FixedDuplication",
                                          "If true, it is fixed number of duplications, otherwise, adaptive duplications",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&LtePdcp::m_isFixedDuplication),
                                          MakeBooleanChecker())
                            .AddAttribute("UrllcUes",
                                          "The number of URLLC UEs",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&LtePdcp::m_urllcUes),
                                          MakeUintegerChecker<uint8_t>())
                            .AddAttribute("DuplicatedPacketDelay",
                                          "Delay of duplicated packets in microseconds",
                                          UintegerValue(500),
                                          MakeUintegerAccessor(&LtePdcp::m_dupDelay),
                                          MakeUintegerChecker<uint16_t>())
                            .AddTraceSource("TxPDU",
                                            "PDU transmission notified to the RLC.",
                                            MakeTraceSourceAccessor(&LtePdcp::m_txPdu),
                                            "ns3::LtePdcp::PduTxTracedCallback")
                            .AddTraceSource("RxPDU",
                                            "PDU received.",
                                            MakeTraceSourceAccessor(&LtePdcp::m_rxPdu),
                                            "ns3::LtePdcp::PduRxTracedCallback");
    return tid;
}

void
LtePdcp::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete (m_pdcpSapProvider);
    delete (m_rlcSapUser);
}

void
LtePdcp::SetRnti(uint16_t rnti)
{
    NS_LOG_FUNCTION(this << (uint32_t)rnti);
    m_rnti = rnti;
}

void
LtePdcp::SetLcId(uint8_t lcId)
{
    NS_LOG_FUNCTION(this << (uint32_t)lcId);
    m_lcid = lcId;
}

void
LtePdcp::SetLtePdcpSapUser(LtePdcpSapUser* s)
{
    NS_LOG_FUNCTION(this << s);
    m_pdcpSapUser = s;
}

LtePdcpSapProvider*
LtePdcp::GetLtePdcpSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_pdcpSapProvider;
}

void
LtePdcp::SetLteRlcSapProvider(LteRlcSapProvider* s)
{
    NS_LOG_FUNCTION(this << s);
    m_rlcSapProvider = s;
}

LteRlcSapUser*
LtePdcp::GetLteRlcSapUser()
{
    NS_LOG_FUNCTION(this);
    return m_rlcSapUser;
}

LtePdcp::Status
LtePdcp::GetStatus() const
{
    Status s;
    s.txSn = m_txSequenceNumber;
    s.rxSn = m_rxSequenceNumber;
    return s;
}

void
LtePdcp::SetStatus(Status s)
{
    m_txSequenceNumber = s.txSn;
    m_rxSequenceNumber = s.rxSn;
}

void LtePdcp::SetNumberOfDuplications(uint8_t dupNum)
{
    m_numberOfDuplications = dupNum;
}

void LtePdcp::SetDuplicationDelay(uint16_t dupDelay)
{
    m_dupDelay = dupDelay;
}


uint8_t LtePdcp::GetDuplicationDelay()
{
    return m_dupDelay;
}

uint8_t LtePdcp::GetMaxNumberOfDuplications()
{
    return m_maxDup;
}

void LtePdcp::SetUrllcUes(uint8_t v)
{
    m_urllcUes = v;
}

uint8_t LtePdcp::GetUrllcUes()
{
    return m_urllcUes;
}


bool LtePdcp::IsFixedDuplication()
{
    return m_isFixedDuplication;
}

////////////////////////////////////////

void
LtePdcp::DoTransmitPdcpSdu(LtePdcpSapProvider::TransmitPdcpSduParameters params)
{
    if (m_isFixedDuplication)
    {
        m_numberOfDuplications = m_maxDup;
    }
    if (m_numberOfDuplications > m_maxDup){
        m_numberOfDuplications = m_maxDup;
    }
    NS_LOG_INFO(this << m_rnti << static_cast<uint16_t>(m_lcid) << params.pdcpSdu->GetSize());
    Ptr<Packet> p = Copy(params.pdcpSdu);
    // Sender timestamp
    PdcpTag pdcpTag(Simulator::Now());
    pdcpTag.SetSequenceNumber(m_txSequenceNumber);
    LtePdcpHeader pdcpHeader;
    pdcpHeader.SetSequenceNumber(m_txSequenceNumber);
    pdcpHeader.SetDcBit(LtePdcpHeader::DATA_PDU);
    p->AddHeader(pdcpHeader);
    p->AddByteTag(pdcpTag, 1, pdcpHeader.GetSerializedSize());

    m_txPdu(m_rnti, m_lcid, p->GetSize());

    LteRlcSapProvider::TransmitPdcpPduParameters txParams;
    txParams.rnti = m_rnti;
    txParams.lcid = m_lcid;
    txParams.pdcpPdu = p;
    // std::cout << "Sender: cur time: " << Simulator::Now() << " pdcp dup Tag: "<< pdcpHeader.GetSequenceNumber() <<std::endl;
        
    
    // if (params.imsi <= m_urllcUes){ // Duplication is only considered by URLLC UEs
        for(uint8_t i = 0; i < m_numberOfDuplications; i++){
            Ptr<Packet> p_dup = Copy(params.pdcpSdu);
            PdcpTag pdcpTag_dup(Simulator::Now());
            pdcpTag_dup.SetSequenceNumber(m_txSequenceNumber);
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
            Simulator::Schedule(MicroSeconds(m_dupDelay*(i+1)),&LteRlcSapProvider::TransmitPdcpPdu,m_rlcSapProvider,txParams_dup);// changed by sangwon
        }
    // }
    

    NS_LOG_INFO("Transmitting PDCP PDU with header: " << txParams.pdcpPdu);
    m_rlcSapProvider->TransmitPdcpPdu(txParams);
    m_txSequenceNumber++;
    if (m_txSequenceNumber > m_maxPdcpSn)
    {
        m_txSequenceNumber = 1;

    }
    // m_rlcSapProvider->TransmitPdcpPdu(txParams);    
}

void
LtePdcp::DoReceivePdu(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << m_rnti << (uint32_t)m_lcid << p->GetSize());
    // Receiver timestamp
    PdcpTag pdcpTag;
    Time delay;
    // finding packet
    p->FindFirstMatchingByteTag(pdcpTag);
    // set delay after finding the packets
    delay = Simulator::Now() - pdcpTag.GetSenderTimestamp();
    LtePdcpHeader pdcpHeader;
    p->RemoveHeader(pdcpHeader);

    // std::cout<< "Without condition" <<"Time: "<< Simulator::Now() << " Delay: " << delay << " Timestamp: " << pdcpTag.GetSenderTimestamp() << " SeqNum: " << pdcpHeader.GetSequenceNumber() << std::endl; 
    
    NS_LOG_LOGIC("PDCP header: " << pdcpHeader);
    if (pdcpSequenceNumbers[m_rnti][pdcpHeader.GetSequenceNumber()] == 1){
        NS_LOG_INFO("Duplicated packet Sequence Number: "<<pdcpHeader.GetSequenceNumber());
        // accumulated_dup_vals++;
        // NS_LOG_INFO("# of duplicated packets: "<<accumulated_dup_vals);
        // std::cout<< "condition 1" << std::endl;
    }
    else if ((m_maxSeqNotifier == true) && (pdcpHeader.GetSequenceNumber() >= m_rxSequenceNumber+4090) && (pdcpSequenceNumbers[m_rnti][pdcpHeader.GetSequenceNumber()]==1)){
        // This else if is for the last part of duplicated packet sequences.
        // std::cout<< "condition 2" << std::endl;
    }
    else{
        NS_LOG_INFO("First arrived packet sequence number: "<<pdcpHeader.GetSequenceNumber());
        if (delay.GetMilliSeconds() < 100){
            m_rxPdu(m_rnti, m_lcid, p->GetSize(), delay.GetNanoSeconds()); // temporal move to check result.
            std::cout<< "Time: "<< Simulator::Now() << " Delay: " << delay << " Timestamp: " << pdcpTag.GetSenderTimestamp() << " SeqNum: " << pdcpHeader.GetSequenceNumber() << std::endl; 
        }
        
        pdcpSequenceNumbers[m_rnti][pdcpHeader.GetSequenceNumber()] = 1;
        
        
        // m_rxSequenceNumber = pdcpHeader.GetSequenceNumber()+1;
        
        // accumulated_seq_vals++;
        // if (accumulated_seq_vals == m_max_dup*MAX_PDCP_SN){
        // }
        if ((pdcpHeader.GetSequenceNumber()+1 >= m_maxPdcpSn) || (pdcpHeader.GetSequenceNumber()+1 < m_rxSequenceNumber))
        {
            m_maxSeqNotifier = true;
            m_rxSequenceNumber = 0;
            std::fill(pdcpSequenceNumbers[m_rnti],pdcpSequenceNumbers[m_rnti]+4000,0);
            // accumulated_seq_vals = 0;
        }
        else{
            m_rxSequenceNumber = pdcpHeader.GetSequenceNumber()+1;
        }

        if ((m_rxSequenceNumber > 2000) && (m_rxSequenceNumber < 3000) && (m_maxSeqNotifier == true)){
            m_maxSeqNotifier = false; // initialize the Max seq notifier when the enough number of sequence number is reached.
            std::fill(pdcpSequenceNumbers[m_rnti]+4000,pdcpSequenceNumbers[m_rnti]+4900,0);
        }
        LtePdcpSapUser::ReceivePdcpSduParameters params;
        params.pdcpSdu = p;
        params.rnti = m_rnti;
        params.lcid = m_lcid;
        if (delay.GetMilliSeconds() < 100){
            m_pdcpSapUser->ReceivePdcpSdu(params);
        }
        
    }


}

} // namespace ns3
