/*
NS_LOG="UdpServer=info|prefix_time|prefix_node|prefix_func" ./ns3 run scratch/adjustGear_2 > scratch/log.adjustGear_2.server.out 2>&1
*/


/*
NS_LOG="UdpServer=info|prefix_time|prefix_node|prefix_func" ./ns3 run scratch/adjustGear_2_fair_mobile_1gnb_UMa > scratch/log.adjustGear_mobile_1gnb10_2.server.out --command-template="%s  --gNbNum=1 --ueNumPergNb=22 --adjustInteval=0.001 --adjustDelay=0.1 --randomStream=1 --windows_size=1" 2>&1
*/
// 9-9

#include <cstring>
#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

#define GearNum 9

NS_LOG_COMPONENT_DEFINE("CttcNrDemo");  // 定义日志组件名称


struct AdjustGearParams {
    std::vector<ns3::Ptr<ns3::UdpClient>> clientApps;
    std::vector<ns3::Ptr<ns3::UdpServer>> serverApps;
    Time adjustInteval;
    Time adjustDelay;
    uint16_t ueAmount;
    int windows_size;
};



// 用户当前档位表
int* ueGearList;

// 档位表
struct GearTable {
    double ueComputationLoad[GearNum];  // 计算量 TFLOPS * 1s
    double cloudComputationLoad[GearNum];   // 计算量   
    // double bandwidthRequirement[GearNum];   // 数据量 Mb
    uint32_t PacketSizeRequirement[GearNum];  // 包大小
};

// 声明并初始化一个GearTable实例
GearTable gearTable = {
    {0.0074, 0.0391, 0.066, 0.0992, 0.1352, 0.0074, 0.0074, 0.0074, 0.0074},
    {0.1577, 0.1278, 0.0918, 0.0594, 0.0318, 0.1278, 0.0918, 0.0594, 0.0318},
    // {0.479, 0.3832, 0.2874, 0.1916, 0.0958, 0.3832, 0.2874, 0.1916, 0.0958},

    {998, 799, 599, 400, 200, 799, 599, 400, 200}       // don't contain 28 header
    // {1026, 827, 627, 428, 228, 827, 627, 428, 228}
};

// 获取适当档位
int relatSchedule(int ueID, double bandWidth) {
	//ueId, bandWidth
    // NS_LOG_UNCOND("  ueID: " << ueID << "\n");
    int newGear = -1;
    double totalTime = 0.0;
    if (ueGearList[ueID] == GearNum-1) return -1;

    for(int l=0;l<GearNum;l++) {
        newGear = l;
        totalTime = 0.0;
        //UE 计算能力3TFLOPS 
        totalTime += gearTable.ueComputationLoad[l]*1000.0/3;
        //cloud 计算能力14TFLOPS
        totalTime += gearTable.cloudComputationLoad[l]*1000.0/14;
        //transmission RTT
        totalTime += ((double)gearTable.PacketSizeRequirement[l])*8/1000/1000/bandWidth * 2.0;

        // NS_LOG_UNCOND("  gear: " << newGear);  
        // NS_LOG_UNCOND("  ue Delay: " << gearTable.ueComputationLoad[l]*1000.0/3 << " ms");
        // NS_LOG_UNCOND("  cloud Delay: " << gearTable.cloudComputationLoad[l]*1000.0/14 << " ms");
        // NS_LOG_UNCOND("  trans Delay: " << ((double)gearTable.PacketSizeRequirement[l] + 28.0)*8/1000/1000/bandWidth * 2.0 << " ms");
        // NS_LOG_UNCOND("  totalTime Delay: " << totalTime << " ms" << "\n");

        if(totalTime < 50) {
            newGear = l;
            break;
        }
    }
    // //档位 0-GearNum /不能e完成 -1
    // if (newGear == -1) {
    //     newGear = 8;
    // }
    NS_LOG_UNCOND("  return gear: " << newGear << "\n");  
    return newGear;
};


