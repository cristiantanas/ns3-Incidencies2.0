/*
 * incidencies-helper.h
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

#ifndef INCIDENCIES_HELPER_H_
#define INCIDENCIES_HELPER_H_

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class IncidentSinkHelper
{
public:
	IncidentSinkHelper (uint16_t port);
	void SetAttribute (std::string name, const AttributeValue &value);

	ApplicationContainer Install (Ptr<Node> node) const;
	ApplicationContainer Install (std::string nodeName) const;
	ApplicationContainer Install (NodeContainer c) const;

private:
	Ptr<Application> InstallPriv (Ptr<Node> node) const;

	ObjectFactory m_factory;
};

class IncidentGeneratorHelper
{
public:
	IncidentGeneratorHelper (uint16_t port);
	void SetAttribute (std::string name, const AttributeValue &value);

	ApplicationContainer Install (Ptr<Node> node) const;
	ApplicationContainer Install (std::string nodeName) const;
	ApplicationContainer Install (NodeContainer c) const;

private:
	Ptr<Application> InstallPriv (Ptr<Node> node) const;

	ObjectFactory m_factory;
};

} // namespace ns3


#endif /* INCIDENCIES_HELPER_H_ */
