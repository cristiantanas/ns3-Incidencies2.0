/*
 * incidencies-sink-application.h
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

#ifndef INCIDENT_SINK_APPLICATION_H_
#define INCIDENT_SINK_APPLICATION_H_

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"


namespace ns3 {

class Packet;
class Socket;

class IncidentSink : public Application
{
public:
	static TypeId GetTypeId (void);

	IncidentSink ();
	virtual ~IncidentSink ();

protected:
	virtual void DoDispose (void);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendConfirmation (Address remote);

	void HandleRead (Ptr<Socket> socket);

	void UpdateReputation (void);

	double RandomNumberUniform ();
	double RandomNumberInterval (double min, double max);
	bool TossBiasedCoin (double bias);

	uint16_t		m_port;
	Address			m_addressLocal;
	Ptr<Socket>		m_socket;

	Ptr<Socket>		m_socketResp;

	uint32_t		m_NConfirmations;

	double			m_confirmedIncWeight;

	bool			m_maliciousNode;
};

} // namespace ns3


#endif /* INCIDENT_SINK_APPLICATION_H_ */
