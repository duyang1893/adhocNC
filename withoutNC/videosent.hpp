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

namespace ns3 {

class Socket;
class Packet;

class VideoSent :public Application
{

public:
	static TypeId GetTypeId (void);

	VideoSent ();
	VideoSent (Ipv4Address ip, uint16_t port, char *traceFile);
	~VideoSent ();
	void SetRemote (Ipv4Address ip, uint16_t port);
	void SetRemote (Ipv6Address ip, uint16_t port);
	void SetRemote (Address ip, uint16_t port);
	void SetTraceFile (std::string filename);
	void SetLayer2flag (bool flag);
	uint16_t GetMaxPacketSize (void);
	void SetMaxPacketSize (uint16_t maxPacketSize);
	void SetVideoStat(uint32_t numfrm, double frmRate);

protected:
	virtual void DoDispose (void);

private:
	void LoadTrace (std::string filename);
	void LoadDefaultTrace (void);
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void Send (void);
	void SendPacket (uint32_t size);
	void readBuffer(void);
	struct TraceEntry
	{
		uint32_t frmid; //frame index
		uint32_t pktid; // packet index
		uint32_t packetSize; //!< Size of the frame
		double txTime; //transmit time in double seconds
		uint32_t layerid;  // layerid=0(base-layer) =1(layer-2)
	};
	uint32_t m_numfrm;
	double m_frmRate; 
	bool enable_layer2; 
	uint8_t m_numcliptx; 

	uint32_t m_sent; //!< Counter for sent packets
	Ptr<Socket> m_socket; //!< Socket
	Address m_peerAddress; //!< Remote peer address
	uint16_t m_peerPort; //!< Remote peer port
	EventId m_sendEvent; //!< Event to send the next packet