// 
void changeGear(int ueNum, std::vector<ns3::Ptr<ns3::UdpClient>>& clientApps, uint32_t newGear) {
    ueGearList[ueNum] = newGear;
    uint32_t new_size = gearTable.PacketSizeRequirement[newGear];
    // NS_LOG_UNCOND("  time " << Simulator::Now().GetSeconds() << " ue " << ueNum << " change Size to " << new_size);
    clientApps[ueNum]->changeGear(newGear, new_size);
}

// 函数定义：动态调档
void adjustGear(AdjustGearParams& params
                ) {
    for (int i = 0; i < params.ueAmount; i++) {
        int ueNum = i;
        double rttDelay = 0.0;
        double FlowThroughput = 0.0;
        double RxSum = 0.0;
        double meanTransDelay = 0.0;
        int recordNum = 0;
        int gear = ueGearList[ueNum];
        ns3::Time recordDelaySum = Seconds(0);

        Time* recordDelay = params.serverApps[i]->getRecordDelay();
        uint16_t* recordGear = params.serverApps[i]->getRecordGear();

        if (recordDelay[params.windows_size-1] == Seconds(0)) {
            continue;
        }
        else {          // last last have data
            // NS_LOG_UNCOND("  all last Delay: " << recordGear[0] << "  "
            //     << recordGear[1] << "  "
            //     << recordGear[2] << "  "
            //     << recordGear[3] << "  "
            //         << recordGear[4] << "  "
            //         << recordGear[5] << " ms");
            // NS_LOG_UNCOND("  all last delay: " << recordDelay[0] << "  "
            //     << recordDelay[1] << "  "
            //     << recordDelay[2] << "  "
            //     << recordDelay[3] << "  "
            //         << recordDelay[4] << "  "
            //         << recordDelay[5] << " ms");


            for (int j = params.windows_size-1; j >= 0; j--) {
                if (recordDelay[j] != Seconds(0) && recordGear[j] == gear) {
                    recordNum++;
                    recordDelaySum += recordDelay[j];
                    RxSum += gearTable.PacketSizeRequirement[recordGear[j]];
                    
                }
            }

            if (recordNum > 0) {
                meanTransDelay = recordDelaySum.GetSeconds() / recordNum * 1000;  // 传输时延
                FlowThroughput = RxSum * 8 / recordNum / meanTransDelay / 1000 / 1000;    // 吞吐量Mbps

                rttDelay += meanTransDelay * 2;

                rttDelay += gearTable.ueComputationLoad[gear]*1000.0/3;
                rttDelay += gearTable.cloudComputationLoad[gear]*1000.0/14;
            }

            // NS_LOG_UNCOND(" ue " << i << "\n");
            // NS_LOG_UNCOND(" recordNum " << recordNum << "\n");
            // NS_LOG_UNCOND(" transDealy " << meanTransDelay * 2 << "\n");
            // NS_LOG_UNCOND(" compDelay " << gearTable.ueComputationLoad[gear]*1000.0/3 + gearTable.cloudComputationLoad[gear]*1000.0/14 << "\n");
            // NS_LOG_UNCOND(" rttDelay " << rttDelay << "\n");

            // change gear
            if (rttDelay > 50 && !params.clientApps[ueNum]->is_lock()) {
                // NS_LOG_UNCOND("  ueNum" << ueNum << "change gear");
                // NS_LOG_UNCOND("  meanTransDelay now is " << meanTransDelay << "ms");

                // NS_LOG_UNCOND("  Throught now is " << FlowThroughput << "Mbps");
                int newGear = relatSchedule(ueNum, FlowThroughput);
                
                //档位 0-GearNum /不能e完成 -1
                if (newGear == -1) {
                    NS_LOG_UNCOND("  ue " << ueNum << " gear -1");
                    newGear = 8;

                    int l_gear = 5; 
                    int flag = 0;
                    while (flag == 0 && l_gear <= 8) {
                        NS_LOG_UNCOND("  l_gear now is " << l_gear );
                        for (int ue = 0; ue < params.ueAmount; ue++) {
                            if (ue != ueNum) {
                                if (ueGearList[ue] < l_gear) {
                                    params.clientApps[ue]->lock();
                                    NS_LOG_UNCOND("  ue " << ue << "change gear to " << l_gear );
                                    Simulator::Schedule(params.adjustDelay, &changeGear, ue, params.clientApps, l_gear);
                                    flag = 1;
                                }
                            }
                        }
                        l_gear++;
                    }
                }
                    
                // NS_LOG_UNCOND("  time " << Simulator::Now().GetSeconds() << " ue " << ueNum << " will change Packet Size to " << new_size << " at " << Simulator::Now().GetMilliSeconds() + adjustDelay.GetMilliSeconds() << "ms");
                params.clientApps[ueNum]->lock();
                Simulator::Schedule(params.adjustDelay, &changeGear, ueNum, params.clientApps, newGear);

            }
        
        }

        
    }

    Simulator::Schedule(params.adjustInteval, &adjustGear, params);
}



