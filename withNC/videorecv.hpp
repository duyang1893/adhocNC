#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/config-store-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/stats-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <kodo/rlnc/full_rlnc_codes.hpp>
#include <kodo/trace.hpp>

namespace ns3 {

typedef kodo::full_rlnc_decoder<fifi::binary8,kodo::enable_trace> rlnc_decoder;

class VideoRecv : public Application
{
public:
	static TypeId GetTypeId (void);
	VideoRecv ();
	virtual ~VideoRecv ();
	uint32_t GetReceived (void) const;
	uint16_t GetPacketWindowSize () const;
	void SetPacketWindowSize (uint16_t size);
protected:
	virtual void DoDispose (void);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void HandleRead (Ptr<Socket> socket);
	void writeBuffer(Ptr<Packet> packet, uint32_t seqnum);

	uint16_t m_port; //!< Port on which we listen for incoming packets.
	Ptr<Socket> m_socket; //!< IPv4 Socket
	Ptr<Socket> m_socket6; //!< IPv6 Socket
	uint32_t m_received; //!< Number of received packets
	PacketLossCounter m_lossCounter; //!< Lost packet counter

	std::vector<uint8_t> data;
	std::vector<uint32_t> pktLenVector;
	std::vector<uint8_t> m_payload_buffer;
	uint32_t genSize; 
	uint32_t pktSize; 
  	rlnc_decoder::pointer m_decoder;
};

TypeId
VideoRecv::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::VideoRecv")
	.SetParent<Application> ()
	.AddConstructor<VideoRecv> ()
	.AddAttribute ("Port",
		   "Port on which we listen for incoming packets.",
		   UintegerValue (100),
		   MakeUintegerAccessor (&VideoRecv::m_port),
		   MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("PacketWindowSize",
		   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
		   UintegerValue (32),
		   MakeUintegerAccessor (&VideoRecv::GetPacketWindowSize,
		                         &VideoRecv::SetPacketWindowSize),
		   MakeUintegerChecker<uint16_t> (8,256))
	;
	return tid;
}

VideoRecv::VideoRecv (): m_lossCounter (0)
{
	NS_LOG_FUNCTION (this);
	m_received=0;
}

VideoRecv::~VideoRecv ()
{
	NS_LOG_FUNCTION (this);
}

uint16_t
VideoRecv::GetPacketWindowSize () const
{
	NS_LOG_FUNCTION (this);
	return m_lossCounter.GetBitMapSize ();
}

void
VideoRecv::SetPacketWindowSize (uint16_t size)
{
	NS_LOG_FUNCTION (this << size);
	m_lossCounter.SetBitMapSize (size);
}

uint32_t
VideoRecv::GetReceived (void) const
{
	NS_LOG_FUNCTION (this);
	return m_received;
}

void
VideoRecv::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}

void
VideoRecv::StartApplication (void)
{
	NS_LOG_FUNCTION (this);

	if (m_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (),m_port);
		m_socket->Bind (local);
	}

	m_socket->SetRecvCallback (MakeCallback (&VideoRecv::HandleRead, this));

	if (m_socket6 == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket6 = Socket::CreateSocket (GetNode (), tid);
		Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),m_port);
		m_socket6->Bind (local);
	}

	m_socket6->SetRecvCallback (MakeCallback (&VideoRecv::HandleRead, this));

}

void
VideoRecv::StopApplication ()
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
}

void
VideoRecv::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
		if (packet->GetSize () > 0)
		{
			SeqTsHeader seqTs;
			packet->RemoveHeader (seqTs);
			uint32_t currentSequenceNumber = seqTs.GetSeq ();
			if (InetSocketAddress::IsMatchingType (from))
			{
				NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
				   " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
				   " Sequence Number: " << currentSequenceNumber <<
				   " Uid: " << packet->GetUid () <<
				   " TXtime: " << seqTs.GetTs () <<
				   " RXtime: " << Simulator::Now () <<
				   " Delay: " << Simulator::Now () - seqTs.GetTs ());
			}
			else if (Inet6SocketAddress::IsMatchingType (from))
			{
				NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
				   " bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
				   " Sequence Number: " << currentSequenceNumber <<
				   " Uid: " << packet->GetUid () <<
				   " TXtime: " << seqTs.GetTs () <<
				   " RXtime: " << Simulator::Now () <<
				   " Delay: " << Simulator::Now () - seqTs.GetTs ());
			}

			m_lossCounter.NotifyReceived (currentSequenceNumber);
			m_received++;
			writeBuffer(packet, currentSequenceNumber);
		}
	}
}

void
VideoRecv::writeBuffer(Ptr<Packet> packet, uint32_t seqnum)
{
}

} // namespace ns3
