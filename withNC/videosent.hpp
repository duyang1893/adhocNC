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

class Socket;
class Packet;

typedef kodo::full_rlnc_encoder<fifi::binary8,kodo::disable_trace> rlnc_encoder;

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
	void SetOverhead (double percentage);

protected:
	virtual void DoDispose (void);

private:
	void LoadTrace (std::string filename);
	void LoadDefaultTrace (void);
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void Send (void);
	void SendPacket (uint16_t size);
	void readBuffer(void);
	struct TraceEntry
	{
		uint32_t frmid; //frame index
		uint32_t pktid; // packet index
		uint16_t packetSize; //!< Size of the frame
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

	std::vector<struct TraceEntry> m_buffer; //!< Entries in the trace to send
	uint32_t m_currentRead; //!< Current entry index
	double m_percentage; //percentage of overead;

	std::vector<uint8_t> frm_data;
	std::vector<uint32_t> pktLenVector;
	std::vector<uint8_t> m_payload_buffer;
	uint32_t genSize; 
	uint32_t pktSize; 
  	rlnc_encoder::pointer m_encoder;
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
	m_percentage=0.0;
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

	m_currentRead=0;
	m_percentage = 0.0;
}

VideoSent::~VideoSent ()
{
	NS_LOG_FUNCTION (this);
	m_entries.clear ();
	m_buffer.clear();
	frm_data.clear();
	pktLenVector.clear();
	m_payload_buffer.clear();
}

void
VideoSent::SetRemote (Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	m_entries.clear ();
	m_peerAddress = ip;
	m_peerPort = port;
	m_buffer.clear();
	frm_data.clear();
	pktLenVector.clear();
	m_payload_buffer.clear();
}

void
VideoSent::SetRemote (Ipv4Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	m_entries.clear ();
	m_peerAddress = Address (ip);
	m_peerPort = port;
	m_buffer.clear();
	frm_data.clear();
	pktLenVector.clear();
	m_payload_buffer.clear();
}

void
VideoSent::SetRemote (Ipv6Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	m_entries.clear ();
	m_peerAddress = Address (ip);
	m_peerPort = port;
	m_buffer.clear();
	frm_data.clear();
	pktLenVector.clear();
	m_payload_buffer.clear();
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
VideoSent:: SetOverhead (double percentage)
{
	m_percentage=percentage;
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
	m_buffer.clear();
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
		m_buffer.push_back(entry);
	}
	ifTraceFile.close ();
	m_currentEntry = 0;
	m_entries.pop_back();
	m_buffer.pop_back();
	m_currentRead=0;
}

