/*
 * incidencies-graphml-trace.cc
 * Copyright (C) 2012  Cristian Tanas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Cristian Tanas <ctanas@deic.uab.cat>
 */

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"

#define EVENT_TIME_INFO 2
#define EVENT_NODE_INFO 3
#define NODE_INFO_LENGTH 8				// Length of '$node_('

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("IncidenciesMobilityTrace");

std::vector<double> currentReputationValues;
double bias = .9;
uint32_t	generatedEvents = 0;

double
RandomNumberUniform ()
{
	return drand48 ();
}

double
RandomNumberInterval (double min, double max)
{
	return (max-min) * RandomNumberUniform () + min;
}

bool
TossBiasedCoin (double bias)
{
	double randomNumber = RandomNumberUniform ();
	bool accept = randomNumber < bias ? true : false;
	return accept;
}

uint32_t
GetNodeNumFromContext (std::string context)
{
	std::vector<std::string> elements;
	std::stringstream ss (context);
	std::string item;

	while (std::getline (ss, item, '/')) {
		elements.push_back (item);
	}

	std::stringstream toInteger (elements.at (2));
	uint32_t nodeId;
	toInteger >> nodeId;

	return nodeId;
}

uint32_t
GetRandomNode (int maxNodes)
{
	return (int) maxNodes * drand48 ();
}

void
InitializeNodeContainer (NodeContainer *n, double selfishProb, double trustedNodes, double initialReputationValue)
{

	for ( unsigned int i = 0; i < n->GetN (); i++ )
	{
		if ( selfishProb == .5 ) n->Get (i)->SetAttribute ("SelfishProb", DoubleValue (RandomNumberInterval (.2, .8)));
		else n->Get (i)->SetAttribute ("SelfishProb", DoubleValue (selfishProb));

		if ( trustedNodes == .0 ) n->Get (i)->SetAttribute ("Reputation", DoubleValue (initialReputationValue));
	}
//	if ( selfishProb == .5 )
//	{
//		for (unsigned int i = 0; i < n->GetN (); i++)
//			n->Get (i)->SetAttribute ("SelfishProb", DoubleValue (RandomNumberInterval(0.2, 0.8)));
//	}
//	else {
//		for (unsigned int i = 0; i < n->GetN (); i++)
//			n->Get (i)->SetAttribute ("SelfishProb", DoubleValue (selfishProb));
//	}
//
//	for ( unsigned int i = 0; i < n->GetN (); i++ )
//	{
//		bool accept = TossBiasedCoin (bias);
//		if ( accept && selfishProb != -1 && selfishProb != 1. ) n->Get (i)->SetAttribute ("Reputation", DoubleValue (1.));
//	}
}

void
SelectTrustedNodes (NodeContainer *n, double trustedNodes)
{
	uint32_t trustedNodeNum = round (n->GetN () * trustedNodes);
	for ( unsigned int i = 0; i < trustedNodeNum; i++ )
	{
		uint32_t node = GetRandomNode (n->GetN ());
		n->Get (node)->SetAttribute ("Reputation", DoubleValue (1.));
	}
}

void
ReputationValueTrace (std::string context, double oldValue, double newValue)
{
//	NS_LOG_INFO (Simulator::Now() << " " << context << " old reputation value or= " <<
//			oldValue << ", new reputation value nr= " << newValue);

//	NS_LOG_INFO (Simulator::Now ().GetSeconds () << " " << GetNodeNumFromContext (context) << " " << newValue);
	currentReputationValues[GetNodeNumFromContext (context)] = newValue;
}

void
CourseChange (std::string context, Ptr<const MobilityModel> model)
{
	Vector position = model->GetPosition ();
	NS_LOG_INFO (context << " x = " << position.x << ", y = " << position.y);
}

void
DumpReputationValues (std::ostream *os, double nextDumpDelay)
{
	std::stringstream dumpStream;
	dumpStream << generatedEvents << ",";
	for (unsigned int i = 0; i < currentReputationValues.size (); i++)
	{
		if ( i == (currentReputationValues.size () - 1) )
			dumpStream << currentReputationValues.at (i);
		else
			dumpStream << currentReputationValues.at (i) << ",";
	}

//	NS_LOG_INFO (dumpStream.str ());
	*os << dumpStream.str () << "\n";
	Simulator::Schedule (Seconds (nextDumpDelay), &DumpReputationValues, os, nextDumpDelay);
}