// 主函数入口
int main(int argc, char* argv[]) {
    // 各种仿真参数的声明和初始化
    double distanceToRx = 200.0; // meters

    int windows_size = 2;
    int64_t randomStream = 1;  // 随机数流的初始值
    uint16_t gNbNum = 1;          // gNb的数量
    uint16_t ueNumPergNb = 3;         // 用户设备数量
    Time adjustInteval = MilliSeconds(10); //调档间隔
    Time adjustDelay = MilliSeconds(50);  //调档时延

    uint16_t dlPortLowLat = 1234;  // 设置低延迟业务的目的端口
    uint32_t send_interval = 60;  // 超低延迟业务的包发送率
    bool logging = false;  // 是否启用日志
    bool doubleOperationalBand = false;  // 是否使用双操作频段

    // Time simTime = MilliSeconds(1000);  // 模拟时间
    Time simTime = Seconds(15);  // 模拟时间设置
    Time udpAppStartTime = MilliSeconds(500);  // UDP应用启动时间


    uint16_t numerologyBwp1 = 1;  // 带宽部分1的数字化
    double centralFrequencyBand1 = 4e9;  // 频带1的中心频率
    double bandwidthBand1 = 100e6;  // 频带1的带宽
    double totalTxPower = 35;  // 总发射功率

    std::string simTag = "default";  // 模拟标签
    std::string outputDir = "./";  // 输出目录

    CommandLine cmd(__FILE__);  // 命令行解析
    cmd.AddValue("distanceToRx", "X-Axis distance between nodes", distanceToRx);
    cmd.AddValue("logging", "Enable logging", logging);
    cmd.AddValue("doubleOperationalBand", "Simulate two operational bands", doubleOperationalBand);
    cmd.AddValue("send_interval", "UDP packets per second for ultra low latency traffic", send_interval);
    cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.AddValue("numerologyBwp1", "Numerology for bandwidth part 1", numerologyBwp1);
    cmd.AddValue("centralFrequencyBand1", "Frequency for band 1", centralFrequencyBand1);
    cmd.AddValue("bandwidthBand1", "Bandwidth for band 1", bandwidthBand1);
    cmd.AddValue("totalTxPower", "Total transmit power", totalTxPower);
    cmd.AddValue("simTag", "Tag for output filenames", simTag);
    cmd.AddValue("outputDir", "Directory for simulation results", outputDir);

    cmd.AddValue("gNbNum", "gNbNum", gNbNum);
    cmd.AddValue("ueNumPergNb", "ueNumPergNb", ueNumPergNb);
    cmd.AddValue("adjustInteval", "adjustInteval", adjustInteval);
    cmd.AddValue("adjustDelay", "adjustDelay", adjustDelay);
    cmd.AddValue("randomStream", "randomStream", randomStream);
    cmd.AddValue("windows_size", "windows_size", windows_size);
    
    cmd.Parse(argc, argv);

    NS_ABORT_IF(centralFrequencyBand1 < 0.5e9 && centralFrequencyBand1 > 100e9);  // 检查中心频率是否在合理范围内

    if (logging) {  // 如果启用日志，则激活相关组件的日志功能
        LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
        LogComponentEnable("LtePdcp", LOG_LEVEL_INFO);

    }
    
    ueGearList = new int[gNbNum*ueNumPergNb + 1];
    for (int i = 0; i < gNbNum*ueNumPergNb ; i++) {
        ueGearList[i] = 0;
    }

    NS_LOG_UNCOND("  " << "gNbNum :" << gNbNum << "\n");
    NS_LOG_UNCOND("  " << "ueNumPergNb :" << ueNumPergNb << "\n");
    NS_LOG_UNCOND("  " << "adjustInteval :" << adjustInteval.GetMilliSeconds() << "ms\n");
    NS_LOG_UNCOND("  " << "adjustDelay :" << adjustDelay.GetMilliSeconds() << "ms\n");

    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));  // 设置最大传输缓冲区大小

    NodeContainer gNbNodes;
    NodeContainer ueNodes;
    MobilityHelper mobility;
    // int nodeSpeed = 0.83; // in m/s

    gNbNodes.Create(1);
    ueNodes.Create(gNbNum*ueNumPergNb);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
    gnbPositionAlloc->Add(Vector(0.0, 0.0, 25));
    
    mobility.SetPositionAllocator(gnbPositionAlloc);
    mobility.Install(gNbNodes);

    mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel",
                            "Bounds",
                            RectangleValue(Rectangle(-500, 500, -500, 500)),
                            "Speed",
                            StringValue("ns3::ConstantRandomVariable[Constant=0.83]")
                            );

    double min = -500.0;
    double max = 500.0;
    Ptr<UniformRandomVariable> randomPos = CreateObject<UniformRandomVariable> ();
    randomPos->SetAttribute ("Min", DoubleValue (min));
    randomPos->SetAttribute ("Max", DoubleValue (max));
    for (int i = 0; i < gNbNum*ueNumPergNb; i++) {
        Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator>();
        double x = randomPos->GetValue();
        double y = randomPos->GetValue();
        uePositionAlloc->Add(Vector(x, y, 1.5));
        NS_LOG_UNCOND(" x " << x << " y " << y << "\n");

        mobility.SetPositionAllocator(uePositionAlloc);
        mobility.Install(ueNodes.Get(i));
        randomStream += mobility.AssignStreams(ueNodes.Get(i), randomStream);
    }


    NS_LOG_UNCOND("  " << Simulator::Now().GetSeconds() << "  Scenario create success " << "\n");

    NodeContainer ueLowLatContainer;  // 创建低延迟用户设备容器

    for (uint32_t j = 0; j < ueNodes.GetN(); ++j) {  // 遍历所有用户终端
        Ptr<Node> ue = ueNodes.Get(j);  // 获取用户终端
        ueLowLatContainer.Add(ue);  // 添加到低延迟容器中
    }

    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();  // 创建点对点EPC助手
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();  // 创建理想波束成形助手
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();  // 创建NR助手

    nrHelper->SetBeamformingHelper(idealBeamformingHelper);  // 设置波束成形助手
    nrHelper->SetEpcHelper(epcHelper);  // 设置EPC助手

    BandwidthPartInfoPtrVector allBwps;  // 创建带宽部分信息指针向量
    CcBwpCreator ccBwpCreator;  // 创建CC BWP创建者
    const uint8_t numCcPerBand = 1;  // 每个频段的CC数量

    CcBwpCreator::SimpleOperationBandConf bandConf1(centralFrequencyBand1, bandwidthBand1, numCcPerBand, BandwidthPartInfo::UMa);  // 创建简单操作频带配置

    OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf1);  // 创建连续CC操作频带

    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(0)));  // 设置3GPP信道模型更新周期
    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));  // 设置信道条件模型属性
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));  // 设置路径损耗属性

    nrHelper->InitializeOperationBand(&band1);  // 初始化操作频带


    double x = pow(10, totalTxPower / 10);  // 计算总发射功率对应的线性值
    double totalBandwidth = bandwidthBand1;  // 设置总带宽

    allBwps = CcBwpCreator::GetAllBwps({band1});  // 获取所有带宽部分

    Packet::EnableChecking();  // 启用数据包检查
    Packet::EnablePrinting();  // 启用数据包打印

    idealBeamformingHelper->SetAttribute("BeamformingMethod", TypeIdValue(DirectPathBeamforming::GetTypeId()));  // 设置波束成形方法
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));  // 设置S1u链接延迟
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));  // 设置用户设备天线行数
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));  // 设置用户设备天线列数
    nrHelper->SetUeAntennaAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));  // 设置用户设备天线元素
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));  // 设置gNb天线行数
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(16));  // 设置gNb天线列数
    nrHelper->SetGnbAntennaAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));  // 设置gNb天线元素

    uint32_t bwpIdForLowLat = 0;  // 设置低延迟业务的带宽部分ID

    nrHelper->SetSchedulerTypeId(TypeId::LookupByName("ns3::NrMacSchedulerOfdmaPF"));

    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_GAMING", UintegerValue(bwpIdForLowLat));  // 设置gNb带宽部分管理算法属性
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_GAMING", UintegerValue(bwpIdForLowLat));  // 设置用户设备带宽部分管理算法属性

    NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice(gNbNodes, allBwps);  // 安装gNb设备
    NetDeviceContainer ueLowLatNetDev = nrHelper->InstallUeDevice(ueLowLatContainer, allBwps);  // 安装用户设备

    randomStream += nrHelper->AssignStreams(enbNetDev, randomStream);  // 分配随机数流
    randomStream += nrHelper->AssignStreams(ueLowLatNetDev, randomStream);  // 分配随机数流

    nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
        ->SetAttribute("Numerology", UintegerValue(numerologyBwp1));  // 设置gNb物理层的数值化
    nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
        ->SetAttribute("TxPower", DoubleValue(10 * log10((bandwidthBand1 / totalBandwidth) * x)));  // 设置gNb发射功率

    for (auto it = enbNetDev.Begin(); it != enbNetDev.End(); ++it) {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();  // 更新gNb配置
    }

    for (auto it = ueLowLatNetDev.Begin(); it != ueLowLatNetDev.End(); ++it) {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();  // 更新用户设备配置
    }

    NS_LOG_UNCOND("  gnb set success " << Simulator::Now().GetSeconds() << "\n");

    Ptr<Node> pgw = epcHelper->GetPgwNode();  // 获取PGW节点
    NodeContainer remoteHostContainer;  // 创建远程主机容器
    remoteHostContainer.Create(1);  // 创建一个远程主机
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);  // 获取远程主机节点
    InternetStackHelper internet;  // 创建互联网堆栈助手
    internet.Install(remoteHostContainer);  // 安装互联网堆栈到远程主机

    PointToPointHelper p2ph;  // 创建点对点助手
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));  // 设置设备数据率
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));  // 设置MTU
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0)));  // 设置通道延迟
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);  // 安装互联网设备
    Ipv4AddressHelper ipv4h;  // 创建IPv4地址助手
    Ipv4StaticRoutingHelper ipv4RoutingHelper;  // 创建IPv4静态路由助手
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");  // 设置IPv4地址基础
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);  // 分配IPv4接口
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());  // 获取远程主机的静态路由
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);  // 添加网络路由
    internet.Install(ueNodes);  // 安装互联网堆栈到用户终端

    Ipv4InterfaceContainer ueLowLatIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLowLatNetDev));  // 分配IPv4地址给低延迟用户设备
  
    // 遍历所有用户终端，设置默认路由
    for (uint32_t j = 0; j < ueNodes.GetN(); ++j) {
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(
            ueNodes.Get(j)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // nrHelper->AttachToClosestEnb(ueLowLatNetDev, enbNetDev);  // 将用户设备连接到最近的基站
    for (int i = 0; i < gNbNum; i++) {
        for (int j = 0; j < ueNumPergNb; j++) {
            nrHelper->AttachToEnb(ueLowLatNetDev.Get(i*ueNumPergNb+j), enbNetDev.Get(i));
        }
    }

    NS_LOG_UNCOND("  p2p an IP set success " << Simulator::Now().GetSeconds() << "\n");

    // uint16_t dlPortLowLat = 1234;  // 设置低延迟业务的目的端口

    // The bearer that will carry low latency traffic
    EpsBearer lowLatBearer(EpsBearer::GBR_GAMING);

    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat;
    dlpfLowLat.localPortStart = dlPortLowLat;
    dlpfLowLat.localPortEnd = dlPortLowLat;
    lowLatTft->Add(dlpfLowLat);

    // ApplicationContainer serverApps;  // 创建服务器应用容器
    UdpServerHelper dlPacketSinkLowLat(dlPortLowLat);  // 创建UDP服务器助手，用于低延迟业务
    std::vector<ns3::Ptr<ns3::UdpServer>> serverApps(gNbNum*ueNumPergNb);



    UdpClientHelper dlClientLowLat[gNbNum*ueNumPergNb];  // 创建UDP客户端助手
    std::vector<ns3::Ptr<ns3::UdpClient>> clientApps(gNbNum*ueNumPergNb);  // 创建客户端应用容器
    ApplicationContainer clientAppsContainer[gNbNum*ueNumPergNb];  // 创建客户端应用容器

    for (uint32_t j = 0; j < gNbNum*ueNumPergNb; ++j)
    {
        dlClientLowLat[j].SetAttribute("RemotePort", UintegerValue(dlPortLowLat));  // 设置远程端口
        dlClientLowLat[j].SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));  // 设置最大包数
        dlClientLowLat[j].SetAttribute("PacketSize", UintegerValue(gearTable.PacketSizeRequirement[ueGearList[j]]));  // 设置包大小
        dlClientLowLat[j].SetAttribute("Interval", TimeValue(Seconds(1.0 / send_interval)));  // 设置发送间隔

        // 配置并安装客户端应用到远程主机
        Ptr<Node> ue = ueLowLatContainer.Get(j);
        Ptr<NetDevice> ueDevice = ueLowLatNetDev.Get(j);
        Address ueAddress = ueLowLatIpIface.GetAddress(j);
        dlClientLowLat[j].SetAttribute("RemoteAddress", AddressValue(ueAddress));
        clientApps[j] = dlClientLowLat[j].Install(remoteHost);

        serverApps[j] = dlPacketSinkLowLat.Install(ue);  // 安装UDP服务器到低延迟用户设备

        // Activate a dedicated bearer for the traffic type
        nrHelper->ActivateDedicatedEpsBearer(ueDevice, lowLatBearer, lowLatTft);

        clientApps[j]->SetStartTime(udpAppStartTime);
        clientApps[j]->SetStopTime(simTime);

        // 启动和停止服务器及客户端应用
        serverApps[j]->SetStartTime(udpAppStartTime);
        serverApps[j]->SetStopTime(simTime);
    }
    
    
    AdjustGearParams params = {
        clientApps,        // std::vector<ns3::Ptr<ns3::UdpClient>>
        serverApps,        // std::vector<ns3::Ptr<ns3::UdpServer>>
        adjustInteval,     // Time adjustInteval
        adjustDelay,       // Time adjustDelay
        (uint16_t)gNbNum*ueNumPergNb, // uint16_t ueAmount
        windows_size
    };

    Simulator::Schedule(MilliSeconds(400) + adjustInteval, &adjustGear, params);



    Simulator::Stop(simTime);  // 设置模拟停止时间
    Simulator::Run();  // 运行模拟


    Simulator::Destroy();  // 销毁模拟器


    return 0;  // 返回0表示成功
}