	std::vector<struct TraceEntry> m_entries; //!< Entries in the trace to send
	uint32_t m_currentEntry; //!< Current entry index
	static struct TraceEntry g_defaultEntries[]; //!< Default trace to send
	uint16_t m_maxPacketSize; //!< Maximum packet size to send (including the SeqTsHeader)
};


/**
 * \brief Default trace to send
 */
struct VideoSent::TraceEntry VideoSent::g_defaultEntries[] = {
{1, 2, 9, 1, 0},
{1, 3, 1402, 1, 0},
{1, 4, 9, 1, 0},
{1, 5, 308, 1, 0},
{1, 6, 9, 1, 0},
{1, 7, 1277, 1, 0},
{1, 8, 9, 1, 0},
{2, 9, 973, 1, 0},
{2, 10, 9, 1, 0},
{2, 11, 1376, 1, 0},
{2, 12, 9, 1, 0}
};

TypeId
VideoSent::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::VideoSent")
	.SetParent<Application> ()
	.AddConstructor<VideoSent> ()
	.AddAttribute ("RemoteAddress",
		   "The destination Address of the outbound packets",
		   AddressValue (),
		   MakeAddressAccessor (&VideoSent::m_peerAddress),
		   MakeAddressChecker ())
	.AddAttribute ("RemotePort",
		   "The destination port of the outbound packets",
		   UintegerValue (100),
		   MakeUintegerAccessor (&VideoSent::m_peerPort),
		   MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("MaxPacketSize",
		   "The maximum size of a packet (including the SeqTsHeader, 12 bytes).",
		   UintegerValue (1024),
		   MakeUintegerAccessor (&VideoSent::m_maxPacketSize),
		   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("TraceFilename",
		   "Name of file to load a trace from. By default, uses a hardcoded trace.",
		   StringValue (""),
		   MakeStringAccessor (&VideoSent::SetTraceFile),
		   MakeStringChecker ())

	;
	return tid;
}

VideoSent::VideoSent ()
{
	NS_LOG_FUNCTION (this);
	m_sent = 0;
	m_socket = 0;
	m_sendEvent = EventId ();
	m_maxPacketSize = 1400;
	m_numfrm = 0;
	m_frmRate = 60.0;
	enable_layer2 = true;
	m_numcliptx = 0;
}

VideoSent::VideoSent (Ipv4Address ip, uint16_t port,char *traceFile)
{
	NS_LOG_FUNCTION (this);
	m_sent = 0;
	m_socket = 0;
	m_sendEvent = EventId ();
	m_peerAddress = ip;
	m_peerPort = port;
	m_currentEntry = 0;
	m_maxPacketSize = 1400;
	m_numfrm = 0;
	m_frmRate = 60.0;
	enable_layer2 = true;
	m_numcliptx = 0;
	if (traceFile != NULL)
	{
		SetTraceFile (traceFile);
	}
}

VideoSent::~VideoSent ()
{
	NS_LOG_FUNCTION (this);
	m_entries.clear ();
}

void
VideoSent::SetRemote (Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	m_entries.clear ();
	m_peerAddress = ip;
	m_peerPort = port;
}

void
VideoSent::SetRemote (Ipv4Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	m_entries.clear ();
	m_peerAddress = Address (ip);
	m_peerPort = port;
}

void
VideoSent::SetRemote (Ipv6Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	m_entries.clear ();
	m_peerAddress = Address (ip);
	m_peerPort = port;
}

void
VideoSent::SetTraceFile (std::string traceFile)
{
	NS_LOG_FUNCTION (this << traceFile);
	if (traceFile == "")
	{
		LoadDefaultTrace ();
	}
	else
	{
		LoadTrace (traceFile);
	}
}

void
VideoSent::SetLayer2flag (bool flag)
{
	enable_layer2 = flag; 
}

void
VideoSent::SetMaxPacketSize (uint16_t maxPacketSize)
{
	NS_LOG_FUNCTION (this << maxPacketSize);
	m_maxPacketSize = maxPacketSize;
}


uint16_t VideoSent::GetMaxPacketSize (void)
{
	NS_LOG_FUNCTION (this);
	return m_maxPacketSize;
}

void
VideoSent::SetVideoStat(uint32_t numfrm, double frmRate)
{
	m_numfrm=numfrm;
	m_frmRate = frmRate;
}

void
VideoSent::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}

void
VideoSent::LoadTrace (std::string filename)
{
	NS_LOG_FUNCTION (this << filename);
	uint32_t  frmid, pktid, size, layerid; double txTime;
	TraceEntry entry;
	std::ifstream ifTraceFile;
	ifTraceFile.open (filename.c_str (), std::ifstream::in);
	m_entries.clear ();
	if (!ifTraceFile.good ())
	{
		LoadDefaultTrace ();
	}
	while (ifTraceFile.good ())
	{
		ifTraceFile >>  frmid >> pktid >> size >> txTime >>layerid;
		entry.frmid = frmid;
		entry.pktid = pktid;
		entry.packetSize = size;
		entry.txTime = txTime;
		entry.layerid = layerid;
		m_entries.push_back (entry);
	}
	ifTraceFile.close ();
	m_currentEntry = 0;
	m_entries.pop_back();
}

void
VideoSent::LoadDefaultTrace (void)
{
	NS_LOG_FUNCTION (this);
	for (uint32_t i = 0; i < (sizeof (g_defaultEntries) / sizeof (struct TraceEntry)); i++)
	{
		struct TraceEntry entry = g_defaultEntries[i];
		m_entries.push_back (entry);
	}
	m_currentEntry = 0;
}

void
VideoSent::StartApplication (void)
{
	NS_LOG_FUNCTION (this);

	if (m_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
	if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
	{
		m_socket->Bind ();
		m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
	}
	else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
	{
		m_socket->Bind6 ();
		m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
	}
	}
	m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	m_sendEvent = Simulator::Schedule (Seconds (0.0), &VideoSent::Send, this);
}

void
VideoSent::StopApplication ()
{
	NS_LOG_FUNCTION (this);
	Simulator::Cancel (m_sendEvent);
}

void
VideoSent::SendPacket (uint32_t size)
{
	NS_LOG_FUNCTION (this << size);
	Ptr<Packet> p;
	uint32_t packetSize;
	if (size>12)
	{
		packetSize = size - 12; // 12 is the size of the SeqTsHeader
	}
	else
	{
		packetSize = 0;
	}
	struct TraceEntry *entry = &m_entries[m_currentEntry];

	p = Create<Packet> (packetSize);
	SeqTsHeader seqTs;
	//  seqTs.SetSeq (m_sent);
	seqTs.SetSeq(entry->pktid*10+m_numcliptx);
	p->AddHeader (seqTs);

	std::stringstream addressString;
	if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
	{
		addressString << Ipv4Address::ConvertFrom (m_peerAddress);
	}
	else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
	{
		addressString << Ipv6Address::ConvertFrom (m_peerAddress);
	}
	else
	{
		addressString << m_peerAddress;
	}

	if ((m_socket->Send (p)) >= 0)
	{
		++m_sent;
		NS_LOG_INFO ("Sent " << size << " bytes to " << addressString.str () << " SeqNO= " <<m_sent);
	}
	else
	{
		NS_LOG_INFO ("Error while sending " << size << " bytes to " << addressString.str ());
	}
}

void
VideoSent::Send (void)
{
	NS_LOG_FUNCTION (this);
	NS_ASSERT (m_sendEvent.IsExpired ());

	struct TraceEntry *entry = &m_entries[m_currentEntry];  

	if (m_currentEntry==0)
	{
		readBuffer();
	}
	else 
	{
		if (entry->frmid!=m_entries[m_currentEntry-1].frmid)
		{
			readBuffer();
		}
	}

	uint32_t pktlen = entry->packetSize;
	for (uint32_t i = 0; i < entry->packetSize / m_maxPacketSize; i++)
	{
		SendPacket(m_maxPacketSize);
		pktlen=pktlen+12;
	}

	uint16_t sizetosend = pktlen % m_maxPacketSize;
	if (sizetosend>0)
	{
		sizetosend = sizetosend+12;
		SendPacket(sizetosend);
	}

	m_currentEntry++;
	if (m_currentEntry==m_entries.size())
	{
		m_numcliptx++;
		m_currentEntry=0;
	}      
	m_sendEvent = Simulator::Schedule (Seconds(entry->txTime), &VideoSent::Send, this);
}

void
VideoSent::readBuffer(void)
{
	uint32_t numPkt=0; uint32_t numByte=0;
	uint32_t currentFrmID = m_entries[m_currentEntry].frmid;
	do
	{
		numByte=numByte+m_entries[m_currentEntry+numPkt].packetSize;
		numPkt++;
		
	}while(m_entries[m_currentEntry+numPkt].frmid==currentFrmID);
 
	
	double pktInterval = 1.0/m_frmRate/numPkt;
	//double microSpByte = 1000000.0/m_frmRate/numByte;
	for (uint32_t i=0; i<numPkt; i++)
	{
		m_entries[m_currentEntry+i].txTime= pktInterval;
		//m_entries[m_currentEntry+i].txTime= m_entries[m_currentEntry+i].packetSize*microSpByte;
	}	

}


} // namespace ns3
