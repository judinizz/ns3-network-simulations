#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab2Part2");

int main(int argc, char* argv[])
{
    std::string transport_prot = "TcpCubic";
    std::string bottleneckRate = "2Mbps";
    std::string bottleneckDelay = "20ms";
    std::string accessRate = "10Mbps";
    std::string delay1 = "10ms"; 
    std::string delay2 = "50ms"; 
    double stopTime = 20.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("transport_prot", "TCP variant", transport_prot);
    cmd.AddValue("delay1", "Atraso do destino 1", delay1);
    cmd.AddValue("delay2", "Atraso do destino 2", delay2);
    cmd.Parse(argc, argv);

    if (transport_prot.find("ns3::") == std::string::npos)
        transport_prot = "ns3::" + transport_prot;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(transport_prot)));

    NodeContainer source, r1, r2, dest1, dest2;
    source.Create(1);
    r1.Create(1);
    r2.Create(1);
    dest1.Create(1);
    dest2.Create(1);

    InternetStackHelper stack;
    stack.InstallAll();

    PointToPointHelper access1, access2, bottleneck;
    access1.SetDeviceAttribute("DataRate", StringValue(accessRate));
    access1.SetChannelAttribute("Delay", StringValue(delay1));

    access2.SetDeviceAttribute("DataRate", StringValue(accessRate));
    access2.SetChannelAttribute("Delay", StringValue(delay2));

    bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
    bottleneck.SetChannelAttribute("Delay", StringValue(bottleneckDelay));

    NetDeviceContainer devSrcR1 = bottleneck.Install(source.Get(0), r1.Get(0));
    NetDeviceContainer devR1R2 = bottleneck.Install(r1.Get(0), r2.Get(0));
    NetDeviceContainer devR2D1 = access1.Install(r2.Get(0), dest1.Get(0));
    NetDeviceContainer devR2D2 = access2.Install(r2.Get(0), dest2.Get(0));


    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0"); addr.Assign(devSrcR1);
    addr.NewNetwork(); addr.Assign(devR1R2);
    addr.NewNetwork(); addr.Assign(devR2D1);
    addr.NewNetwork(); addr.Assign(devR2D2);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    uint16_t port1 = 50000, port2 = 50001;
    PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
    PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));

    ApplicationContainer sinkApp1 = sinkHelper1.Install(dest1.Get(0));
    ApplicationContainer sinkApp2 = sinkHelper2.Install(dest2.Get(0));
    sinkApp1.Start(Seconds(0.0));
    sinkApp2.Start(Seconds(0.0));
    sinkApp1.Stop(Seconds(stopTime));
    sinkApp2.Stop(Seconds(stopTime));

    Address addr1(InetSocketAddress(Ipv4Address("10.1.3.2"), port1));
    Address addr2(InetSocketAddress(Ipv4Address("10.1.4.2"), port2));

    BulkSendHelper srcHelper1("ns3::TcpSocketFactory", addr1);
    srcHelper1.SetAttribute("MaxBytes", UintegerValue(0));
    BulkSendHelper srcHelper2("ns3::TcpSocketFactory", addr2);
    srcHelper2.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer srcApp1 = srcHelper1.Install(source.Get(0));
    ApplicationContainer srcApp2 = srcHelper2.Install(source.Get(0));

    srcApp1.Start(Seconds(1.0));
    srcApp2.Start(Seconds(1.0));
    srcApp1.Stop(Seconds(stopTime));
    srcApp2.Stop(Seconds(stopTime));

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApp1.Get(0));
    Ptr<PacketSink> sink2 = DynamicCast<PacketSink>(sinkApp2.Get(0));

    double g1 = (sink1->GetTotalRx() * 8.0) / (stopTime - 1.0);
    double g2 = (sink2->GetTotalRx() * 8.0) / (stopTime - 1.0);

    std::cout << "Protocol=" << transport_prot
              << " Delay1=" << delay1
              << " Delay2=" << delay2
              << " Goodput1=" << g1 / 1e6 << "Mbps"
              << " Goodput2=" << g2 / 1e6 << "Mbps"
              << std::endl;

    Simulator::Destroy();
    return 0;
}
