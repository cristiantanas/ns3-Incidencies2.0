/*
 * incidencies-generator-application.cc
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

#include <sstream>
#include <math.h>

#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/double.h"
#include "ns3/string.h"

#include "incident-generator-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IncidentGeneratorApplication");
NS_OBJECT_ENSURE_REGISTERED(IncidentGenerator);

TypeId
IncidentGenerator::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::IncidentGenerator")
			.SetParent<Application> ()
			.AddConstructor<IncidentGenerator> ()
			.AddAttribute("StartOffset", "Wait time before generating a new incident",
					TimeValue (Seconds (1.0)),
					MakeTimeAccessor (&IncidentGenerator::m_startOffset),
					MakeTimeChecker ())
			.AddAttribute("RemotePort", "Destination port where the packets should be sent",
					UintegerValue (8089),
					MakeUintegerAccessor (&IncidentGenerator::m_remotePort),
					MakeUintegerChecker<uint16_t> ())
			.AddAttribute("TimerDelay", "Time to wait for broadcast confirmations",
					TimeValue (Seconds (1.0)),
					MakeTimeAccessor (&IncidentGenerator::m_timerDelay),
					MakeTimeChecker())
			.AddAttribute ("SelfishProb", "The probability of a Node being selfish",
					DoubleValue (0.0),
					MakeDoubleAccessor (&IncidentGenerator::m_selfishProb),
					MakeDoubleChecker<double> ())
			.AddAttribute ("BlackList", "List of Nodes that are selfish by nature",
					StringValue (),
					MakeStringAccessor (&IncidentGenerator::m_blackList),
					MakeStringChecker ())
			.AddAttribute ("ValidationMode", "Incident validation method.",
					UintegerValue (0),
					MakeUintegerAccessor (&IncidentGenerator::m_validationMode),
					MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("WeightFunction", "The weight function to use (i.e. linear, exponential or table-based.",
					UintegerValue (21),
					MakeUintegerAccessor (&IncidentGenerator::m_weightFunction),
					MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("ConfirmationThreshold", "Minimum confirmations required.",
					DoubleValue (1.),
					MakeDoubleAccessor (&IncidentGenerator::m_confirmationThreshold),
					MakeDoubleChecker<double> ())
			.AddAttribute ("DecreaseThreshold", "Reputation decrease threshold.",
					DoubleValue (1.),
					MakeDoubleAccessor (&IncidentGenerator::m_falseIncidentThreshold),
					MakeDoubleChecker<double> ())
			.AddAttribute ("ReputationThreshold", "Minimum reputation value required.",
					DoubleValue (0.9),
					MakeDoubleAccessor (&IncidentGenerator::m_reputationThreshold),
					MakeDoubleChecker<double> ())
			.AddAttribute ("GenerationWeight", "Weight assigned to generated incidents.",
					DoubleValue (1.),
					MakeDoubleAccessor (&IncidentGenerator::m_generatedIncWeight),
					MakeDoubleChecker<double> ())
	;

	return tid;
}

IncidentGenerator::IncidentGenerator()
{
	NS_LOG_FUNCTION_NOARGS();

	m_sent = 0;
	m_socket = 0;
	m_sendEvent = EventId ();
	m_timer = Timer ();

	m_NGeneratedIncidents = 0;

	m_atLeastOneUserWithHR = 0;

	m_selfishProb = .0;

	m_validationMode = ABSOLUTE_VALUE_MODE;
	m_confirmedIncWeight = 1.;
	m_maliciousNode = false;

	srand (time (0));
}

IncidentGenerator::~IncidentGenerator()
{
	NS_LOG_FUNCTION_NOARGS();

	m_socket = 0;
}
void
IncidentGenerator::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
IncidentGenerator::StartApplication (void)
{
	NS_LOG_FUNCTION_NOARGS();

	m_timer.SetFunction (&IncidentGenerator::AllConfirmationsReceived, this);
	m_timer.SetDelay (m_timerDelay);

	if ( m_socket == 0 )
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);

		m_socket->Bind ();
		m_socket->Connect (InetSocketAddress (Ipv4Address ("255.255.255.255"), m_remotePort));
		m_socket->SetAllowBroadcast (true);
	}

	m_socket->SetRecvCallback(MakeCallback (&IncidentGenerator::HandleConfirmations, this));

	if ( !m_blackList.empty () )
	{
		std::stringstream ss (m_blackList);
		std::string item;

		while ( std::getline(ss, item, ',') )
		{
			m_blackListIpv4.push_back(Ipv4Address (item.c_str ()));
		}
	}

	DoubleValue selfishProb; GetNode ()->GetAttribute ("SelfishProb", selfishProb);
	m_maliciousNode = selfishProb.Get () == -1 ? true : false;

//	GenerateNewIncident (m_startOffset);
}

void
IncidentGenerator::StopApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();

	if ( m_socket != 0 )
	{
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
		m_socket = 0;
	}
}

void
IncidentGenerator::GenerateNewIncident (Time dt)
{
	NS_LOG_FUNCTION_NOARGS();
	++m_NGeneratedIncidents;
	m_sendEvent = Simulator::Schedule(dt, &IncidentGenerator::SendBroadcast, this);
}

void
IncidentGenerator::SendBroadcast (void)
{
	NS_LOG_FUNCTION_NOARGS();

	NS_ASSERT (m_sendEvent.IsExpired());

	// Clear previous neighbours and saved reputation values
	m_confirmationArray.clear ();
	m_neighbours.clear ();
	m_reputationMap.clear ();

	Ptr<Packet> packet = Create<Packet> (512);
	m_socket->Send(packet);
	++m_sent;

	m_timer.Schedule ();

	// Get Node's local IP address
	Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
	Ipv4Address local = ipv4->GetAddress (1, 0).GetLocal ();

	NS_LOG_INFO ("+" << Simulator::Now().GetSeconds () << " " << local << " " << "255.255.255.255"
			<< " " << "m=" << m_maliciousNode << " " << "[GEN_INC]");
}

void
IncidentGenerator::SendReputationUpdate (uint8_t action)
{
	NS_LOG_FUNCTION_NOARGS ();

	NS_ASSERT (m_sendEvent.IsExpired ());
	Ptr<Packet> packet = Create<Packet> (256);
	ReputationTag tag;
	tag.SetDoAction (action);
	packet->AddPacketTag (tag);

	// Get Node's local IP address
	Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
	Ipv4Address local = ipv4->GetAddress (1, 0).GetLocal ();

	std::vector<Address>::reverse_iterator rit;

	for ( rit = m_confirmationArray.rbegin (); rit < m_confirmationArray.rend (); ++rit)
	{
		Ipv4Address sendToIp = InetSocketAddress::ConvertFrom (*rit).GetIpv4 ();
		m_socket->SendTo (packet, 0, InetSocketAddress (sendToIp, 8089));
		++m_sent;

		std::string actionStr = action == 0 ? "INCREASE_REP" : "DECREASE_REP";
		NS_LOG_INFO ("+" << Simulator::Now ().GetSeconds () << " " << local << " " << sendToIp
				<< " " << "m=" << m_maliciousNode << " " << "a=" << actionStr << " " << "[REP_UPDATE]");
	}
}

void
IncidentGenerator::HandleConfirmations (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;

	while ( (packet = socket->RecvFrom (from)) )
	{
		double reputationVal = .0;
		double selfishProb = .0;

		uint8_t *buffer = new uint8_t [packet->GetSize ()];
		memset(buffer, 0, packet->GetSize ());
		packet->CopyData(buffer, packet->GetSize ());
		std::stringstream ss; ss << buffer;
//		ss >> reputationVal;

		std::vector<std::string> elements;
		std::string item;
		while ( std::getline (ss, item, '#') )
			elements.push_back (item);

		NS_LOG_INFO ("--" << elements.at (0) << " " << elements.at (1) << " " << packet->GetSize () << " " << "[CONF_RCVD]");

		// Get Reputation value from the packet received
		std::stringstream repStrToDouble; repStrToDouble << elements.at (0); repStrToDouble >> reputationVal;
		// Get SelfishProb value from the packet received
		std::stringstream selStrToDouble; selStrToDouble << elements.at (1); selStrToDouble >> selfishProb;

		Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
		Ipv4Address local = ipv4->GetAddress (1, 0).GetLocal ();
//		NS_LOG_INFO ("[CONF_RCVD] " << Simulator::Now ().GetSeconds () << " " << local << " " << packet->GetSize () << " " <<
//					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " <<
//					InetSocketAddress::ConvertFrom (from).GetPort ());

		Ipv4Address fromAddress = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

		m_neighbours.push_back (from);

		// The Incident Generator Nodes decides which confirmations are valid based on the
		// selfishness probability of the Node that confirmed
		bool keepConfirmation;
		if ( !m_maliciousNode ) {
			keepConfirmation = TossBiasedCoin (selfishProb);
		}
		else {
			keepConfirmation = (selfishProb == -1);
		}

		NS_LOG_INFO ("-" << Simulator::Now ().GetSeconds () << " " << fromAddress << " " << local
				<< " " << "m=" << m_maliciousNode << " " << "r=" << reputationVal << " " << "s=" << selfishProb
				<< " " << "k=" << keepConfirmation << " " << "[CONF_RCVD]");

		if ( keepConfirmation )
		{
			m_confirmationArray.push_back(from);

			if ( reputationVal >= m_reputationThreshold ) {
				m_atLeastOneUserWithHR = 1;
			}
			m_reputationMap.insert (std::pair<Ipv4Address, double> (fromAddress, reputationVal));
		}
	}
}

void
IncidentGenerator::AllConfirmationsReceived (void)
{
	NS_LOG_FUNCTION_NOARGS ();

//	NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s confirmation timer has expired.");
//	NS_LOG_INFO ("Confirmation list:");
//	for (unsigned int i = 0; i < m_confirmationArray.size (); i++)
//	{
//		Ipv4Address ip = InetSocketAddress::ConvertFrom (m_confirmationArray.at (i)).GetIpv4 ();
//		NS_LOG_INFO (ip);
//	}

	// Print statistics. |Time | ConfirmationThreshold | #ConfirmationsReceived | #Neighbours|
//	NS_LOG_INFO ("[STATS] " << Simulator::Now ().GetSeconds () << " " << m_confirmationThreshold <<
//			" " << m_confirmationArray.size () << " " << m_neighbours.size ());

//	NS_LOG_INFO ("m_confirmationArray=" << m_confirmationArray.size () << ", m_confirmationThreshold=" << m_confirmationThreshold);
//	std::string cond = m_confirmationArray.size()>m_confirmationThreshold ? "true" : "false";
//	NS_LOG_INFO (cond);
	Ipv4Address local = GetNode ()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
	DoubleValue myReputation; GetNode ()->GetAttribute ("Reputation", myReputation);

	uint32_t doAction = ValidateIncidentWithMode (m_validationMode);
	DoubleValue validIncidents, invalidIncidents;
	double newValidIncidents, newInvalidIncidents;
	std::string doActionStr;
	switch ( doAction ) {
	case INCREASE_REPUTATION:
		doActionStr = "INCREASE_REP";
		GetNode ()->GetAttribute ("ValidIncidents", validIncidents);
		GetNode ()->GetAttribute ("InvalidIncidents", invalidIncidents);
		newValidIncidents = validIncidents.Get () + m_generatedIncWeight;
		GetNode ()->SetAttribute ("ValidIncidents", DoubleValue (newValidIncidents));

		NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << local << " "
				<< "m=" << m_maliciousNode << " " << "a=" << doActionStr << " "
				<< "alfa_b=" << validIncidents.Get () << " " << "alfa_a=" << newValidIncidents
				<< " " << "beta=" << invalidIncidents.Get () << " " << "[STATS]");

		if ( myReputation.Get () != 1 ) UpdateNodeReputation();
		SendReputationUpdate (0);
		break;

	case DECREASE_REPUTATION:
		doActionStr = "DECREASE_REP";
		GetNode ()->GetAttribute ("ValidIncidents", validIncidents);
		GetNode ()->GetAttribute ("InvalidIncidents", invalidIncidents);
		newInvalidIncidents = invalidIncidents.Get () + 1;
		GetNode ()->SetAttribute ("InvalidIncidents", DoubleValue (newInvalidIncidents));

		NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << local << " "
				<< "m=" << m_maliciousNode << " " << "a=" << doActionStr << " "
				<< "alfa=" << validIncidents.Get ()
				<< " " << "beta_b=" << invalidIncidents.Get () << " " << "beta_a=" << newInvalidIncidents << " " << "[STATS]");

		UpdateNodeReputation ();
		SendReputationUpdate (1);
		break;

	case DO_NOTHING:
		doActionStr = "DO_NOTHING";
		break;

	default:
		break;
	}
}

uint32_t
IncidentGenerator::ValidateIncidentWithMode (uint32_t validationMode)
{
	Ipv4Address local = GetNode ()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
	uint32_t requiredConfirmations = 0;
	uint32_t minConfirmations = 0;

	DoubleValue myReputation;
	double weight = .0;
	switch ( validationMode )
	{
	case ABSOLUTE_VALUE_MODE:
		requiredConfirmations = (uint32_t) m_confirmationThreshold;
		minConfirmations = (uint32_t) m_falseIncidentThreshold;
		NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << local << " " << "m=" << m_maliciousNode <<
				" " << "max_t=" << requiredConfirmations <<
				" " << "min_t=" << minConfirmations << " " << "nc=" << m_confirmationArray.size () <<
				" " << "nn=" << m_neighbours.size () << " " << "[STATS-AV]");
		if ( m_confirmationArray.size () >= requiredConfirmations ) return INCREASE_REPUTATION;
		else if ( m_confirmationArray.size () <= minConfirmations ) return DECREASE_REPUTATION;
		return  DO_NOTHING;

	case DENSITY_FUNCTION_MODE:
		requiredConfirmations = (uint32_t) ceil (m_neighbours.size () * m_confirmationThreshold);
		minConfirmations = (uint32_t) ceil (m_neighbours.size () * m_falseIncidentThreshold);
		NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << local << " " << "m=" << m_maliciousNode <<
						" " << "max_t=" << requiredConfirmations <<
						" " << "min_t=" << minConfirmations << " " << "nc=" << m_confirmationArray.size () <<
						" " << "nn=" << m_neighbours.size () << " " << "[STATS-DF]");
		if ( m_confirmationArray.size () >= requiredConfirmations ) return INCREASE_REPUTATION;
		else if ( m_confirmationArray.size () <= minConfirmations ) return DECREASE_REPUTATION;
		return  DO_NOTHING;

	case WEIGHT_FUNCTION_MODE:
		GetNode ()->GetAttribute ("Reputation", myReputation);
		weight += GetConfirmationWeight (myReputation.Get ());
		for ( std::map<Ipv4Address, double>::iterator it = m_reputationMap.begin (); it != m_reputationMap.end (); ++it )
		{
			weight += GetConfirmationWeight (it->second);
		}
		NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << local << " " << "m=" << m_maliciousNode <<
						" " << "max_t=" << m_confirmationThreshold <<
						" " << "min_t=" << m_falseIncidentThreshold << " " << "nc=" << m_reputationMap.size () <<
						" " << "w=" << weight << " " << "[STATS-WF]");
		if ( weight >= m_confirmationThreshold) return INCREASE_REPUTATION;
		else if ( weight <= m_falseIncidentThreshold ) return DECREASE_REPUTATION;
		return DO_NOTHING;

	default:
		return false;
	}
}

void
IncidentGenerator::UpdateNodeReputation (void)
{
	DoubleValue nValidIncidents;
	GetNode ()->GetAttribute ("ValidIncidents", nValidIncidents);
	DoubleValue nInvalidIncidents;
	GetNode ()->GetAttribute ("InvalidIncidents", nInvalidIncidents);

	double newReputationVal = (nValidIncidents.Get () + 1) /
			(nValidIncidents.Get () + nInvalidIncidents.Get () + 2);
	GetNode()->SetAttribute ("Reputation", DoubleValue (newReputationVal));
//	NS_LOG_INFO ("Reputation value of Node " << GetNode()->GetId () <<
//			" changed and the new value is " << rep.Get ());
}

bool
IncidentGenerator::TossBiasedCoin (double bias)
{
	double randomNumber = rand () / double (RAND_MAX);
	bool accept = randomNumber < bias ? false : true;
	return accept;
}

double
IncidentGenerator::GetConfirmationWeight (double reputationVal)
{
//	NS_LOG_INFO ("[GET_WEIGHT] " << Simulator::Now ().GetSeconds () << " GetConfirmationWeight function called with "
//			"reputationVal = " << reputationVal);
	switch ( m_weightFunction )
	{
	case LINEAR_WEIGHT_FUN:
		return reputationVal / m_reputationThreshold;

	case EXP_WEIGHT_FUN:
		return exp (4 * (reputationVal - m_reputationThreshold));

	case QUADRATIC_WEIGHT_FUN:
		return reputationVal * reputationVal;

	default:
		return .0;
	}
}


/***************************************************************
 *           Reputation Tags
 ***************************************************************/

ReputationTag::ReputationTag ()
{

}

void
ReputationTag::SetDoAction (uint8_t action)
{
  m_doAction = action;
}

uint8_t
ReputationTag::GetDoAction (void) const
{
  return m_doAction;
}

NS_OBJECT_ENSURE_REGISTERED (ReputationTag);

TypeId
ReputationTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ReputationTag")
    .SetParent<Tag> ()
    .AddConstructor<ReputationTag> ()
  ;
  return tid;
}
TypeId
ReputationTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
ReputationTag::GetSerializedSize (void) const
{
  return 1;
}
void
ReputationTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_doAction);
}
void
ReputationTag::Deserialize (TagBuffer i)
{
  m_doAction = i.ReadU8 ();
}
void
ReputationTag::Print (std::ostream &os) const
{
  os << "Action=" << (uint32_t) m_doAction;
}

} // namespace ns3


