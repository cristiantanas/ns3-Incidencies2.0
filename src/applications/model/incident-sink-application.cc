/*
 * incidencies-sink-application.cc
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

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include <ctime>
#include "incident-sink-application.h"
#include "incident-generator-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IncidentSinkApplication");
NS_OBJECT_ENSURE_REGISTERED(IncidentSink);

TypeId
IncidentSink::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::IncidentSink")
			.SetParent<Application> ()
			.AddConstructor<IncidentSink> ()
			.AddAttribute("Port", "Port on which we listen for incoming packets.",
					UintegerValue (8089),
					MakeUintegerAccessor (&IncidentSink::m_port),
					MakeUintegerChecker<uint16_t> ())
			.AddAttribute("ConfirmationWeight", "Weight assigned to confirmed incidents.",
					DoubleValue (1.),
					MakeDoubleAccessor (&IncidentSink::m_confirmedIncWeight),
					MakeDoubleChecker<double> ())
	;
	return tid;
}

IncidentSink::IncidentSink ()
{
	NS_LOG_FUNCTION_NOARGS ();

	m_NConfirmations = 0;
	m_maliciousNode = false;

	srand (time (0));
}

IncidentSink::~IncidentSink ()
{
	NS_LOG_FUNCTION_NOARGS ();

	m_socket = 0;
	m_socketResp = 0;
}

void
IncidentSink::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
IncidentSink::StartApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();

	if ( m_socketResp == 0 )
	{
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socketResp = Socket::CreateSocket (GetNode (), tid);
		m_socketResp->Bind();
	}

	if ( m_socket == 0 )
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
		m_socket->Bind (local);
	}

	m_socket->SetRecvCallback (MakeCallback (&IncidentSink::HandleRead, this));

	DoubleValue selfishProb; GetNode ()->GetAttribute ("SelfishProb", selfishProb);
	m_maliciousNode = selfishProb.Get () == -1 ? true : false;
}

void
IncidentSink::StopApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();

	if ( m_socketResp != 0 )
	{
		m_socketResp->Close ();
	}

	if ( m_socket != 0 )
	{
		m_socket->Close ();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
	}
}

void
IncidentSink::SendConfirmation (Address remote)
{
	DoubleValue myReputation; GetNode()->GetAttribute ("Reputation", myReputation);
	DoubleValue mySelfishness; GetNode ()->GetAttribute ("SelfishProb", mySelfishness);

	std::string myReputationStr = myReputation.SerializeToString (MakeDoubleChecker<double> ());
	myReputationStr.append ("#");
	std::string mySelfishnessStr = mySelfishness.SerializeToString (MakeDoubleChecker<double> ());
	myReputationStr.append (mySelfishnessStr);
	myReputationStr.append ("#");
	Ptr<Packet> confirmationPkt = Create<Packet> (reinterpret_cast<const uint8_t*> (myReputationStr.c_str ()),
			myReputationStr.length ());

	m_socketResp->Connect (remote);
	m_socketResp->Send (confirmationPkt);

	++m_NConfirmations;

	Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
	Ipv4Address local = ipv4->GetAddress (1, 0).GetLocal ();

	NS_LOG_INFO ("+"<< Simulator::Now ().GetSeconds () << " " << local << " " << InetSocketAddress::ConvertFrom (remote).GetIpv4 () << " "
			<< "m=" << m_maliciousNode << " " << myReputationStr << " " << "[CONF_SEND]");
}

void
IncidentSink::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;

	while ( (packet = socket->RecvFrom (from)) )
	{
		Ipv4Address fromAddress = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
		ReputationTag tag;
		bool reputationUpdate = packet->RemovePacketTag (tag);

		if ( reputationUpdate ) // Received reputation update packet and the Node must update its reputation
		{
			uint8_t action = tag.GetDoAction ();
			std::string actionStr = action == 0 ? "INCREASE_REP" : "DECREASE_REP";

			Ipv4Address localhost = GetNode ()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
			NS_LOG_INFO ("-" << Simulator::Now ().GetSeconds () << " " << fromAddress << " " << localhost
					<< " " << "m=" << m_maliciousNode << " " << "a=" << actionStr << " " << "[REP_UPDATE]");

			if ( action == 0 ) {
				DoubleValue nValidIncidents, nInvalidIncidents;
				GetNode ()->GetAttribute ("ValidIncidents", nValidIncidents);
				GetNode ()->GetAttribute ("InvalidIncidents", nInvalidIncidents);
				double newValidIncidents = nValidIncidents.Get () + m_confirmedIncWeight;
				GetNode ()->SetAttribute ("ValidIncidents", DoubleValue (newValidIncidents));

				NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << localhost << " "
						<< "m=" << m_maliciousNode << " " << "a=" << actionStr << " "
						<< "alfa_b=" << nValidIncidents.Get () << " " << "alfa_a=" << newValidIncidents
						<< " " << "beta=" << nInvalidIncidents.Get () << " " << "[STATS]");

				DoubleValue myReputation; GetNode ()->GetAttribute ("Reputation", myReputation);
				if ( myReputation.Get () != 1 ) UpdateReputation ();
			}
			else if ( action == 1 ) {
				DoubleValue nValidIncidents, nInvalidIncidents;
				GetNode ()->GetAttribute ("ValidIncidents", nValidIncidents);
				GetNode ()->GetAttribute ("InvalidIncidents", nInvalidIncidents);
				double newInvalidIncidents = nInvalidIncidents.Get () + 1;
				GetNode ()->SetAttribute ("InvalidIncidents", DoubleValue (newInvalidIncidents));

				NS_LOG_INFO ("*" << Simulator::Now ().GetSeconds () << " " << localhost << " "
						<< "m=" << m_maliciousNode << " " << "a=" << actionStr << " "
						<< "alfa=" << nValidIncidents.Get () << " " << "beta_b=" << nInvalidIncidents.Get ()
						<< " " << "beta_a=" << newInvalidIncidents << " " << "[STATS]");

				UpdateReputation ();
			}
		}
		else { // Received broadcast message (i.e. an incident was generated)

			Ipv4Address localhost = GetNode ()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
			NS_LOG_INFO ("-" << Simulator::Now ().GetSeconds () << " " << fromAddress << " " << localhost
					<< " " << "m=" << m_maliciousNode << " " << "[BRD_RCVD]");

			DoubleValue selfishProb = DoubleValue (.0);
			bool shouldIConfirm = TossBiasedCoin(selfishProb.Get ());
			if ( shouldIConfirm ) {
				double delay = RandomNumberInterval (0.1, 0.5);
				Simulator::Schedule(Seconds (delay), &IncidentSink::SendConfirmation, this, from);
			}
			//SendConfirmation (from, Seconds (0));

			//NS_LOG_LOGIC ("Sending confirmation...");
			//socket->SendTo(packet, 0, from);
			//m_socketResp->Connect (from);
			//m_socketResp->Send (Create<Packet> (512));
		}
	}
}

void
IncidentSink::UpdateReputation (void)
{
	DoubleValue nValidIncidents;
	GetNode ()->GetAttribute ("ValidIncidents", nValidIncidents);
	DoubleValue nInvalidIncidents;
	GetNode ()->GetAttribute ("InvalidIncidents", nInvalidIncidents);

	double newReputationVal = (nValidIncidents.Get () + 1) /
			(nValidIncidents.Get () + nInvalidIncidents.Get () + 2);
	GetNode()->SetAttribute ("Reputation", DoubleValue (newReputationVal));

//	DoubleValue rep;
//	GetNode()->GetAttribute ("Reputation", rep);
//	NS_LOG_INFO ("Reputation value of Node " << GetNode()->GetId () <<
//			" changed and the new value is " << rep.Get ());
}

double
IncidentSink::RandomNumberUniform ()
{
	return rand () / double (RAND_MAX);
}

double
IncidentSink::RandomNumberInterval (double min, double max)
{
	return (max-min) * RandomNumberUniform () + min;
}

bool
IncidentSink::TossBiasedCoin (double bias)
{
	double randomNumber = RandomNumberUniform ();
	bool accept = randomNumber < bias ? false : true;
	return accept;
}

} // namespace ns3


