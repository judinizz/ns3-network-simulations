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

NS_LOG_COMPONENT_DEFINE("Lab2Part1c");

int main(int argc, char* argv[])
{
    std::string transport_prot = "TcpCubic";
    std::string dataRate = "1Mbps";
    std::string delay = "1ms";
    double errorRate = 0.00001;
    uint16_t nFlows = 1;
    std::string prefix = "lab2-part1c";
    double sim_stop = 20.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("transport_prot", "TcpCubic or TcpNewReno", transport_prot);
    cmd.AddValue("dataRate", "Bottleneck data rate", dataRate);
    cmd.AddValue("delay", "Bottleneck delay", delay);
    cmd.AddValue("errorRate", "Bottleneck error rate", errorRate);
    cmd.AddValue("nFlows", "Number of TCP flows", nFlows);
    cmd.AddValue("prefix", "Output prefix", prefix);
    cmd.Parse(argc, argv);

    if (transport_prot.find("ns3::") == std::string::npos)
        transport_prot = "ns3::" + transport_prot;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(transport_prot)));

    NodeContainer src, r1, r2, dst;
    src.Create(1);
    r1.Create(1);
    r2.Create(1);
    dst.Create(nFlows);

    InternetStackHelper stack;
    stack.InstallAll();

    PointToPointHelper fast, bottleneck;
    fast.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    fast.SetChannelAttribute("Delay", StringValue("0.01ms"));
    bottleneck.SetDeviceAttribute("DataRate", StringValue(dataRate));
    bottleneck.SetChannelAttribute("Delay", StringValue(delay));

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(errorRate));

    NetDeviceContainer devSrcR1 = fast.Install(src.Get(0), r1.Get(0));
    NetDeviceContainer devR1R2 = bottleneck.Install(r1.Get(0), r2.Get(0));
    for (uint32_t d = 0; d < devR1R2.GetN(); ++d)
        DynamicCast<PointToPointNetDevice>(devR1R2.Get(d))->SetReceiveErrorModel(em);

    std::vector<NetDeviceContainer> devR2DstVec;
    PointToPointHelper fast2;
    fast2.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    fast2.SetChannelAttribute("Delay", StringValue("0.01ms"));
    for (uint32_t i = 0; i < nFlows; ++i)
        devR2DstVec.push_back(fast2.Install(r2.Get(0), dst.Get(i)));

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    addr.Assign(devSrcR1);
    addr.NewNetwork(); addr.Assign(devR1R2);
    for (uint32_t i = 0; i < nFlows; ++i)
    { addr.NewNetwork(); addr.Assign(devR2DstVec[i]); }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;
    ApplicationContainer sinks;
    for (uint32_t i = 0; i < nFlows; ++i)
    {
        std::string ipStr = "10.1." + std::to_string(3 + i) + ".2";
        Address sinkAddr(InetSocketAddress(Ipv4Address(ipStr.c_str()), port + i));
    
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddr);
        ApplicationContainer sinkApp = sinkHelper.Install(dst.Get(i));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(sim_stop));
        sinks.Add(sinkApp);
    
        BulkSendHelper srcHelper("ns3::TcpSocketFactory", sinkAddr);
        srcHelper.SetAttribute("MaxBytes", UintegerValue(0));
        srcHelper.SetAttribute("SendSize", UintegerValue(400));
        ApplicationContainer srcApp = srcHelper.Install(src.Get(0));
        srcApp.Start(Seconds(1.0));
        srcApp.Stop(Seconds(sim_stop));
    }


    Simulator::Stop(Seconds(sim_stop));
    Simulator::Run();

    double aggGoodput = 0.0;
    for (uint32_t i = 0; i < sinks.GetN(); ++i)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinks.Get(i));
        if (sink)
        {
            double g = (sink->GetTotalRx() * 8.0) / (sim_stop - 1.0);
            aggGoodput += g;
        }
    }

    std::cout << "Protocol=" << transport_prot
              << " nFlows=" << nFlows
              << " errorRate=" << errorRate
              << " Goodput_agregado=" << aggGoodput / 1e6 << " Mbps" << std::endl;

    Simulator::Destroy();
    return 0;
}
