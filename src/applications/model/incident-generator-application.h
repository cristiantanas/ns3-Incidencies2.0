/*
 * incidencies-generator-application.h
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

#ifndef INCIDENT_GENERATOR_APPLICATION_H_
#define INCIDENT_GENERATOR_APPLICATION_H_

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/timer.h"
#include "ns3/tag.h"

#include <map>

#define CONFIRMATION_THRESHOLD 1
#define REPUTATION_THRESHOLD 0.9

#define ABSOLUTE_VALUE_MODE 0
#define DENSITY_FUNCTION_MODE 1
#define WEIGHT_FUNCTION_MODE 2

#define LINEAR_WEIGHT_FUN 21
#define EXP_WEIGHT_FUN 22
#define QUADRATIC_WEIGHT_FUN 23
#define TABLE_WEIGHT_FUN 24

#define INCREASE_REPUTATION 1
#define DECREASE_REPUTATION -1
#define DO_NOTHING 0

namespace ns3 {

class Socket;
class Packet;

class IncidentGenerator : public Application
{
public:
	static TypeId GetTypeId (void);

	IncidentGenerator ();
	virtual ~IncidentGenerator ();

	void GenerateNewIncident (Time dt);

protected:
	virtual void DoDispose (void);

private:

	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendBroadcast (void);
	void SendReputationUpdate (uint8_t action);

	void HandleConfirmations (Ptr<Socket> socket);
	void AllConfirmationsReceived (void);
	uint32_t ValidateIncidentWithMode (uint32_t validationMode);

	void UpdateNodeReputation (void);

	bool TossBiasedCoin (double bias);
	double GetConfirmationWeight (double reputationVal);

	Time		m_startOffset;	// Time interval before generating any incident

	uint32_t	m_sent;			// Number of incidents generated

	Ptr<Socket>	m_socket;		// Transmission socket
	uint16_t	m_remotePort;

	EventId		m_sendEvent;

	Timer		m_timer;		// Timer to wait for broadcast confirmations
	Time		m_timerDelay;	// Timer schedule delay

	std::vector<Address> 			m_confirmationArray;
	std::vector<Address>			m_neighbours;
	std::map<Ipv4Address, double> 	m_reputationMap;
	uint8_t 						m_atLeastOneUserWithHR;

	uint32_t	m_NGeneratedIncidents;	// Nombre d'incidències generades

	double 		m_selfishProb;			// Probabilitat de que un Node sigui 'selfish'
	std::string	m_blackList;			// String amb una llista dels Nodes que sempre son 'selfish'
	std::vector<Ipv4Address>	m_blackListIpv4;

	uint32_t	m_validationMode;
	uint32_t	m_weightFunction;
	double		m_confirmationThreshold;
	double		m_falseIncidentThreshold;
	double		m_reputationThreshold;

	double		m_confirmedIncWeight;
	double 		m_generatedIncWeight;

	bool 		m_maliciousNode;
};


class ReputationTag : public Tag
{
public:
	ReputationTag ();
	void SetDoAction (uint8_t action);
	uint8_t GetDoAction (void) const;

	static TypeId GetTypeId (void);
	virtual TypeId GetInstanceTypeId (void) const;
	virtual uint32_t GetSerializedSize (void) const;
	virtual void Serialize (TagBuffer i) const;
	virtual void Deserialize (TagBuffer i);
	virtual void Print (std::ostream &os) const;

private:
	uint8_t		m_doAction;
};

} // namespace ns3


#endif /* INCIDENT_GENERATOR_APPLICATION_H_ */
