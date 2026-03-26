/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ofdma-edf.h"

#include "nr-mac-scheduler-ue-info-edf.h"

#include <ns3/double.h>
#include <ns3/log.h>

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaEdf");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaEdf);

TypeId
NrMacSchedulerOfdmaEdf::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrMacSchedulerOfdmaEdf")
            .SetParent<NrMacSchedulerOfdmaRR>()
            .AddConstructor<NrMacSchedulerOfdmaEdf>()
            .AddAttribute("FairnessIndex",
                          "Value (between 0 and 1) that defines the PF metric (1 is the "
                          "traditional 3GPP PF, 0 is RR in throughput",
                          DoubleValue(0),
                          MakeDoubleAccessor(&NrMacSchedulerOfdmaEdf::SetFairnessIndex,
                                             &NrMacSchedulerOfdmaEdf::GetFairnessIndex),
                          MakeDoubleChecker<float>(0, 1))
            .AddAttribute(
                "LastAvgTPutWeight",
                "Weight of the last average throughput in the average throughput calculation",
                DoubleValue(99),
                MakeDoubleAccessor(&NrMacSchedulerOfdmaEdf::SetTimeWindow,
                                   &NrMacSchedulerOfdmaEdf::GetTimeWindow),
                MakeDoubleChecker<float>(0));
    return tid;
}

NrMacSchedulerOfdmaEdf::NrMacSchedulerOfdmaEdf()
    : NrMacSchedulerOfdmaRR()
{
}

void
NrMacSchedulerOfdmaEdf::SetFairnessIndex(double v)
{
    NS_LOG_FUNCTION(this);
    m_alpha = v;
}

double
NrMacSchedulerOfdmaEdf::GetFairnessIndex() const
{
    NS_LOG_FUNCTION(this);
    return m_alpha;
}

void
NrMacSchedulerOfdmaEdf::SetTimeWindow(double v)
{
    NS_LOG_FUNCTION(this);
    m_timeWindow = v;
}

double
NrMacSchedulerOfdmaEdf::GetTimeWindow() const
{
    NS_LOG_FUNCTION(this);
    return m_timeWindow;
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerOfdmaEdf::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoEdf>(
        m_alpha,
        params.m_rnti,
        params.m_beamConfId,
        std::bind(&NrMacSchedulerOfdmaEdf::GetNumRbPerRbg, this));
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaEdf::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoEdf::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaEdf::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoEdf::CompareUeWeightsUl;
}

void
NrMacSchedulerOfdmaEdf::AssignedDlResources(const UePtrAndBufferReq& ue,
                                            [[maybe_unused]] const FTResources& assigned,
                                            const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoEdf>(ue.first);
    uePtr->UpdateDlQosMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerOfdmaEdf::NotAssignedDlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& assigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoEdf>(ue.first);
    uePtr->UpdateDlQosMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerOfdmaEdf::AssignedUlResources(const UePtrAndBufferReq& ue,
                                            [[maybe_unused]] const FTResources& assigned,
                                            const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoEdf>(ue.first);
    uePtr->UpdateUlQosMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerOfdmaEdf::NotAssignedUlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& assigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoEdf>(ue.first);
    uePtr->UpdateUlQosMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerOfdmaEdf::BeforeDlSched(const UePtrAndBufferReq& ue,
                                      const FTResources& assignableInIteration) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoEdf>(ue.first);
    uePtr->CalculatePotentialTPutDl(assignableInIteration, m_dlAmc);
}

void
NrMacSchedulerOfdmaEdf::BeforeUlSched(const UePtrAndBufferReq& ue,
                                      const FTResources& assignableInIteration) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoEdf>(ue.first);
    uePtr->CalculatePotentialTPutUl(assignableInIteration, m_ulAmc);
}

} // namespace ns3
