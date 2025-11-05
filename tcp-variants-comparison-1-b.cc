#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab2Part1b");

int main(int argc, char* argv[])
{
    std::string transport_prot = "TcpCubic";
    std::string dataRate = "1Mbps";
    std::string delay = "50ms";
    double errorRate = 0.00001;
    uint16_t nFlows = 1;
    std::string prefix = "lab2-part1b";
    double sim_stop = 20.0;
    uint64_t data_mbytes = 0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("transport_prot", "TcpCubic or TcpNewReno", transport_prot);
    cmd.AddValue("dataRate", "Bottleneck data rate", dataRate);
    cmd.AddValue("delay", "Bottleneck delay", delay);
    cmd.AddValue("errorRate", "Error rate", errorRate);
    cmd.AddValue("nFlows", "Number of TCP flows", nFlows);
    cmd.AddValue("prefix", "Output prefix", prefix);
    cmd.Parse(argc, argv);

    if (transport_prot.find("ns3::") == std::string::npos)
        transport_prot = "ns3::" + transport_prot;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(transport_prot)));


    NodeContainer src, router1, router2, dst;
    src.Create(1);
    router1.Create(1);
    router2.Create(1);
    dst.Create(nFlows);

    InternetStackHelper stack;
    stack.InstallAll();

    PointToPointHelper p2pSrcR1, p2pR1R2, p2pR2Dst;
    p2pSrcR1.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pSrcR1.SetChannelAttribute("Delay", StringValue("0.01ms"));
    p2pR1R2.SetDeviceAttribute("DataRate", StringValue(dataRate));
    p2pR1R2.SetChannelAttribute("Delay", StringValue(delay));
    p2pR2Dst.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pR2Dst.SetChannelAttribute("Delay", StringValue("0.01ms"));

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(errorRate));

    NetDeviceContainer devSrcR1 = p2pSrcR1.Install(src.Get(0), router1.Get(0));
    NetDeviceContainer devR1R2 = p2pR1R2.Install(router1.Get(0), router2.Get(0));
    for (uint32_t d = 0; d < devR1R2.GetN(); ++d)
        DynamicCast<PointToPointNetDevice>(devR1R2.Get(d))->SetReceiveErrorModel(em);

    std::vector<NetDeviceContainer> devR2DstVec;
    for (uint32_t i = 0; i < nFlows; ++i)
        devR2DstVec.push_back(p2pR2Dst.Install(router2.Get(0), dst.Get(i)));

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifSrcR1 = addr.Assign(devSrcR1);
    addr.NewNetwork();
    Ipv4InterfaceContainer ifR1R2 = addr.Assign(devR1R2);

    std::vector<Ipv4InterfaceContainer> ifR2DstVec;
    for (uint32_t i = 0; i < nFlows; ++i)
    {
        addr.NewNetwork();
        ifR2DstVec.push_back(addr.Assign(devR2DstVec[i]));
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;
    ApplicationContainer sinks;
    ApplicationContainer sources;
    for (uint32_t i = 0; i < nFlows; ++i)
    {
        Address sinkAddr(InetSocketAddress(ifR2DstVec[i].GetAddress(1), port + i));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddr);
        ApplicationContainer sinkApp = sinkHelper.Install(dst.Get(i));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(sim_stop));
        sinks.Add(sinkApp);

        BulkSendHelper sender("ns3::TcpSocketFactory", sinkAddr);
        sender.SetAttribute("MaxBytes", UintegerValue(data_mbytes));
        sender.SetAttribute("SendSize", UintegerValue(400));
        ApplicationContainer srcApp = sender.Install(src.Get(0));
        srcApp.Start(Seconds(1.0));
        srcApp.Stop(Seconds(sim_stop));
        sources.Add(srcApp);
    }

    Simulator::Stop(Seconds(sim_stop));
    Simulator::Run();

    double totalGoodput = 0.0;
    for (uint32_t i = 0; i < sinks.GetN(); ++i)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinks.Get(i));
        if (sink)
        {
            double g = (sink->GetTotalRx() * 8.0) / (sim_stop - 1.0);
            totalGoodput += g;
        }
    }
    std::cout << "Protocol=" << transport_prot
              << " nFlows=" << nFlows
              << " delay=" << delay
              << " Goodput_agregado=" << totalGoodput / 1e6 << " Mbps" << std::endl;

    Simulator::Destroy();
    return 0;
}
