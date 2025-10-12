/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SecondScriptExample");


int main(int argc, char* argv[])
{

    bool verbose = true;
    uint32_t nCsma = 4;   
    uint32_t nPackets = 1; 

    CommandLine cmd(__FILE__);
    cmd.AddValue("nPackets", "Number of packets per client (max 20)", nPackets);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.Parse(argc, argv);

    if (nPackets > 20) nPackets = 20;

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    NodeContainer p2pNodes;
    p2pNodes.Create(2); 

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1)); 
    csmaNodes.Create(nCsma);        

    NodeContainer p2p2Nodes;
    p2p2Nodes.Add(csmaNodes.Get(nCsma)); 
    p2p2Nodes.Create(1);                 

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices = pointToPoint.Install(p2pNodes);
    NetDeviceContainer p2p2Devices = pointToPoint.Install(p2p2Nodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices = csma.Install(csmaNodes);


    InternetStackHelper stack;
    stack.Install(p2pNodes.Get(0));     
    stack.Install(csmaNodes);           
    stack.Install(p2p2Nodes.Get(1));   


    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2p2Interfaces = address.Assign(p2p2Devices);

    UdpEchoServerHelper echoServer(15);

    ApplicationContainer serverApps = echoServer.Install(p2p2Nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(30.0));

    UdpEchoClientHelper echoClient(p2p2Interfaces.GetAddress(1), 15);
    echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(p2pNodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(30.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
