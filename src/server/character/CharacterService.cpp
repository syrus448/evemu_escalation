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

#include "EvemuPCH.h"

PyCallable_Make_InnerDispatcher(CharacterService)

CharacterService::CharacterService(PyServiceMgr *mgr, DBcore *dbc)
: PyService(mgr, "character"),
  m_dispatch(new Dispatcher(this)),
  m_db(dbc)
{
	_SetCallDispatcher(m_dispatch);

	PyCallable_REG_CALL(CharacterService, GetCharactersToSelect)
	PyCallable_REG_CALL(CharacterService, GetCharacterToSelect)
	PyCallable_REG_CALL(CharacterService, SelectCharacterID)
	PyCallable_REG_CALL(CharacterService, GetOwnerNoteLabels)
	PyCallable_REG_CALL(CharacterService, GetCharCreationInfo)
	PyCallable_REG_CALL(CharacterService, GetCharNewExtraCreationInfo)
	PyCallable_REG_CALL(CharacterService, GetAppearanceInfo)
	PyCallable_REG_CALL(CharacterService, ValidateName)
	PyCallable_REG_CALL(CharacterService, ValidateNameEx)
	PyCallable_REG_CALL(CharacterService, CreateCharacter2) // replica of CreateCharacter for now.
	PyCallable_REG_CALL(CharacterService, Ping)
	PyCallable_REG_CALL(CharacterService, PrepareCharacterForDelete)
	PyCallable_REG_CALL(CharacterService, CancelCharacterDeletePrepare)
	PyCallable_REG_CALL(CharacterService, AddOwnerNote)
	PyCallable_REG_CALL(CharacterService, EditOwnerNote)
	PyCallable_REG_CALL(CharacterService, GetOwnerNote)
	PyCallable_REG_CALL(CharacterService, GetHomeStation)
	PyCallable_REG_CALL(CharacterService, GetCloneTypeID)
	PyCallable_REG_CALL(CharacterService, GetCharacterAppearanceList)
	PyCallable_REG_CALL(CharacterService, GetRecentShipKillsAndLosses)

	PyCallable_REG_CALL(CharacterService, GetCharacterDescription)
	PyCallable_REG_CALL(CharacterService, SetCharacterDescription)

	PyCallable_REG_CALL(CharacterService, GetNote)
	PyCallable_REG_CALL(CharacterService, SetNote)

	PyCallable_REG_CALL(CharacterService, LogStartOfCharacterCreation)
}

CharacterService::~CharacterService() {
	delete m_dispatch;
}

PyResult CharacterService::Handle_GetCharactersToSelect(PyCallArgs &call) {
	return(m_db.GetCharacterList(call.client->GetAccountID()));
}

PyResult CharacterService::Handle_GetCharacterToSelect(PyCallArgs &call) {
	Call_SingleIntegerArg args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "Invalid arguments");
		return NULL;
	}

	PyRep *result = m_db.GetCharSelectInfo(args.arg);
	if(result == NULL) {
		_log(CLIENT__ERROR, "Failed to load character %d", args.arg);
		return NULL;
	}

	return(result);
}

PyResult CharacterService::Handle_SelectCharacterID(PyCallArgs &call) {
	CallSelectCharacterID args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "Failed to parse args from account %u", call.client->GetAccountID());
		//TODO: throw exception
		return NULL;
	}

	//we don't care about tutorial dungeon right now
	call.client->SelectCharacter(args.charID);

	return NULL;
}

PyResult CharacterService::Handle_GetOwnerNoteLabels(PyCallArgs &call) {
	return m_db.GetOwnerNoteLabels(call.client->GetCharacterID());
}

PyResult CharacterService::Handle_GetCharCreationInfo(PyCallArgs &call) {
	PyRepDict *result = new PyRepDict();

	//send all the cache hints needed for char creation.
	m_manager->cache_service->InsertCacheHints(
		ObjCacheService::hCharCreateCachables,
		result);
	_log(CLIENT__MESSAGE, "Sending char creation info reply");

	return(result);
}

PyResult CharacterService::Handle_GetCharNewExtraCreationInfo(PyCallArgs &call) {
	PyRepDict *result = new PyRepDict();

	m_manager->cache_service->InsertCacheHints(
		ObjCacheService::hCharNewExtraCreateCachables,
		result);
	_log(CLIENT__MESSAGE, "Sending char new extra creation info reply");

	return(result);
}

