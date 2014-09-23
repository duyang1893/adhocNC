#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/stats-module.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/olsr-helper.h"
#include "ns3/config-store-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

#include "videosent.hpp"
#include "videorecv.hpp"

using namespace ns3;
void Ipv4TxRx(FILE *p,uint16_t targetport, Ptr<const Packet> packet, Ptr<Ipv4> ipv4,  uint32_t ifaceIndex)
{
	Ptr<Packet> packet1 = packet->Copy ();
	Ipv4Header ipHeader;
	packet1->RemoveHeader(ipHeader);
        UdpHeader udpheader;
        packet1->RemoveHeader(udpheader);
        if (udpheader.GetDestinationPort()==targetport)
        {
		SeqTsHeader seqTs;
		packet1->RemoveHeader (seqTs);
		uint32_t pktid= seqTs.GetSeq()/10;
                uint32_t n_tx = seqTs.GetSeq()%10; 
        	fprintf(p,"%d	%d	%d	%f\n",n_tx, pktid,packet1->GetSize(),Simulator::Now ().GetSeconds());
        }
}

int main (int argc, char *argv[])
{
	std::cout << "experiment started" <<std::endl;
	RngSeedManager::SetSeed (time(NULL));

	// Initialization 
	double distance = 90;  // m
	uint32_t numNodes = 2;  // by default, 5x5
	uint32_t sourceNode = 0; 

	std::string bVideoFile("crew_base_layer_v1");
	std::string eVideoFile("crew_2nd_layer_v1");
	double videoDuration = 10.0;  //seconds
	double frmRate = 60.0;  // 60 frame per second  
	uint32_t numfrm = 600; // total number of frame in the videoClip 

	double simStart = 0.0; // seconds
	double routingConv = 30.0; // seconds
	double simEnd = 40.0;  //seconds
	uint32_t trial = 1; //number of repeating
	bool layer2Enable = false; 

	CommandLine cmd;
	cmd.AddValue ("distance", "distance (m)", distance);
	cmd.AddValue ("numNodes", "number of nodes", numNodes);
	cmd.AddValue ("sourceNode", "Sender node id", sourceNode);

	cmd.AddValue ("bVideoFile","Filename of the input video base-layer", bVideoFile);
	cmd.AddValue ("eVideoFile","Filename of the input video enhancement layer", eVideoFile);
	cmd.AddValue ("videoDuration", "Duration of video clip (seconds)", videoDuration);
	cmd.AddValue ("frmRate","Frame per second", frmRate);
	cmd.AddValue ("numfrm", "total number of frames in the video-clip",numfrm);

	cmd.AddValue ("simStart", "simulation Start (seconds)", simStart);
	cmd.AddValue ("simEnd", "simulation End (seconds)", simEnd);
	cmd.AddValue ("routingConv","Set time for routing to converge",routingConv);
	cmd.AddValue ("trial", "Number of experiments", trial);

	cmd.AddValue ("layer2Enable", "Turn on or off enhancement layer", layer2Enable);
	cmd.Parse (argc, argv);

	Config::SetDefault ("ns3::ArpCache::PendingQueueSize", UintegerValue (100));

	/*Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
	Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
	Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
	ConfigStore outputConfig2;
	outputConfig2.ConfigureDefaults ();
	outputConfig2.ConfigureAttributes ();*/

	uint32_t sinkNode = numNodes-1;

	uint32_t numberLayer=0;
	if (layer2Enable==false)
	{
		numberLayer=1;
	}
	else
	{
		numberLayer=2;
	}

	// Create network nodes, position and moblity model;
	NodeContainer c;
	c.Create (numNodes);
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc =CreateObject<ListPositionAllocator>();
	for (uint32_t i; i<numNodes; i++) 
	{
		positionAlloc->Add (Vector (0.0, distance*i, 0.0));
	}
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (c);

	//Configure WiFiNetDevices: Channel, PHY, MAC(low,manager,high), 
	YansWifiChannelHelper yanswifiChannel=YansWifiChannelHelper::Default();
	YansWifiPhyHelper yanswifiPhy =  YansWifiPhyHelper::Default ();
	yanswifiPhy.SetChannel (yanswifiChannel.Create ());
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default (); //Default:ns:AdhocWifiMac
	WifiHelper wifi= WifiHelper::Default();  //Default: ns3::ArfWifiManager
	NetDeviceContainer devices = wifi.Install (yanswifiPhy, wifiMac, c);

	//Routing
	OlsrHelper olsr;
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add (staticRouting, 0);
	list.Add (olsr, 10);

        //Internet Stack
	InternetStackHelper internet;
	internet.SetRoutingHelper (list); // has effect on the next Install ()
	internet.Install (c); 

        //IPv4Adress 
	Ipv4AddressHelper ipv4;
	NS_LOG_INFO ("Assign IP Addresses.");
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);


	//Configure applications
	uint16_t MaxPacketSize = 1472;  // Back off 20 (IP) + 8 (UDP) bytes from MTU
	uint16_t bLayerPort = 80; 
	Ptr<VideoSent> bLayerSent = CreateObject<VideoSent>();
	bLayerSent->SetRemote(interfaces.GetAddress (sinkNode), bLayerPort);
	bLayerSent->SetTraceFile(bVideoFile);
	bLayerSent->SetMaxPacketSize(MaxPacketSize);
	bLayerSent->SetVideoStat(numfrm, frmRate);
	bLayerSent->SetNode(c.Get (sourceNode));
	c.Get (sourceNode)->AddApplication (bLayerSent);
	bLayerSent->SetStartTime(Seconds (simStart+routingConv));
	bLayerSent->SetStopTime (Seconds (simEnd));

	// Create one VideoRecv applications on node one.
	Ptr<VideoRecv> bLayerRx = CreateObject<VideoRecv> ();
	bLayerRx->SetNode(c.Get (sinkNode)); 
	bLayerRx->SetAttribute("Port",UintegerValue (bLayerPort));
	c.Get (sinkNode)->AddApplication (bLayerRx);
	bLayerRx->SetStartTime(Seconds (simStart+routingConv));
	bLayerRx->SetStopTime (Seconds (simEnd));
        
	uint16_t eLayerPort = 100;
	Ptr<VideoSent> eLayerSent = CreateObject<VideoSent> ();
	Ptr<VideoRecv> eLayerRx = CreateObject<VideoRecv> ();
	if (layer2Enable==true)
	{
		eLayerSent->SetRemote(interfaces.GetAddress (sinkNode), eLayerPort);
		eLayerSent->SetTraceFile(eVideoFile);
		eLayerSent->SetMaxPacketSize(MaxPacketSize);
		eLayerSent->SetVideoStat(numfrm, frmRate);
		eLayerSent->SetNode(c.Get (sourceNode));
		eLayerSent->SetLayer2flag(layer2Enable);
		c.Get (sourceNode)->AddApplication (eLayerSent);
		eLayerSent->SetStartTime(Seconds (simStart+routingConv));
		eLayerSent->SetStopTime (Seconds (simEnd));

		eLayerRx->SetNode(c.Get (sinkNode)); 
		eLayerRx->SetAttribute("Port",UintegerValue (eLayerPort));
		c.Get (sinkNode)->AddApplication (eLayerRx);
		eLayerRx->SetStartTime(Seconds (simStart+routingConv));
		eLayerRx->SetStopTime (Seconds (simEnd));
	}

 
        //Configure Output
	char bLayerOutput[100]; char bLayerInput[100];char eLayerOutput[100]; char eLayerInput[100];char routeRec[100]; char dropRec[100];
	sprintf(bLayerOutput,"bLayerOutput_numLayer%d_numNode%d_distance%.1f_trial%d.txt",numberLayer,numNodes,distance,trial);
	sprintf(bLayerInput,"bLayerInput_numLayer%d_numNode%d_distance%.1f_trial%d.txt",numberLayer,numNodes,distance,trial);  
	sprintf(eLayerOutput,"eLayerOutput_numLayer%d_numNode%d_distance%.1f_trial%d.txt",numberLayer,numNodes,distance,trial);
	sprintf(eLayerInput,"eLayerInput_numLayer%d_numNode%d_distance%.1f_trial%d.txt",numberLayer,numNodes,distance,trial);
        sprintf(dropRec,"drop_numLayer%d_numNode%d_distance%.1f_trial%d.txt",numberLayer,numNodes,distance,trial);
   	sprintf(routeRec,"routeRec_numNode%d_distance%.1f.txt",numNodes,distance);

        Names::Add("NodeS",c.Get (sourceNode));
        Names::Add("NodeD",c.Get(sinkNode));
        Names::Add("rxPhy",c.Get(sinkNode)->GetDevice(0)->GetObject<WifiNetDevice>()->GetPhy());
        Names::Add("txPhy",c.Get(sourceNode)->GetDevice(0)->GetObject<WifiNetDevice>()->GetPhy());
        Names::Add("rxMAC",c.Get(sinkNode)->GetDevice(0)->GetObject<WifiNetDevice>()->GetMac());
        Names::Add("txMAC",c.Get(sourceNode)->GetDevice(0)->GetObject<WifiNetDevice>()->GetMac());


	Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (routeRec, std::ios::out);
	olsr.PrintRoutingTableAllEvery (Seconds (10.0), routingStream);

	FILE * pFileS;
	pFileS = fopen (bLayerInput,"w");
	Config::ConnectWithoutContext ("NodeS/$ns3::Ipv4L3Protocol/Tx",MakeBoundCallback (&Ipv4TxRx,pFileS,bLayerPort));
	FILE * pFileR;
	pFileR = fopen (bLayerOutput,"w");
	Config::ConnectWithoutContext ("NodeD/$ns3::Ipv4L3Protocol/Rx",MakeBoundCallback (&Ipv4TxRx,pFileR,bLayerPort));

	FILE * pFileS1;FILE * pFileR1;
        if (layer2Enable==true)
	{
		pFileS1 = fopen (eLayerInput,"w");
		Config::ConnectWithoutContext ("NodeS/$ns3::Ipv4L3Protocol/Tx",MakeBoundCallback (&Ipv4TxRx,pFileS1,eLayerPort));
		pFileR1 = fopen (eLayerOutput,"w");
		Config::ConnectWithoutContext ("NodeD/$ns3::Ipv4L3Protocol/Tx",MakeBoundCallback (&Ipv4TxRx,pFileR1,eLayerPort));
	}

	// Run simulation 
	std::cout <<"Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance<<std::endl;
	for (uint32_t i =0; i<numNodes; i++)
	{	std::cout << "Position of Node " <<i << ": "<< c.Get(i)->GetObject<MobilityModel>()-> GetPosition()<<std::endl;}
	std::cout<<"simStart:"<<simStart<<" simEnd:"<<simEnd<<" RoutingConv:"<<routingConv<<std::endl;
	Simulator::Stop (Seconds (simEnd));
	Simulator::Run ();

	// Data processing
	std::cout <<"Base Layer received-pacekt=" << bLayerRx-> GetReceived() << std::endl;
	fclose(pFileS);fclose(pFileR);
	if (layer2Enable==true)
	{
		std::cout <<"2nd Layer received-pacekt=" << eLayerRx-> GetReceived() << std::endl;
		fclose(pFileS1);fclose(pFileR1);
	}

	Simulator::Destroy ();
	return 0;
}