void
VideoSent::LoadDefaultTrace (void)
{
	NS_LOG_FUNCTION (this);
	for (uint32_t i = 0; i < (sizeof (g_defaultEntries) / sizeof (struct TraceEntry)); i++)
	{
		struct TraceEntry entry = g_defaultEntries[i];
		m_entries.push_back (entry);
		m_buffer.push_back(entry);
	}
	m_currentEntry = 0;
	m_currentRead=0;
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
VideoSent::SendPacket (uint16_t size)
{
	NS_LOG_FUNCTION (this << size);
	struct TraceEntry *entry = &m_buffer[m_currentRead];
//std::cout<<"sent packet:"<<m_currentRead<<" "<<entry->pktid<<std::endl; 

	m_payload_buffer.clear();
	m_payload_buffer.resize(m_encoder->payload_size());
	m_encoder->encode(&m_payload_buffer[0]);

	uint32_t frmid = entry->frmid;
	m_payload_buffer.push_back(uint8_t(frmid%255));
	uint32_t tmp1=frmid/255;
  	for (uint32_t i=1; i<ceil(m_numfrm/255.0);i++)
	{	
		if (tmp1>0)
		{
			m_payload_buffer.push_back(uint8_t(255));
			tmp1--;
		}
		else
		{
			m_payload_buffer.push_back(uint8_t(0));
		}
	}		
	
if ((size!=entry->packetSize) || (size!=m_payload_buffer.size())	|| (entry->packetSize!=m_payload_buffer.size()))
{
	std::cout<<"not equal  "<<size<<"  "<<entry->packetSize<<"  "<<m_payload_buffer.size()<<"  "<<m_encoder->payload_size()<<std::endl; 
}
	Ptr<Packet> p;
	p = Create<Packet> (&m_payload_buffer[0],size);
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

	struct TraceEntry *entry = &m_buffer[m_currentRead];  

	if (m_currentRead==0)
	{
		readBuffer();
	}
	else 
	{
		if (entry->frmid!=m_buffer[m_currentRead-1].frmid)
		{
			readBuffer();
		}
	}
//std::cout<<"enter Send"<<std::endl; 
//std::cout<<" and sent packet:"<<m_currentRead<<" "<<m_buffer.size()<<std::endl;
	entry = &m_buffer[m_currentRead];  
	SendPacket(entry->packetSize);
	m_currentRead++;
	if (m_currentRead==m_buffer.size())
	{
		m_numcliptx++;
		m_currentRead=0;
	}      
	m_sendEvent = Simulator::Schedule (Seconds(entry->txTime), &VideoSent::Send, this);
}

void
VideoSent::readBuffer(void)
{
std::cout<<"m_currentEntry="<<m_currentEntry<<" m_currentRead=="<<m_currentRead<<std::endl;
	uint16_t tmpStartId;
	uint32_t numPkt=0; 
	uint32_t currentFrmID = m_entries[m_currentEntry].frmid;
	uint16_t pktSize = m_entries[m_currentEntry].packetSize;
	
	if (m_currentEntry==0)
	{
		m_buffer.clear();
		tmpStartId=0;
	}
	else
	{
		m_buffer.pop_back();
		tmpStartId = m_buffer.size();
	}
std::cout<<"tmpStartID=="<<tmpStartId<<std::endl;
	pktLenVector.clear();
	TraceEntry entry;
	do
	{
		uint16_t pktlen = m_entries[m_currentEntry].packetSize;
		for (uint32_t i = 0; i < pktlen/ m_maxPacketSize; i++)
		{
			entry.frmid = m_entries[m_currentEntry].frmid;
			entry.pktid = m_entries[m_currentEntry].pktid;
			entry.packetSize = m_maxPacketSize;
			entry.layerid = m_entries[m_currentEntry].layerid;
			m_buffer.push_back(entry);
			pktLenVector.push_back(m_currentEntry);

			if (pktSize< m_maxPacketSize)
			{
				pktSize= m_maxPacketSize;
			}
			numPkt++;
		}

		uint16_t sizetosend = pktlen % m_maxPacketSize;
		if (sizetosend>0)
		{
			entry.frmid = m_entries[m_currentEntry].frmid;
			entry.pktid = m_entries[m_currentEntry].pktid;
			entry.packetSize = sizetosend;
			entry.layerid = m_entries[m_currentEntry].layerid;
			m_buffer.push_back(entry);	
			pktLenVector.push_back(sizetosend);

			if (pktSize< m_maxPacketSize)
			{
				pktSize= m_maxPacketSize;
			}

			numPkt++;
		}
		m_currentEntry=(m_currentEntry+1)%m_entries.size();
		
	}while(m_entries[m_currentEntry].frmid==currentFrmID);

	genSize=numPkt;
	rlnc_encoder::factory encoder_factory(genSize, pktSize);
	m_encoder=encoder_factory.build();	

	frm_data.resize(m_encoder->block_size());
	std::generate_n(begin(frm_data), frm_data.size(), rand);
	
	m_encoder->set_symbols(sak::storage(frm_data));
	m_encoder->set_systematic_off();
	m_encoder->seed(time(0));

	uint32_t pktSizeNC = m_encoder->payload_size()+ceil(m_numfrm/255.0);

std::cout<<"data size="<<frm_data.size()<<" genSize="<<genSize<<" pktSize="<<pktSize<<" payloadsize="<<m_encoder->payload_size()<<" pktNC="<<std::endl;


	uint32_t numTxPkt = ceil(numPkt*(1+m_percentage));
std::cout<<"numPkt="<<numPkt<<" numTxPkt="<<numTxPkt<<std::endl;

	double pktInterval = 1.0/m_frmRate/numTxPkt;
	for (uint32_t i=0; i<numPkt; i++)
	{
		m_buffer[tmpStartId+i].txTime=pktInterval;
		m_buffer[tmpStartId+i].packetSize=pktSizeNC;
	}
	for (uint32_t i=0; i<numTxPkt-numPkt; i++)
	{
		entry.frmid = m_buffer[tmpStartId+numPkt-1].frmid;
		entry.pktid = m_buffer[tmpStartId+numPkt-1].pktid;
		//entry.packetSize = m_buffer[tmpStartId+numPkt-1].packetSize;
		entry.packetSize=pktSizeNC;
		entry.txTime = pktInterval;
		entry.layerid = m_buffer[tmpStartId+numPkt-1].layerid;
		m_buffer.push_back(entry);
std::cout<<"add redudent="<<entry.pktid<<std::endl;
	}

	if(m_currentEntry!=0)
	{
		entry.frmid = m_entries[m_currentEntry].frmid;
		entry.pktid = m_entries[m_currentEntry].pktid;
		entry.packetSize = m_entries[m_currentEntry].packetSize;
		entry.txTime = pktInterval;
		entry.layerid = m_entries[m_currentEntry].layerid;
		m_buffer.push_back(entry);
std::cout<<"add nextfrm" <<entry.pktid<<std::endl;
	}	

std::cout<<"m_currentEntry="<<m_currentEntry<<" m_currentRead=="<<m_currentRead<<" buffersize="<<m_buffer.size()<<std::endl<<std::endl;

}


} // namespace ns3