PyResult CharacterService::Handle_GetAppearanceInfo(PyCallArgs &call) {
	PyRepDict *result = new PyRepDict();

	//send all the cache hints needed for char creation.
	m_manager->cache_service->InsertCacheHints(
		ObjCacheService::hAppearanceCachables,
		result );

	_log(CLIENT__MESSAGE, "Sending appearance info reply");

	return(result);
}

PyResult CharacterService::Handle_ValidateName(PyCallArgs &call) {
	Call_SingleStringArg arg;
	if(!arg.Decode(&call.tuple)) {
		_log(SERVICE__ERROR, "Failed to decode args.");
		return NULL;
	}

	// perform checks on the "name" that is passed.  we may want to impliment something
	// to limit the kind of names allowed.

	return(new PyRepBoolean(m_db.ValidateCharName(arg.arg.c_str())));
}

PyResult CharacterService::Handle_ValidateNameEx(PyCallArgs &call) {
	//just redirect it now
	return(Handle_ValidateName(call));
}

PyResult CharacterService::Handle_CreateCharacter2(PyCallArgs &call) {
	CallCreateCharacter2 arg;
	if(!arg.Decode(&call.tuple)) {
		_log(CLIENT__ERROR, "Failed to decode CreateCharacter2 arg.");
		return NULL;
	}

	_log(CLIENT__MESSAGE, "CreateCharacter2 called for '%s'", arg.name.c_str());
	_log(CLIENT__MESSAGE, "  bloodlineID=%u genderID=%u ancestryID=%u",
			arg.bloodlineID, arg.genderID, arg.ancestryID);

	// obtain character type
	const CharacterType *char_type = m_manager->item_factory.GetCharacterTypeByBloodline(arg.bloodlineID);
	if(char_type == NULL)
		return NULL;

	// we need to fill these to successfully create character item
	ItemData idata;
	CharacterData cdata;
	CharacterAppearance capp;
	CorpMemberInfo corpData;

	idata.typeID = char_type->id();
	idata.name = arg.name;
	idata.ownerID = 1; // EVE System
	idata.quantity = 1;
	idata.singleton = true;

	cdata.accountID = call.client->GetAccountID();
	cdata.gender = arg.genderID;
	cdata.ancestryID = arg.ancestryID;

	// just hack something
	cdata.careerID = 11;
	cdata.careerSpecialityID = 11;

	corpData.corpRole = 0;
	corpData.rolesAtAll = 0;
	corpData.rolesAtBase = 0;
	corpData.rolesAtHQ = 0;
	corpData.rolesAtOther = 0;

	// Variables for storing attribute bonuses
	uint8 intelligence = char_type->intelligence();
	uint8 charisma = char_type->charisma();
	uint8 perception = char_type->perception();
	uint8 memory = char_type->memory();
	uint8 willpower = char_type->willpower();

	// Setting character's starting position, and getting it's location...
	// Also query attribute bonuses from ancestry
	if(    !m_db.GetLocationCorporationByCareer(cdata)
		|| !m_db.GetAttributesFromAncestry(cdata.ancestryID, intelligence, charisma, perception, memory, willpower)
	) {
		codelog(CLIENT__ERROR, "Failed to load char create details. Bloodline %u, ancestry %u.",
			char_type->bloodlineID(), cdata.ancestryID);
		return NULL;
	}

	//NOTE: these are currently hard coded to Todaki because other things are
	//also hard coded to only work in Todaki. Once these various things get fixed,
	//just take this code out and the above calls should have cdata populated correctly.
	cdata.stationID = idata.locationID = 60004420;
	cdata.solarSystemID = 30001407;
	cdata.constellationID = 20000206;
	cdata.regionID = 10000016;

	cdata.bounty = 0;
	cdata.balance = sConfig.server.startBalance;
	cdata.securityRating = 0;
	cdata.logonMinutes = 0;
	cdata.title = "No Title";

	cdata.startDateTime = Win32TimeNow();
	cdata.createDateTime = cdata.startDateTime;
	cdata.corporationDateTime = cdata.startDateTime;

	//this builds appearance data from strdict
	capp.Build(arg.appearance);

	typedef std::map<uint32, uint32>        CharSkillMap;
	typedef CharSkillMap::iterator          CharSkillMapItr;

	//load skills
	CharSkillMap startingSkills;
	if( !m_db.GetSkillsByRace(char_type->race(), startingSkills) )
	{
		codelog(CLIENT__ERROR, "Failed to load char create skills. Bloodline %u, Ancestry %u.",
			char_type->bloodlineID(), cdata.ancestryID);
		return NULL;
	}

	//now we have all the data we need, stick it in the DB
	//create char item
	Character *char_item = m_manager->item_factory.SpawnCharacter(idata, cdata, capp, corpData);
	if(char_item == NULL) {
		//a return to the client of 0 seems to be the only means of marking failure
		codelog(CLIENT__ERROR, "Failed to create character '%s'", idata.name.c_str());
		return NULL;
	}

	// add attribute bonuses
	// (use Set_##_persist to properly persist attributes into DB)
	char_item->Set_intelligence_persist(intelligence);
	char_item->Set_charisma_persist(charisma);
	char_item->Set_perception_persist(perception);
	char_item->Set_memory_persist(memory);
	char_item->Set_willpower_persist(willpower);

	// register name
	m_db.add_name_validation_set(char_item->itemName().c_str(), char_item->itemID());

	//spawn all the skills
	CharSkillMapItr cur, end;
	cur = startingSkills.begin();
	end = startingSkills.end();
	for(; cur != end; cur++) 
	{
		ItemData skillItem( cur->first, char_item->itemID(), char_item->itemID(), flagSkill );
		InventoryItem *i = m_manager->item_factory.SpawnItem( skillItem );
		if(i == NULL) {
			_log(CLIENT__ERROR, "Failed to add skill %u to char %s (%u) during char create.", cur->first, char_item->itemName().c_str(), char_item->itemID());
			continue;
		}

		_log(CLIENT__MESSAGE, "Training skill %u to level %d (%d points)", i->typeID(), i->skillLevel(), i->skillPoints());
		i->Set_skillLevel(cur->second);
		i->Set_skillPoints(GetSkillPointsForSkillLevel(i, cur->second));

		//we don't actually need the item anymore...
		i->DecRef();
	}

	//now set up some initial inventory:
	InventoryItem *initInvItem;

	// add "Damage Control I"
	ItemData itemDamageControl( 2046, char_item->itemID(), char_item->locationID(), flagHangar, 1 );
	initInvItem = m_manager->item_factory.SpawnItem( itemDamageControl );
	if(initInvItem == NULL)
		codelog(CLIENT__ERROR, "%s: Failed to spawn a starting item", char_item->itemName().c_str());
	else
		initInvItem->DecRef();

	// add 1 unit of "Tritanium"
	ItemData itemTritanium( 34, char_item->itemID(), char_item->locationID(), flagHangar, 1 );
	initInvItem = m_manager->item_factory.SpawnItem( itemTritanium );

	if(initInvItem == NULL)
		codelog(CLIENT__ERROR, "%s: Failed to spawn a starting item", char_item->itemName().c_str());
	else
		initInvItem->DecRef();

	// give the player its ship.
	std::string ship_name = char_item->itemName() + "'s Ship";

	ItemData shipItem( char_type->shipTypeID(), char_item->itemID(), char_item->locationID(), flagHangar, ship_name.c_str() );
	InventoryItem *ship_item = m_manager->item_factory.SpawnItem(shipItem);

	if(ship_item == NULL)
	{
		codelog(CLIENT__ERROR, "%s: Failed to spawn a starting item", char_item->itemName().c_str());
	}
	else
	{
		//welcome on board your starting ship
		char_item->MoveInto(ship_item, flagPilot, false);
		ship_item->DecRef();
	}

	uint32 characterID = char_item->itemID();

	//recursively save everything we just did.
	char_item->Save(true);
	char_item->DecRef();

	_log(CLIENT__MESSAGE, "Sending char create ID %u as reply", characterID);

	return new PyRepInteger(characterID);
}

