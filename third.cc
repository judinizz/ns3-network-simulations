/* SPDX-License-Identifier: GPL-2.0-only */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThirdScriptExample");

int main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nWifi = 4;
    uint32_t nPackets = 1;
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of wifi STA devices per network", nWifi);
    cmd.AddValue("nPackets", "Number of packets to send (max 20)", nPackets);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    cmd.Parse(argc, argv);

    if (nWifi > 9)
    {
        std::cout << "nWifi should be 9 or less" << std::endl;
        return 1;
    }
    if (nPackets > 20) nPackets = 20;

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices = pointToPoint.Install(p2pNodes);

    NodeContainer wifiStaNodes1;
    wifiStaNodes1.Create(nWifi);
    NodeContainer wifiApNode1 = p2pNodes.Get(0);

    YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy1;
    phy1.SetChannel(channel1.Create());

    WifiHelper wifi1;
    wifi1.SetRemoteStationManager("ns3::MinstrelHtWifiManager"); //ALT

    WifiMacHelper mac1;
    Ssid ssid1 = Ssid("network-1");

    mac1.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid1),
                 "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices1 = wifi1.Install(phy1, mac1, wifiStaNodes1);

    mac1.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid1));
    NetDeviceContainer apDevices1 = wifi1.Install(phy1, mac1, wifiApNode1);

    NodeContainer wifiStaNodes2;
    wifiStaNodes2.Create(nWifi);
    NodeContainer wifiApNode2 = p2pNodes.Get(1);

    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy2;
    phy2.SetChannel(channel2.Create());

    WifiHelper wifi2;
    wifi2.SetRemoteStationManager("ns3::MinstrelHtWifiManager"); //ALT

    WifiMacHelper mac2;
    Ssid ssid2 = Ssid("network-2");

    mac2.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid2),
                 "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices2 = wifi2.Install(phy2, mac2, wifiStaNodes2);

    mac2.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid2));
    NetDeviceContainer apDevices2 = wifi2.Install(phy2, mac2, wifiApNode2);

    // --- Mobility ---
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes1);
    mobility.Install(wifiStaNodes2);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode1);
    mobility.Install(wifiApNode2);

    InternetStackHelper stack;
    stack.Install(wifiApNode1);
    stack.Install(wifiApNode2);
    stack.Install(wifiStaNodes1);
    stack.Install(wifiStaNodes2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces1 = address.Assign(staDevices1);
    address.Assign(apDevices1);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces2 = address.Assign(staDevices2);
    address.Assign(apDevices2);

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(wifiStaNodes1.Get(nWifi - 1)); 
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    UdpEchoClientHelper echoClient(staInterfaces1.GetAddress(nWifi - 1), 9); // dest = servidor
    echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(wifiStaNodes2.Get(nWifi - 1)); 
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(20.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(25.0));

    if (tracing)
    {
        phy1.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy2.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        pointToPoint.EnablePcapAll("lab1-part3");
        phy1.EnablePcap("lab1-part3-network1", apDevices1.Get(0));
        phy2.EnablePcap("lab1-part3-network2", apDevices2.Get(0));
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
