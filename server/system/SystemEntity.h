/*
	------------------------------------------------------------------------------------
	LICENSE:
	------------------------------------------------------------------------------------
	This file is part of EVEmu: EVE Online Server Emulator
	Copyright 2006 - 2008 The EVEmu Team
	For the latest information visit http://evemu.mmoforge.org
	------------------------------------------------------------------------------------
	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free Software
	Foundation; either version 2 of the License, or (at your option) any later
	version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place - Suite 330, Boston, MA 02111-1307, USA, or go to
	http://www.gnu.org/copyleft/lesser.txt.
	------------------------------------------------------------------------------------
	Author:		Zhur
*/

#ifndef __SYSTEMENTITY_H_INCL__
#define __SYSTEMENTITY_H_INCL__

#include "../ship/TargetManager.h"
#include <vector>


class PyRepDict;
class PyRepList;
class PyRepTuple;
class DoDestiny_AddBall;
class DoDestinyDamageState;
class DBSystemEntity;
class InventoryItem;
class SystemDB;
class GPoint;
class Damage;
class SystemBubble;
class SystemManager;

class Client;
class NPC;


class SystemEntity {
public:
	typedef enum {
		ecClient,
		ecNPC,
		ecCelestial,
		ecOther
	} EntityClass;
	
	SystemEntity();
	virtual ~SystemEntity() {}
	
	TargetManager targets;
	
	virtual void Process();
	virtual void ProcessDestiny() = 0;

	//this is a bit crude, but I prefer this over RTTI.
	virtual EntityClass GetClass() const { return(ecOther); }
	virtual bool IsClient() const { return false; }
	virtual bool IsNPC() const { return false; }
	virtual bool IsCelestial() const { return false; }
	virtual Client *CastToClient() { return NULL; }
	virtual const Client *CastToClient() const { return NULL; }
	virtual NPC *CastToNPC() { return NULL; }
	virtual const NPC *CastToNPC() const { return NULL; }
	
	virtual bool IsStaticEntity() const { return true; }	//will this entity's position never change?
	virtual bool IsVisibleSystemWide() const { return false; }
	
	inline SystemBubble *Bubble() const { return(m_bubble); }	//may be NULL
	
	//may consume the arguments, or not.
	virtual void QueueDestinyUpdate(PyRepTuple **du) = 0;
	virtual void QueueDestinyEvent(PyRepTuple **multiEvent) = 0;
	
	//I am not sure if I want this here...
	virtual InventoryItem *Item() const = 0;
	
	virtual SystemManager *System() const = 0;	//may yeild NULL
	
	//expand the vector as needed, and encode the destiny update into it.
	virtual void EncodeDestiny(std::vector<uint8> &into) const = 0;
	//return ownership of a new foo.SlimItem dict
	virtual PyRepDict *MakeSlimItem() const = 0;
	//fill in the supplied damage state object.
	virtual void MakeDamageState(DoDestinyDamageState &into) const = 0;
	//return ownership of a new damage state tuple (calls MakeDamageState)
	PyRepTuple *MakeDamageState() const;
	//get the item ID of this entity
	virtual uint32 GetID() const = 0;
	//get the position of this entity in space.
	virtual const GPoint &GetPosition() const = 0;
	//get other attributes of the entity:
	virtual const char *GetName() const = 0;
	virtual double GetRadius() const = 0;
	
	//Im not happy with these being here..
	virtual void TargetAdded(SystemEntity *who) = 0;
	virtual void TargetLost(SystemEntity *who) = 0;
	virtual void TargetedAdd(SystemEntity *who) = 0;
	virtual void TargetedLost(SystemEntity *who) = 0;
	virtual void TargetsCleared() = 0;

	//process outgoing damage, applying any bonuses...
	virtual void ApplyDamageModifiers(Damage &d, SystemEntity *target) = 0;
	//process incoming damage, returns true on death.
	virtual bool ApplyDamage(Damage &d) = 0;
	//handles death.
	virtual void Killed(Damage &fatal_blow);
	
	
	virtual void MakeAddBall(DoDestiny_AddBall &into, uint32 updateID) const;

	//helpers:
	double DistanceTo2(const SystemEntity *other) const;
	
protected:
	
private:
	friend class SystemBubble;	//only to update m_bubble
	SystemBubble *m_bubble;	//we do not own this, may be NULL. Only changed by SystemBubble
};

class ItemSystemEntity : public SystemEntity {
public:
	ItemSystemEntity(InventoryItem *self=NULL);
	virtual ~ItemSystemEntity();
	
	//Default implementations fall to m_self.
	virtual uint32 GetID() const;
	virtual InventoryItem *Item() const { return(m_self); }
	virtual const char *GetName() const;
	virtual const GPoint &GetPosition() const;
	virtual double GetRadius() const;
	virtual PyRepDict *MakeSlimItem() const;
	virtual void MakeDamageState(DoDestinyDamageState &into) const;
	
	//process outgoing damage, applying any bonuses...
	virtual void ApplyDamageModifiers(Damage &d, SystemEntity *target);
	//process incoming damage, returns true on death.
	virtual bool ApplyDamage(Damage &d);
	
protected:
	InventoryItem *m_self;	// we have a ref of this
	
	void _SendDamageStateChanged() const;
	void _SetSelf(InventoryItem *);
};


class DestinyManager;

class DynamicSystemEntity : public ItemSystemEntity {
public:
	DynamicSystemEntity(DestinyManager *mgr=NULL /*ownership taken*/, InventoryItem *self=NULL /* takes ref */);
	virtual ~DynamicSystemEntity();

	//partial implementation of SystemEntity interface:
	virtual void ProcessDestiny();
	virtual const GPoint &GetPosition() const;
	virtual void EncodeDestiny(std::vector<uint8> &into) const;
	virtual bool IsStaticEntity() const { return false; }

	//Added interface:
	//get the corporation of this entity
	virtual uint32 GetCorporationID() const = 0;
	virtual uint32 GetAllianceID() const = 0;
	virtual double GetMass() const;
	virtual double GetMaxVelocity() const;
	virtual double GetAgility() const;
	
	inline DestinyManager *Destiny() const { return(m_destiny); }
	
	virtual void Killed(Damage &fatal_blow);
	
protected:
	DestinyManager *m_destiny;	//we own this! NULL if we are not in a system
};


#endif