PyResult CharacterService::Handle_Ping(PyCallArgs &call)
{
	return(new PyRepInteger(call.client->GetAccountID()));
}

PyResult CharacterService::Handle_PrepareCharacterForDelete(PyCallArgs &call) {
	/*
	 * Deletion is apparently a timed process, and is related to the
	 * deletePrepareDateTime column in the char select result.
	 *
	 * For now, we will just implement an immediate delete as its better than
	 * nothing, since clearing out all the tables by hand is a nightmare.
	 */

	Call_SingleIntegerArg args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "%s: failed to decode arguments", call.client->GetName());
		return NULL;
	}

	//TODO: make sure this person actually owns this char...

	_log(CLIENT__MESSAGE, "Timed delete of char %u unimplemented. Deleting Immediately.", args.arg);

	{ // character scope to make sure char_item is no longer accessed after deletion
		Character *char_item = m_manager->item_factory.GetCharacter(args.arg, true);
		if(char_item == NULL) {
			codelog(CLIENT__ERROR, "Failed to load char item %u.", args.arg);
			return NULL;
		}
		//unregister name
		m_db.del_name_validation_set(char_item->itemID());
		//does the recursive delete of all contained items
		char_item->Delete();
	}

	//now, clean up all items which werent deleted
	std::vector<uint32> items;
	if(!m_db.GetCharItems(args.arg, items)) {
		codelog(CLIENT__ERROR, "Unable to get items of char %u.", args.arg);
		return NULL;
	}

	std::vector<uint32>::iterator cur, end;
	cur = items.begin();
	end = items.end();
	for(; cur != end; cur++) {
		InventoryItem *i = m_manager->item_factory.GetItem(*cur, true);
		if(i == NULL) {
			codelog(CLIENT__ERROR, "Failed to load item %u to delete. Skipping.", *cur);
			continue;
		}

		i->Delete();
	}

	//we return deletePrepareDateTime, in eve time format.
	return(new PyRepInteger(Win32TimeNow() + Win32Time_Second*5));
}

