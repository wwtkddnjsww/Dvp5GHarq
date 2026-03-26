/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "udp-client.h"

#include "seq-ts-header.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include <ns3/boolean.h>

#include <cstdio>
#include <cstdlib>
#include <random>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpClient");

NS_OBJECT_ENSURE_REGISTERED(UdpClient);

TypeId
UdpClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpClient")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<UdpClient>()
            .AddAttribute(
                "MaxPackets",
                "The maximum number of packets the application will send (zero means infinite)",
                UintegerValue(100),
                MakeUintegerAccessor(&UdpClient::m_count),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("Interval",
                          "The time to wait between packets",
                          TimeValue(Seconds(1.0)),
                          MakeTimeAccessor(&UdpClient::m_interval),
                          MakeTimeChecker())
            .AddAttribute("StochasticInterval",
                          "Random function is activated for stochastic interval",
                          BooleanValue(false),
                          MakeBooleanAccessor(&UdpClient::m_stochasticInterval),
                          MakeBooleanChecker())
            .AddAttribute("RemoteAddress",
                          "The destination Address of the outbound packets",
                          AddressValue(),
                          MakeAddressAccessor(&UdpClient::m_peerAddress),
                          MakeAddressChecker())
            .AddAttribute("RemotePort",
                          "The destination port of the outbound packets",
                          UintegerValue(100),
                          MakeUintegerAccessor(&UdpClient::m_peerPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("PacketSize",
                          "Size of packets generated. The minimum packet size is 12 bytes which is "
                          "the size of the header carrying the sequence number and the time stamp.",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&UdpClient::m_size),
                          MakeUintegerChecker<uint32_t>(12, 65507))
            .AddTraceSource("Tx",
                            "A new packet is created and sent",
                            MakeTraceSourceAccessor(&UdpClient::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithAddresses",
                            "A new packet is created and sent",
                            MakeTraceSourceAccessor(&UdpClient::m_txTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

UdpClient::UdpClient()
{
    NS_LOG_FUNCTION(this);
    m_sent = 0;
    m_totalTx = 0;
    m_socket = nullptr;
    m_sendEvent = EventId();
}

UdpClient::~UdpClient()
{
    NS_LOG_FUNCTION(this);
}

void
UdpClient::SetRemote(Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    m_peerAddress = ip;
    m_peerPort = port;
}

void
UdpClient::SetRemote(Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_peerAddress = addr;
}


void
UdpClient::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
UdpClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        if (Ipv4Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(
                InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (Ipv6Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(
                Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(m_peerAddress);
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(m_peerAddress);
        }
        else
        {
            NS_ASSERT_MSG(false, "Incompatible address type: " << m_peerAddress);
        }
    }

#ifdef NS3_LOG_ENABLE
    std::stringstream peerAddressStringStream;
    if (Ipv4Address::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << Ipv4Address::ConvertFrom(m_peerAddress);
    }
    else if (Ipv6Address::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << Ipv6Address::ConvertFrom(m_peerAddress);
    }
    else if (InetSocketAddress::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << InetSocketAddress::ConvertFrom(m_peerAddress).GetIpv4();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << Inet6SocketAddress::ConvertFrom(m_peerAddress).GetIpv6();
    }
    m_peerAddressString = peerAddressStringStream.str();
#endif // NS3_LOG_ENABLE

    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    m_socket->SetAllowBroadcast(true);
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &UdpClient::Send, this);
}

void
UdpClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_sendEvent);
}


#include <random>   // 파일 상단에 추가 (udp-client.cc)

void
UdpClient::Send()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_sendEvent.IsExpired());

    // m_interval(밀리초)을 확률 분모 N으로 사용: p = 1/N
    uint32_t N = static_cast<uint32_t>(std::max<int64_t>(1, m_interval.GetMilliSeconds()));
    double pSend = 1.0 / static_cast<double>(N);

    // Send() 안에서만 쓰는 RNG (한 번만 시드, 이후 재사용)
    static std::mt19937 gen(12345); // 재현성 원하면 고정 시드, 매번 다르게면 random_device로 초기화
    static std::uniform_real_distribution<double> dist(0.0, 1.0);

    double u = dist(gen);

    // "실시간" 출력
    // std::cout << Simulator::Now().GetSeconds()
    //           << "\tu=" << u
    //           << "\tp=" << pSend
    //           << "\t" << (u < pSend ? "SEND" : "SKIP")
    //           << std::endl;

    bool doSend = (u < pSend);

    if (doSend)
    {
        Address from;
        Address to;
        m_socket->GetSockName(from);
        m_socket->GetPeerName(to);

        SeqTsHeader seqTs;
        seqTs.SetSeq(m_sent);
        NS_ABORT_IF(m_size < seqTs.GetSerializedSize());
        Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize());

        // Trace before adding header, for consistency with PacketSink
        m_txTrace(p);
        m_txTraceWithAddresses(p, from, to);

        p->AddHeader(seqTs);

        if ((m_socket->Send(p)) >= 0)
        {
            ++m_sent;
            m_totalTx += p->GetSize();
        }
    }

    // 매 1ms마다 다시 호출해서 다시 결정
    if (m_sent < m_count || m_count == 0)
    {
        m_sendEvent = Simulator::Schedule(MilliSeconds(1), &UdpClient::Send, this);
    }
}
// void
// UdpClient::Send()
// {
//     NS_LOG_FUNCTION(this);
//     NS_ASSERT(m_sendEvent.IsExpired());

//     Address from;
//     Address to;
//     m_socket->GetSockName(from);
//     m_socket->GetPeerName(to);
//     SeqTsHeader seqTs;
//     seqTs.SetSeq(m_sent);
//     NS_ABORT_IF(m_size < seqTs.GetSerializedSize());
//     Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize());

//     // Trace before adding header, for consistency with PacketSink
//     m_txTrace(p);
//     m_txTraceWithAddresses(p, from, to);

//     p->AddHeader(seqTs);

//     if ((m_socket->Send(p)) >= 0)
//     {
//         ++m_sent;
//         m_totalTx += p->GetSize();
// #ifdef NS3_LOG_ENABLE
//         NS_LOG_INFO("TraceDelay TX " << m_size << " bytes to " << m_peerAddressString << " Uid: "
//                                      << p->GetUid() << " Time: " << (Simulator::Now()).As(Time::S));
// #endif // NS3_LOG_ENABLE
//     }
// #ifdef NS3_LOG_ENABLE
//     else
//     {
//         NS_LOG_INFO("Error while sending " << m_size << " bytes to " << m_peerAddressString);
//     }
// #endif // NS3_LOG_ENABLE

//     if (m_sent < m_count || m_count == 0)
//     {
//         if(m_stochasticInterval == true){
//             std::random_device rd{};
//             std::mt19937 gen{rd()};
//             std::normal_distribution<float> d{0.0, 1000.0}; //1 frame length
//             Time stochasticT = MicroSeconds(d(gen));
//             m_sendEvent = Simulator::Schedule(m_interval+stochasticT, &UdpClient::Send, this);
//         }
//         else{
//             m_sendEvent = Simulator::Schedule(m_interval, &UdpClient::Send, this);
//         }
//     }
// }

uint64_t
UdpClient::GetTotalTx() const
{
    return m_totalTx;
}

} // Namespace ns3