void
DumpNodeInfo (NodeContainer n, std::string filename)
{
	std::ofstream networkTopology (filename.c_str ());

	for (unsigned int i = 0; i < n.GetN (); i++)
	{
		Ptr<Node> node = n.Get (i);

		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
		Ipv4Address localhost = ipv4->GetAddress(1, 0).GetLocal ();

		Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
		Vector3D nodePos = mobility->GetPosition ();

		DoubleValue selfishProb; node->GetAttribute ("SelfishProb", selfishProb);
		DoubleValue reputationValue; node->GetAttribute ("Reputation", reputationValue);

		networkTopology << "Created Node " << node->GetId () << " [" << localhost <<  "]. Position (" <<
				nodePos.x << ", " << nodePos.y << ", " << nodePos.z << ") -- Selfishness probability = " <<
				selfishProb.Get () << ", Reputation = " << reputationValue.Get () << "\n";
	}

	networkTopology.close();
}

void
DumpPosStatistics (std::ostream *os, NodeContainer container, uint32_t nodeId)
{
	Ptr<Node> selectedNode = container.Get (nodeId);
	Ptr<MobilityModel> mob = selectedNode->GetObject<MobilityModel> ();

	std::stringstream dumpStream;
	dumpStream << nodeId << "," << mob->GetPosition ().x << "," << mob->GetPosition ().y << ",";

	for ( unsigned int i = 0; i < container.GetN (); i++ )
	{
		if ( i != nodeId )
		{
			Ptr<Node> current = container.Get (i);
			mob = current->GetObject<MobilityModel> ();
			if ( i == (container.GetN () - 1) ) {
				dumpStream << mob->GetPosition ().x << "," << mob->GetPosition ().y;
			}
			else {
				dumpStream << mob->GetPosition ().x << "," << mob->GetPosition ().y << ",";
			}
		}
	}

	*os << dumpStream.str () << "\n";
}

void
NewEvent (NodeContainer container, double eventGenDelay)
{
	uint32_t genNode = GetRandomNode (container.GetN ());
	Ptr<Node> selectedNode = container.Get (genNode);

//	NS_LOG_INFO ("At time " << Simulator::Now() << " Node " << selectedNode->GetId () << " was selected.");

	Ptr<IncidentGenerator> genApp = selectedNode->GetApplication (1)->GetObject<IncidentGenerator> ();
	genApp->GenerateNewIncident (Seconds (0.0));
	generatedEvents++;

	Simulator::Schedule(Seconds (eventGenDelay), &NewEvent, container, eventGenDelay);
}

void
AddEvent (NodeContainer container, uint32_t nodeId)
{
	Ptr<Node> selectedNode = container.Get (nodeId);
	NS_LOG_INFO ("At time " << Simulator::Now().GetSeconds () << " Node " << nodeId << " was selected.");

	Ptr<IncidentGenerator> genApp = selectedNode->GetApplication (1)->GetObject<IncidentGenerator> ();
	genApp->GenerateNewIncident(Seconds (.0));
	generatedEvents++;
}

void
WifiPhyTxBeginTrace(std::string context, Ptr<const Packet> p)
{
	NS_LOG_INFO ("INCIDENCIES_GRAPHML_TRACE::WifiPhyTxBeginTrace (" << context << ")");
}

void
WifiPhyRxBeginTrace(std::string context, Ptr<const Packet> p)
{
	NS_LOG_INFO ("INCIDENCIES_GRAPHML_TRACE::WifiPhyRxBeginTrace (" << context << ")");
}