PyResult CharacterService::Handle_CancelCharacterDeletePrepare(PyCallArgs &call) {
	Call_SingleIntegerArg args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "%s: failed to decode arguments", call.client->GetName());
		return NULL;
	}

	_log(CLIENT__ERROR, "Cancel delete (of char %u) unimplemented.", args.arg);

	//returns nothing.
	return NULL;
}

PyResult CharacterService::Handle_AddOwnerNote(PyCallArgs &call) {
	Call_AddOwnerNote args;
	if (!args.Decode(&call.tuple)) {
		codelog(SERVICE__ERROR, "%s: Bad arguments", call.client->GetName());
		return(new PyRepInteger(0));
	}

	uint32 noteID = m_db.AddOwnerNote(call.client->GetCharacterID(), args.label, args.content);
	if (noteID == 0) {
		codelog(SERVICE__ERROR, "%s: Failed to set Owner note.", call.client->GetName());
		return (new PyRepNone());
	}

	return(new PyRepInteger(noteID));
}

PyResult CharacterService::Handle_EditOwnerNote(PyCallArgs &call) {
	Call_EditOwnerNote args;
	if (!args.Decode(&call.tuple)) {
		codelog(SERVICE__ERROR, "%s: Bad arguments", call.client->GetName());
		return(new PyRepInteger(0));
	}

	if (!m_db.EditOwnerNote(call.client->GetCharacterID(), args.noteID, args.label, args.content)) {
		codelog(SERVICE__ERROR, "%s: Failed to set Owner note.", call.client->GetName());
		return (new PyRepNone());
	}

	return(new PyRepInteger(args.noteID));
}

PyResult CharacterService::Handle_GetOwnerNote(PyCallArgs &call) {
	Call_SingleIntegerArg arg;
	if (!arg.Decode(&call.tuple)) {
		codelog(SERVICE__ERROR, "%s: Bad arguments", call.client->GetName());
		return(new PyRepInteger(0));
	}
	return m_db.GetOwnerNote(call.client->GetCharacterID(), arg.arg);
}


PyResult CharacterService::Handle_GetHomeStation(PyCallArgs &call) {
	//returns the station ID of the station where this player's Clone is
	PyRep *result = NULL;

	_log(CLIENT__ERROR, "GetHomeStation() is not really implemented.");

	result = new PyRepInteger(call.client->GetStationID());

	return(result);
}

PyResult CharacterService::Handle_GetCloneTypeID(PyCallArgs &call) {
	PyRep *result = NULL;

	_log(CLIENT__ERROR, "GetCloneTypeID() is not really implemented.");

	//currently hardcoded Clone Grade Alpha
	result = new PyRepInteger(164);

	return(result);
}

