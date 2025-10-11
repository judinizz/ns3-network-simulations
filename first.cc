/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int
main(int argc, char* argv[])
{
    uint32_t nClients = 1;
    uint32_t nPackets = 1;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nClients", "Escolha o número de clientes (máx: 5)", nClients);
    cmd.AddValue("nPackets", "Escolha o número de pacotes por cliente (máx: 5)", nPackets);
    cmd.Parse(argc, argv);

    //trata >5
    if (nClients > 5) nClients = 5;
    if (nPackets > 5) nPackets = 5;

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer serverNode;
    serverNode.Create(1);

    NodeContainer clientNodes;
    clientNodes.Create(nClients);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    InternetStackHelper stack;
    stack.Install(serverNode);
    stack.Install(clientNodes);

    Ipv4AddressHelper address;
    std::vector<Ipv4InterfaceContainer> interfaces;

    for (uint32_t i = 0; i < nClients; i++)
    {
        NodeContainer pair(clientNodes.Get(i), serverNode.Get(0));
        NetDeviceContainer devices = pointToPoint.Install(pair);

        std::ostringstream subnet;
        subnet << "10.1." << i + 1 << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");

        interfaces.push_back(address.Assign(devices));
    }

    //config roteamento
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //config UDP porta 15
    uint16_t port = 15;
    UdpEchoServerHelper echoServer(port);

    ApplicationContainer serverApps = echoServer.Install(serverNode.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    //config clientes
    for (uint32_t i = 0; i < nClients; i++)
    {
        Ipv4Address serverAddress = interfaces[0].GetAddress(1);

        UdpEchoClientHelper echoClient(serverAddress, port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApp = echoClient.Install(clientNodes.Get(i));

        Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
        double startTime = rand->GetValue(2.0, 7.0);

        clientApp.Start(Seconds(startTime));
        clientApp.Stop(Seconds(20.0));
    }

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
