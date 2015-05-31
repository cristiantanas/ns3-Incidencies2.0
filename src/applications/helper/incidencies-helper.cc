/*
 * incidencies-helper.cc
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

#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/incident-generator-application.h"
#include "ns3/incident-sink-application.h"

#include "incidencies-helper.h"

namespace ns3 {

/*
 * INCIDENT_SINK_HELPER IMPLEMENTED METHDOS -- INCIDENT_SINK_APPLICATION
 */

IncidentSinkHelper::IncidentSinkHelper (uint16_t port)
{
	m_factory.SetTypeId (IncidentSink::GetTypeId ());
	SetAttribute ("Port", UintegerValue (port));
}

void
IncidentSinkHelper::SetAttribute(std::string name, const AttributeValue &value)
{
	m_factory.Set(name, value);
}

ApplicationContainer
IncidentSinkHelper::Install(Ptr<Node> node) const
{
	return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
IncidentSinkHelper::Install(std::string nodeName) const
{
	Ptr<Node> node = Names::Find<Node> (nodeName);
	return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
IncidentSinkHelper::Install(NodeContainer c) const
{
	ApplicationContainer apps;
	for ( NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
	{
		apps.Add (InstallPriv (*i));
	}

	return apps;
}

Ptr<Application>
IncidentSinkHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<IncidentSink> ();
  node->AddApplication (app);

  return app;
}

/*
 * INCIDENT_GENERATOR_HELPER IMPLEMENTED METHODS -- INCIDENT_GENERATOR_APPLICATION
 */

IncidentGeneratorHelper::IncidentGeneratorHelper (uint16_t port)
{
	m_factory.SetTypeId (IncidentGenerator::GetTypeId ());
	SetAttribute ("RemotePort", UintegerValue (port));
}

void
IncidentGeneratorHelper::SetAttribute(std::string name, const AttributeValue &value)
{
	m_factory.Set(name, value);
}

ApplicationContainer
IncidentGeneratorHelper::Install(Ptr<Node> node) const
{
	return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
IncidentGeneratorHelper::Install(std::string nodeName) const
{
	Ptr<Node> node = Names::Find<Node> (nodeName);
	return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
IncidentGeneratorHelper::Install(NodeContainer c) const
{
	ApplicationContainer apps;
	for ( NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
	{
		apps.Add (InstallPriv (*i));
	}

	return apps;
}

Ptr<Application>
IncidentGeneratorHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<IncidentGenerator> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3



