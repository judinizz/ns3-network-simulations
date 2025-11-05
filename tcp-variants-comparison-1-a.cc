#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-header.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab2Part1");

static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, bool> firstCwnd;

static uint32_t GetNodeIdFromContext(std::string context)
{
    const std::size_t n1 = context.find_first_of('/', 1);
    const std::size_t n2 = context.find_first_of('/', n1 + 1);
    return std::stoul(context.substr(n1 + 1, n2 - n1 - 1));
}

static void CwndTracer(std::string context, uint32_t oldval, uint32_t newval)
{
    uint32_t nodeId = GetNodeIdFromContext(context);
    if (firstCwnd[nodeId])
    {
        *cWndStream[nodeId]->GetStream() << "0.0 " << oldval << std::endl;
        firstCwnd[nodeId] = false;
    }
    *cWndStream[nodeId]->GetStream() << Simulator::Now().GetSeconds() << " " << newval << std::endl;
}

static void TraceCwnd(std::string cwnd_tr_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    cWndStream[nodeId] = ascii.CreateFileStream(cwnd_tr_file_name);
    Config::Connect("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",
                MakeCallback(&CwndTracer));

}


int main(int argc, char *argv[])
{
    std::string transport_prot = "TcpCubic";
    std::string dataRate = "1Mbps";
    std::string delay = "20ms";
    double errorRate = 0.00001;
    uint16_t nFlows = 1;
    std::string prefix_file_name = "lab2-part1";
    bool tracing = true;
    uint64_t data_mbytes = 0;
    uint32_t mtu_bytes = 400;
    double sim_stop = 20.0;
    bool pcap = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("dataRate", "Bottleneck data rate", dataRate);
    cmd.AddValue("delay", "Bottleneck link delay", delay);
    cmd.AddValue("errorRate", "Bottleneck link error rate", errorRate);
    cmd.AddValue("nFlows", "Number of TCP flows", nFlows);
    cmd.AddValue("transport_prot", "TCP variant: TcpCubic or TcpNewReno", transport_prot);
    cmd.AddValue("prefix_name", "Prefix for output files", prefix_file_name);
    cmd.AddValue("tracing", "Enable tracing", tracing);
    cmd.Parse(argc, argv);

    if (transport_prot.find("ns3::") == std::string::npos)
        transport_prot = "ns3::" + transport_prot;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName(transport_prot)));

    NodeContainer nodes;
    nodes.Create(4);

    PointToPointHelper p2pFast;
    p2pFast.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pFast.SetChannelAttribute("Delay", StringValue("0.01ms"));

    PointToPointHelper p2pBottleneck;
    p2pBottleneck.SetDeviceAttribute("DataRate", StringValue(dataRate));
    p2pBottleneck.SetChannelAttribute("Delay", StringValue(delay));

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(errorRate));


    InternetStackHelper stack;
    stack.InstallAll();

    NetDeviceContainer devSrcR1 = p2pFast.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer devR1R2 = p2pBottleneck.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer devR2Dst = p2pFast.Install(nodes.Get(2), nodes.Get(3));

    for (uint32_t d = 0; d < devR1R2.GetN(); ++d)
    {
        Ptr<PointToPointNetDevice> p2pnd = DynamicCast<PointToPointNetDevice>(devR1R2.Get(d));
        if (p2pnd)
            p2pnd->SetReceiveErrorModel(em);
    }

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifSrcR1 = address.Assign(devSrcR1);

    address.NewNetwork();
    Ipv4InterfaceContainer ifR1R2 = address.Assign(devR1R2);

    address.NewNetwork();
    Ipv4InterfaceContainer ifR2Dst = address.Assign(devR2Dst);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;
    ApplicationContainer sinkApps;
    ApplicationContainer sourceApps;

    Address sinkAddress(InetSocketAddress(ifR2Dst.GetAddress(1), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(3));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(sim_stop));
    sinkApps.Add(sinkApp);

    BulkSendHelper sender("ns3::TcpSocketFactory", sinkAddress);
    sender.SetAttribute("MaxBytes", UintegerValue(data_mbytes));
    sender.SetAttribute("SendSize", UintegerValue(mtu_bytes));
    ApplicationContainer sourceApp = sender.Install(nodes.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(sim_stop));
    sourceApps.Add(sourceApp);

    if (tracing)
    {
        uint32_t srcId = nodes.Get(0)->GetId();
        firstCwnd[srcId] = true;
        Simulator::Schedule(Seconds(1.01),
                            &TraceCwnd,
                            prefix_file_name + "-flow0-cwnd.data",
                            srcId);
    }

    if (pcap)
    {
        p2pBottleneck.EnablePcapAll(prefix_file_name);
    }

    Simulator::Stop(Seconds(sim_stop));

    for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(i));
        if (sink)
        {
            uint64_t totalBytes = sink->GetTotalRx();
            double activeTime = sim_stop - 1.0; // 19s ativo
            double goodput_bps = (totalBytes * 8.0) / activeTime;
            std::cout << "Flow " << i << " Goodput = " << goodput_bps
                      << " bps (" << goodput_bps / 1e6 << " Mbps)" << std::endl;
        }
    }


    Simulator::Destroy();
    return 0;
}