int main (int argc, char *argv[])
{
	// Enable logging from the ns2 helper
	//LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);
	LogComponentEnable ("IncidenciesMobilityTrace", LOG_LEVEL_INFO);

	std::string 	paramsFile;
//	std::string		traceFilePath = "/home/cristian/apps/bonnmotion-2.0/mobility-traces";
	std::string 	traceFile;
	std::string		outputFile;
	std::string		reputationTraceFile;
	std::string		overrideReputationTraceFile;
	std::string		eventListFile;					// Fitxer que guarda la llista de totes les incidències generades
	std::string 	posStatisticsFile;				// Fitxer que guarda informació estadística del posicionament dels nodes
	uint32_t		numNodes = 100;
	double			selfishNodesP = .25;
	double			altruisticNodesP = .5;
	double			maliciousNodesP = .1;
	double			trustedNodes = .0;
	double			initialReputationValue = .0;
	double			duration = 100.;
	double			waitForConfDelay = 1.;
	uint32_t		validationMode = 0;
	uint32_t		weightFunction = 21;
	double			confirmationThreshold = 1.;
	double			falseIncidentThreshold = 1.;
	double			reputationThreshold = .9;
	double			generatedIncWeight = 2.;
	double			generationInterval = 3.;
	double			wifiRange = 100.;
	uint32_t		printNetworkTopology = 0;
	std::string		topologyFile;
	uint32_t		printLogInfo = 0;
	uint32_t		genAnimation = 0;

	srand48 (time (0));

	// Parse command line attribute
	CommandLine cmd;
	cmd.AddValue ("params", "File containing the parameters for the simulation", paramsFile);
	cmd.AddValue ("reputationTraceFile", "File containing the reputation traces", overrideReputationTraceFile);
//	cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);
//	cmd.AddValue ("outputFile", "Generated animation file", outputFile);
//	cmd.AddValue ("reputationTraceFile", "Reputation values file", reputationTraceFile);
//	cmd.AddValue ("nodeNum", "Number of nodes", numNodes);
//	cmd.AddValue ("selfishNodes", "Number of nodes", selfishNodesP);
//	cmd.AddValue ("altruisticNodes", "Number of nodes", altruisticNodesP);
//	cmd.AddValue ("duration", "Duration of Simulation", duration);
//	cmd.AddValue ("waitConfirmations", "Time window (in seconds) to wait for confirmations", waitForConfDelay);
//	cmd.AddValue ("confirmationThr", "Number of confirmation needed for incident validation", confirmationThreshold);
//	cmd.AddValue ("reputationThr", "Minimum reputation value needed for incident validation", reputationThreshold);
//	cmd.AddValue ("generationInterval", "Incident generation interval (in seconds)", generationInterval);
//	cmd.AddValue ("wifiRange", "Propagation loss range for the WifiChannel", wifiRange);
//	cmd.AddValue ("t", "Enable or disable network topology dump", printNetworkTopology);
//	cmd.AddValue ("topologyFile", "Network topology generated file", topologyFile);
//	cmd.AddValue ("log", "Enable LOG_INFO messages", printLogInfo);
	cmd.Parse (argc,argv);

	std::ifstream params (paramsFile.c_str ());
	std::string paramTuple;
	std::string::size_type pos;
	while ( !params.eof () )
	{
		std::getline(params, paramTuple);
		if ( paramTuple.find ("#") == std::string::npos )
		{
			pos = paramTuple.find ("=");
			std::string paramName = paramTuple.substr(0, pos);
			std::string paramValue = paramTuple.substr(pos + 1, paramTuple.size () - pos);
			std::stringstream parse (paramValue);

			if ( paramName == "traceFile" ) {
				parse >> traceFile;
			}
			else if ( paramName == "outputFile" ) {
				parse >> outputFile;
			}
			else if ( paramName == "reputationTraceFile" ) {
				parse >> reputationTraceFile;
			}
			else if ( paramName == "eventListFile" ) {
				parse >> eventListFile;
			}
			else if ( paramName == "posStatsFile" ) {
				parse >> posStatisticsFile;
			}
			else if ( paramName == "nodeNum" ) {
				parse >> numNodes;
			}
			else if ( paramName == "selfishNodes" ) {
				parse >> selfishNodesP;
			}
			else if ( paramName == "altruisticNodes" ) {
				parse >> altruisticNodesP;
			}
			else if ( paramName == "maliciousNodes" ) {
				parse >> maliciousNodesP;
			}
			else if ( paramName == "trustedNodes" ) {
				parse >> trustedNodes;
			}
			else if ( paramName == "initialReputationValue" ) {
				parse >> initialReputationValue;
			}
			else if ( paramName == "duration" ) {
				parse >> duration;
			}
			else if ( paramName == "waitConfirmations" ) {
				parse >> waitForConfDelay;
			}
			else if ( paramName == "validationMode" ) {
				parse >> validationMode;
			}
			else if ( paramName == "weightFunction" ) {
				parse >> weightFunction;
			}
			else if ( paramName == "confirmationThr" ) {
				parse >> confirmationThreshold;
			}
			else if ( paramName == "falseIncidentThr" ) {
				parse >> falseIncidentThreshold;
			}
			else if ( paramName == "reputationThr" ) {
				parse >> reputationThreshold;
			}
			else if ( paramName == "generatedIncWeight" ) {
				parse >> generatedIncWeight;
			}
			else if ( paramName == "generationInterval" ) {
				parse >> generationInterval;
			}
			else if ( paramName == "wifiRange" ) {
				parse >> wifiRange;
			}
			else if ( paramName == "t" ) {
				parse >> printNetworkTopology;
			}
			else if ( paramName == "topologyFile" ) {
				parse >> topologyFile;
			}
			else if ( paramName == "log" ) {
				parse >> printLogInfo;
			}
			else if ( paramName == "anim" ) {
				parse >> genAnimation;
			}
		}
	}
	params.close ();

	if ( !overrideReputationTraceFile.empty () ) reputationTraceFile = overrideReputationTraceFile;

	if ( printLogInfo == 1 )
	{
		LogComponentEnable ("IncidentGeneratorApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("IncidentSinkApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("IncidenciesMobilityTrace", LOG_LEVEL_INFO);
	}

//	traceFilePath.append (traceFile);

	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
			StringValue ("DsssRate1Mbps"));

	// Create Ns2MobilityHelper with the specified trace log file as parameter
	Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

	// Create all nodes.
	NodeContainer selfishNodes, altruisticNodes, maliciousNodes, randomNodes;
	uint32_t numOfSelfishNodes = (uint32_t) numNodes * selfishNodesP; // Nodes that are selfish by nature
	selfishNodes.Create (numOfSelfishNodes);
	InitializeNodeContainer(&selfishNodes, 1.0, .0, initialReputationValue);

	uint32_t numOfAltruisticNodes = (uint32_t) numNodes * altruisticNodesP;
	altruisticNodes.Create (numOfAltruisticNodes);	// Nodes that are altruistic by nature
	InitializeNodeContainer (&altruisticNodes, 0.0, .0, initialReputationValue);
	SelectTrustedNodes (&altruisticNodes, trustedNodes);

	uint32_t numOfMaliciousNodes = (uint32_t) numNodes * maliciousNodesP;
	maliciousNodes.Create (numOfMaliciousNodes);
	InitializeNodeContainer (&maliciousNodes, -1, .0, initialReputationValue);

	uint32_t numOfRandomNodes = numNodes - numOfSelfishNodes - numOfAltruisticNodes - numOfMaliciousNodes;
	randomNodes.Create (numOfRandomNodes); // Node that have variable probability of being selfish
	InitializeNodeContainer(&randomNodes, 0.5, .0, initialReputationValue);

	NodeContainer allNodes;
	allNodes.Add (selfishNodes);
	allNodes.Add (altruisticNodes);
	allNodes.Add (maliciousNodes);
	allNodes.Add (randomNodes);

	ns2.Install (); // configure movements for each node, while reading trace file

	InternetStackHelper internet;
	internet.Install (allNodes);

	// Explicitly create the channels required by the topology
	WifiHelper wifi;
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager("ns3::IdealWifiManager");

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();

	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel", "Speed", DoubleValue(1));
	wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",
			DoubleValue (wifiRange));

	wifiPhy.SetChannel(wifiChannel.Create ());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType("ns3::AdhocWifiMac");

	NetDeviceContainer d = wifi.Install (wifiPhy, wifiMac, allNodes);

	// We've got the "hardware" in place. Now we need to add IP addresses
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.0.0", "255.255.0.0");
	Ipv4InterfaceContainer i = ipv4.Assign (d);

	// Create IncidentSink application and install it on all nodes
	uint16_t port = 8089;
	IncidentSinkHelper incidentSink (port);
	incidentSink.SetAttribute ("ConfirmationWeight", DoubleValue (1/generatedIncWeight));
	ApplicationContainer sinkApps = incidentSink.Install (allNodes);
	sinkApps.Start (Seconds (1.0));

	// Create IncidentGenerator application to generate new incidents and install it on all nodes
	IncidentGeneratorHelper incidentGen (port);
	incidentGen.SetAttribute ("StartOffset", TimeValue (Seconds (1.0)));
	incidentGen.SetAttribute ("TimerDelay", TimeValue (Seconds (waitForConfDelay)));
	incidentGen.SetAttribute ("ValidationMode", UintegerValue (validationMode));
	incidentGen.SetAttribute ("WeightFunction", UintegerValue (weightFunction));
	incidentGen.SetAttribute ("ConfirmationThreshold", DoubleValue (confirmationThreshold));
	incidentGen.SetAttribute ("DecreaseThreshold", DoubleValue (falseIncidentThreshold));
	incidentGen.SetAttribute ("ReputationThreshold", DoubleValue (reputationThreshold));
	incidentGen.SetAttribute ("GenerationWeight", DoubleValue (1.));
	ApplicationContainer generatorApps = incidentGen.Install (allNodes);
	generatorApps.Start (Seconds (1.0));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Initialize current reputation array with the initial values for all the nodes
	for (unsigned int i = 0; i < allNodes.GetN (); i++)
	{
		DoubleValue rep; allNodes.Get (i)->GetAttribute("Reputation", rep);
		currentReputationValues.push_back(rep.Get ());
	}

	// Define traceback call for changes in the reputation value
	Config::Connect ("/NodeList/*/Reputation", MakeCallback (&ReputationValueTrace));

	// Define traceback call for changes in the position and/or velocity vector
	//Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback (&CourseChange));

	//Config::Connect ("NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin",
	//		MakeCallback (&WifiPhyTxBeginTrace));
	//Config::Connect ("NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxBegin",
	//		MakeCallback (&WifiPhyRxBeginTrace));

	if ( printNetworkTopology == 1 ) // Print network topology if indicated
		DumpNodeInfo (allNodes, topologyFile);

	std::ofstream repFile (reputationTraceFile.c_str ());

	// Print reputation values information
	Simulator::Schedule (Seconds (1.0), &DumpReputationValues, &repFile, generationInterval);

	// Start generating incidents
	//Simulator::Schedule (Seconds (3.0), &NewEvent, allNodes, generationInterval);
	// Schedule incident generation events based on the event list passed along as a parameter
	std::ifstream eventList (eventListFile.c_str ());
	std::ofstream posStatistics (posStatisticsFile.c_str ());
	std::string event;
	while ( !eventList.eof() ) {

		std::getline(eventList, event);
		// We only process the lines that begin with '$ns_ at'
		if ( !(event.find("$ns_ at") == std::string::npos) ) {

			NS_LOG_INFO ("==PROCESSED EVENT: " << event << "==");

			std::stringstream eventStream (event);
			std::string info;
			std::vector<std::string> eventInfo;

			while ( std::getline(eventStream, info, ' ') ) {
				eventInfo.push_back(info);
			}

			//NS_LOG_INFO ("Event string separated into " << eventInfo.size() << " parts");
			NS_LOG_INFO ("Time and node info: " << eventInfo.at (EVENT_TIME_INFO) << " " << eventInfo.at (EVENT_NODE_INFO));

			// Decode the time when an incident should be generated
			std::stringstream toDouble (eventInfo.at (EVENT_TIME_INFO));
			double timeAt = .0; toDouble >> timeAt;

			// Decode the Node that should generate the incident
			std::string nodeIdStr = eventInfo.at (EVENT_NODE_INFO).substr(
					NODE_INFO_LENGTH,
					eventInfo.at (EVENT_NODE_INFO).length() - NODE_INFO_LENGTH - 1);

			//NS_LOG_INFO ("Parsed nodeId: " << nodeIdStr);

			std::stringstream toInteger (nodeIdStr);
			int32_t nodeId = 0; toInteger >> nodeId;

			NS_LOG_INFO ("Parsed nodeId=" << nodeId);

			if ( nodeId >= 0 ) {
				Simulator::Schedule (Seconds (timeAt), &AddEvent, allNodes, nodeId);
				Simulator::Schedule (Seconds (timeAt), &DumpPosStatistics, &posStatistics, allNodes, nodeId);
			}

		}
	}

	NS_LOG_INFO("Starting simulation...");

	Simulator::Stop (Seconds (duration));


	NS_LOG_INFO ("Generating animation file...");

	// Generate NetAnim XML file
	AnimationInterface animation (outputFile.c_str ());
	animation.EnablePacketMetadata (true);
	animation.SetStartTime(Seconds (.0));

	if ( genAnimation == 1 ) {
		animation.SetStopTime(Seconds (duration));
	}
	else {
		animation.SetStopTime(Seconds (.0));
	}

	Simulator::Run ();
	Simulator::Destroy ();

	//repFile.close ();
	posStatistics.close ();

	return 0;
}