PyResult CharacterService::Handle_GetRecentShipKillsAndLosses(PyCallArgs &call) {
	_log(SERVICE__WARNING, "%s::GetRecentShipKillsAndLosses unimplemented.", GetName());

	util_Rowset rs;

	rs.header.push_back("killID");
	rs.header.push_back("solarSystemID");
	rs.header.push_back("victimCharacterID");
	rs.header.push_back("victimCorporationID");
	rs.header.push_back("victimAllianceID");
	rs.header.push_back("victimFactionID");
	rs.header.push_back("victimShipTypeID");
	rs.header.push_back("finalCharacterID");
	rs.header.push_back("finalCorporationID");
	rs.header.push_back("finalAllianceID");
	rs.header.push_back("finalFactionID");
	rs.header.push_back("finalShipTypeID");
	rs.header.push_back("finalWeaponTypeID");
	rs.header.push_back("killBlob");	//string
	rs.header.push_back("killTime");	//uint64
	rs.header.push_back("victimDamageTaken");
	rs.header.push_back("finalSecurityStatus");	//real
	rs.header.push_back("finalDamageDone");
	rs.header.push_back("moonID");

	return(rs.FastEncode());
}

/////////////////////////
PyResult CharacterService::Handle_GetCharacterDescription(PyCallArgs &call) {
	Call_SingleIntegerArg args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "Invalid arguments");
		return NULL;
	}

	Character *c = m_manager->item_factory.GetCharacter(args.arg);
	if(c == NULL) {
		_log(CLIENT__ERROR, "Failed to load character %u.", args.arg);
		return NULL;
	}

	PyRep *result = c->GetDescription();
	c->DecRef();

	return result;
}

//////////////////////////////////
PyResult CharacterService::Handle_SetCharacterDescription(PyCallArgs &call) {
	Call_SingleStringArg args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "Invalid arguments");
		return NULL;
	}

	Character *c = call.client->Char();
	if(c == NULL) {
		_log(CLIENT__ERROR, "SetCharacterDescription called with no char!");
		return NULL;
	}

	c->SetDescription(args.arg.c_str());

	return NULL;
}

PyResult CharacterService::Handle_GetCharacterAppearanceList(PyCallArgs &call) {
	Call_SingleIntList args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "%s: Invalid arguments", call.client->GetName());
		return NULL;
	}

	PyRepList *l = new PyRepList();

	std::vector<uint32>::iterator cur, end;
	cur = args.ints.begin();
	end = args.ints.end();
	for(; cur != end; cur++) {
		PyRep *r = m_db.GetCharacterAppearance(*cur);
		if(r == NULL)
			r = new PyRepNone();

		l->add(r);
	}

	return(l);
}

/** Retrieves a Character note.
*
* Checks the database for a given character note and returns it.
*
* @return PyRepSubStream pointer with the string with the note or with PyRepNone if no note is stored.
*
*  **LSMoura
*/
PyResult CharacterService::Handle_GetNote(PyCallArgs &call) {
	Call_SingleIntegerArg args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "Invalid arguments");
		return NULL;
	}

	return(m_db.GetNote(call.client->GetCharacterID(), args.arg));
}

/** Stores a Character note.
*
*  Stores the given character note in the database.
*
* @return PyRepSubStream pointer with a PyRepNone() value.
*
*  **LSMoura
*/
PyResult CharacterService::Handle_SetNote(PyCallArgs &call) {
	Call_SetNote args;
	if(!args.Decode(&call.tuple)) {
		codelog(CLIENT__ERROR, "Invalid arguments");
		return NULL;
	}

	if (!m_db.SetNote(call.client->GetCharacterID(), args.itemID, args.note.c_str()))
		codelog(CLIENT__ERROR, "Error while storing the note on the database.");

	return NULL;
}

uint32 CharacterService::GetSkillPointsForSkillLevel(InventoryItem *i, uint8 level)
{
	if (i == NULL)
		return 0;

	float fLevel = level;
	fLevel = (fLevel - 1.0) * 0.5;

	uint32 calcSkillPoints = SKILL_BASE_POINTS * i->skillTimeConstant() * pow(32, fLevel);
	return calcSkillPoints;
}

PyResult CharacterService::Handle_LogStartOfCharacterCreation(PyCallArgs &call)
{
	/* we seem to send nothing back. */
	return NULL;
}
