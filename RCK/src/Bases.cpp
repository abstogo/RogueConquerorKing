#include "Bases.h"

BaseManager* BaseManager::LoadBaseData()
{
	gLog->Log("Base Loader", "Started");
	
	std::string baseFilename = "scripts/bases.json";
	std::ifstream is(baseFilename);

	BaseManager* output = new BaseManager(jsoncons::decode_json<BaseInfoSet>(is));

	is.close();

	gLog->Log("Base Loader", "Decoded " + baseFilename);

	// feed reverse lookup
	for (int i = 0; i < output->baseInfoSet.BaseTypes().size(); i++)
	{
		const BaseType& it = output->baseInfoSet.BaseTypes()[i];
		output->reverseBaseTypeDictionary[it.Name()] = i;
	}

	for (int i = 0; i < output->baseInfoSet.Tags().size(); i++)
	{
		const BaseTag& it = output->baseInfoSet.Tags()[i];
		output->reverseBaseTagDictionary[it.Tag()] = i;
	}
	
	output->nextBaseID = 0;

	gLog->Log("Base Loader", "Completed");
	
	return output;
}

int BaseManager::shellGenerate()
{
	int output = nextBaseID++;

	DebugLog("Generating Empty Base #" + std::to_string(output));

	baseType.push_back(0); // default is camp
	baseType.push_back(-1);
	baseXPos.push_back(-1);
	baseYPos.push_back(-1);
	basePartyID.push_back(-1);

	return output;
}

void BaseManager::AddPlayerCharacter(int characterID)
{
	playerCharacters.push_back(characterID);
	DebugLog("PC (character ID #" + std::to_string(characterID) + ") added to party");
}

void BaseManager::AddHenchman(int characterID)
{
	henchmen.push_back(characterID);
	DebugLog("Henchman (character ID #" + std::to_string(characterID) + ") added to party");
}

void BaseManager::AddAnimal(int mobID)
{
	animals.push_back(mobID);
	DebugLog("Animal (mob ID #" + std::to_string(mobID) + ") added to party");
}

void BaseManager::RemoveCharacter(int entityID)
{
	if(std::remove(playerCharacters.begin(), playerCharacters.end(), entityID) == playerCharacters.end())
	{
		// not found in the PC set, check Henches too
		std::remove(henchmen.begin(), henchmen.end(), entityID);
	}
}

void BaseManager::RemoveAnimal(int entityID)
{
	std::remove(animals.begin(), animals.end(), entityID);
}

int BaseManager::GetBaseAt(int find_x, int find_y)
{
	int output = -1;
	int i = 0;
	for (int x : baseXPos)
	{
		if (x == find_x)
		{
			if (baseYPos[i] == find_y)
			{
				output = i;
				break;
			}
		}
		i++;
	}

	return output;
}

std::string BaseManager::GetBaseType(int baseID)
{
	int bT = baseType[baseID];
	return baseInfoSet.BaseTypes()[bT].Name();
}

void BaseManager::MergeInParty(int baseID, int partyID)
{
	gGame->mPartyManager->MergeParty(partyID, basePartyID[baseID]);
	gGame->SetSelectedPartyID(basePartyID[baseID]);
}

int BaseManager::SpawnOutParty(int baseID)
{
	// this creates a new party with the selected characters, animals and items from the menu.
	// Note that this does not automatically destroy the old Base 
	// TODO: Destroy base when it is now completely empty if it has the "autodestroy" property

	int oldPartyID = basePartyID[baseID];
	int newPartyID = gGame->mPartyManager->GenerateEmptyParty();

	for (int c : playerCharacters)
	{
		gGame->mPartyManager->AddPlayerCharacter(newPartyID,c);
		gGame->mPartyManager->RemoveCharacter(oldPartyID, c);
	}
	for (int c : henchmen)
	{
		gGame->mPartyManager->AddHenchman(newPartyID, c);
		gGame->mPartyManager->RemoveCharacter(oldPartyID, c);
	}
	for (int a : animals)
	{
		gGame->mPartyManager->AddAnimal(newPartyID, a);
		gGame->mPartyManager->RemoveAnimal(oldPartyID, a);
	}
	playerCharacters.clear();
	henchmen.clear();
	animals.clear();

	for (std::pair<int, int> item : inventory)
	{
		gGame->mPartyManager->getPartyInventory(newPartyID).push_back(item);
		std::vector<std::pair<int, int>> oldItems = gGame->mPartyManager->getPartyInventory(oldPartyID);
		// now reduce all the "olditems" by the number moved over, and remove the item if it's now 0
		std::vector<std::pair<int, int>> rebuild;
		for (std::pair<int, int> p : oldItems)
		{
			if (p.first == item.first)
			{
				p.second -= item.second;
				if (p.second > 0)
					rebuild.push_back(p);
			}
		}
		gGame->mPartyManager->SetPartyInventory(oldPartyID, rebuild);
	}
	inventory.clear();

	return newPartyID;
}

int BaseManager::GenerateCampAtLocation(int partyID, int basePosX, int basePosY)
{
	DebugLog("Creating a camp controlled by party #" + std::to_string(partyID) + " at (" + std::to_string(basePosX) + "," + std::to_string(basePosY) + ")");
	int output = shellGenerate();

	basePartyID[output] = partyID; // take on original Party ID

	baseXPos[output] = basePosX;
	baseYPos[output] = basePosY;

	int campID = 0;
	try
	{
		campID = reverseBaseTypeDictionary.at("Camp");
	}
	catch(std::out_of_range)
	{
		campID = 0; // default first one
	}
	baseType[output] = campID;
	
	return output;
}

void BaseManager::DebugLog(std::string message)
{
	gLog->Log("Base Manager", message);
}

bool BaseManager::TurnHandler(int entityID, double time)
{
	return true;
}

bool BaseManager::TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months)
{
	return true;
}

void BaseManager::DumpBase(int baseID)
{
	DebugLog("Dumping base party");
	
	for(int character : gGame->mPartyManager->getPlayerCharacters(basePartyID[baseID]))
	{
		gGame->mCharacterManager->DumpCharacter(character);
	}
	
	for(int hench : gGame->mPartyManager->getHenchmen(basePartyID[baseID]))
	{
		gGame->mCharacterManager->DumpCharacter(hench);
	}
	
	for(int beast : gGame->mPartyManager->getAnimals(basePartyID[baseID]))
	{
		gGame->mMobManager->DumpMob(beast);
	}
	
}



bool BaseManager::ControlCommand(TCOD_key_t* key,int baseID)
{
	// if we don't have a base here (for whatever reason)
	if (baseID == -1)
	{
		// "A" key creates a base
		if (key->c == 'a')
		{
			// make a base! (We don't bother doing a prof check here as there is no circumstance in which no character with
			// Adventuring will be in the party.)
			int partyID = gGame->GetSelectedPartyID();
			int partyX = gGame->mPartyManager->GetPartyX(partyID);
			int partyY = gGame->mPartyManager->GetPartyY(partyID);

			int newBaseID = this->GenerateCampAtLocation(partyID, partyX, partyY);
			
			gGame->SetSelectedBaseID(newBaseID);

			return true;
		}
	}
	else
	{
		if (key->vk == TCODK_TAB)
		{
			if (controlPane < 3)
			{
				controlPane = controlPane + 3;
			}
			else
			{
				controlPane = controlPane - 3;
			}
		}

		if (key->vk == TCODK_LEFT || key->vk == TCODK_RIGHT)
		{

			if (controlPane == PANE_BASE_CHARACTERS)
			{
				controlPane = PANE_PARTY_CHARACTERS;
			}
			else
			{
				if (controlPane == PANE_PARTY_CHARACTERS) controlPane = PANE_BASE_CHARACTERS;
			}


			if (controlPane == PANE_PARTY_INVENTORY)
			{
				controlPane = PANE_BASE_INVENTORY;
			}
			else
			{
				if (controlPane == PANE_BASE_INVENTORY) controlPane = PANE_PARTY_INVENTORY;
			}

		}

		if (key->vk == TCODK_UP)
		{
			menuPosition--;
			// this is entirely dependent on what's inside the pane we're controlling
			if (controlPane == PANE_BASE_CHARACTERS)
			{
				if (menuPosition < 0)
				{
					menuPosition = henchmen.size() + playerCharacters.size() + animals.size();
				}
			}

			if (controlPane == PANE_PARTY_CHARACTERS)
			{
				if (menuPosition < 0)
				{
					int partyID = gGame->GetSelectedPartyID();
					int total = gGame->mPartyManager->getPlayerCharacters(partyID).size();
					total += gGame->mPartyManager->getHenchmen(partyID).size();
					total += gGame->mPartyManager->getAnimals(partyID).size();

					menuPosition = total;
				}
			}


		}
		else if (key->vk == TCODK_DOWN)
		{
			menuPosition++;
			//if (menuPosition[mode] == mCharacterManager->GetInventory(currentCharacterID).size())
			//{
			//	menuPosition[mode] = 0;
			//}
		}
		else if (key->vk == TCODK_ENTER)
		{
			// enter is used to equip and unequip
			//if (mCharacterManager->GetEquipSlotForInventoryItem(currentCharacterID, menuPosition[mode]) != -1)
			//{
			//	mCharacterManager->UnequipItem(currentCharacterID, menuPosition[mode]);
			//}
			//else
			//{
			//	mCharacterManager->EquipItem(currentCharacterID, menuPosition[mode]);
			//}
		}
		else if (key->c >= '1' && key->c <= '9')
		{
			// option selection!
			if (controlPane < PANE_CHARACTER_OPTIONS)
			{
				// character option selected
				int charID = GetSelectedCharacter(baseID);
				std::vector<BaseTag> bts = GetCharacterActionList(baseID, charID);
				// set character to perform action
				pcSelectedAction[baseID][charID] = key->c - '1';
			}
		}
	}

	return false;
}

std::vector<int> BaseManager::GetBasePartyCharacters(int baseID)
{
	int partyID = basePartyID[baseID];
	std::vector<int> base_cs;
	std::vector<int> base_pcs = gGame->mPartyManager->getPlayerCharacters(partyID);
	std::vector<int> base_h = gGame->mPartyManager->getHenchmen(partyID);
	std::copy(base_pcs.begin(), base_pcs.end(), base_cs.begin());
	std::copy(base_h.begin(), base_h.end(), base_cs.begin());
	return base_cs;
}

std::vector<int> BaseManager::GetOutPartyCharacters(int baseID)
{
	int partyID = basePartyID[baseID];
	std::vector<int> out_cs;
	std::copy(playerCharacters.begin(), playerCharacters.end(), out_cs.begin());
	std::copy(henchmen.begin(), henchmen.end(), out_cs.begin());
	return out_cs;
}

void BaseManager::RenderBaseMenu(int baseID)
{
	if (baseID == -1)
	{
		// no base here
		gGame->sampleConsole->printFrame(0, 0, SAMPLE_SCREEN_WIDTH, SAMPLE_SCREEN_HEIGHT, true, TCOD_BKGND_SET, "");

		gGame->sampleConsole->printf(2, 2, "There is no camp or settlement here.");
		gGame->sampleConsole->printf(2, 4, "Press A to have your party");
		gGame->sampleConsole->printf(2, 5, "build a camp here.");
	}
	else
	{
		int baseOwner = this->GetBaseOwner(baseID);
		if (baseOwner != gGame->GetSelectedPartyID())
		{
			// another party's base!
			gGame->sampleConsole->printFrame(0, 0, SAMPLE_SCREEN_WIDTH, SAMPLE_SCREEN_HEIGHT, true, TCOD_BKGND_SET, "");
			std::string message = "There is a camp here belonging to another party.";
			gGame->sampleConsole->printf(2, 2, message.c_str());
		}
		else
		{
			gGame->sampleConsole->printFrame(0, 0, SAMPLE_SCREEN_WIDTH / 2, SAMPLE_SCREEN_HEIGHT / 2, true, TCOD_BKGND_SET, "");
			gGame->sampleConsole->printFrame(SAMPLE_SCREEN_WIDTH / 2, 0, SAMPLE_SCREEN_WIDTH / 2, SAMPLE_SCREEN_HEIGHT / 2, true, TCOD_BKGND_SET, "");
			gGame->sampleConsole->printFrame(0, SAMPLE_SCREEN_HEIGHT / 2, SAMPLE_SCREEN_WIDTH, SAMPLE_SCREEN_HEIGHT, true, TCOD_BKGND_SET, "");

			int bT = baseType[baseID];
			int partyID = basePartyID[baseID];

			if (controlPane < PANE_PARTY_INVENTORY)
			{
				// Character Screen:
				// Top Left: Characters in Base Party
				// Top Right: Characters in Intended Party
				// Bottom: Choices available to selected character

				// draw out the characters from the managers
				std::vector<int> base_cs = GetBasePartyCharacters(baseID);
				std::vector<int> out_cs = GetOutPartyCharacters(baseID);

				// add overall frame with title
				gGame->sampleConsole->printFrame(0, 0, SAMPLE_SCREEN_WIDTH, SAMPLE_SCREEN_HEIGHT, false, TCOD_BKGND_SET, "Party Management");

				// output base characters
				for (int i = 0; i < base_cs.size(); i++)
				{
					std::string name = gGame->mCharacterManager->getCharacterName(base_cs[i]);
					if (gGame->mPartyManager->IsAPC(partyID, base_cs[i])) name += "*";
					TCOD_bkgnd_flag_t backg = TCOD_BKGND_NONE;
					int y_pos = 3 + i;
					gGame->sampleConsole->printEx(2, y_pos, backg, TCOD_LEFT, name.c_str());
				}

				// output party characters
				for (int i = 0; i < out_cs.size(); i++)
				{
					std::string name = gGame->mCharacterManager->getCharacterName(out_cs[i]);
					if (gGame->mPartyManager->IsAHenchman(partyID, base_cs[i])) name += "*";
					TCOD_bkgnd_flag_t backg = TCOD_BKGND_NONE;
					int y_pos = 3 + i;
					gGame->sampleConsole->printEx(2 + SAMPLE_SCREEN_WIDTH, y_pos, backg, TCOD_LEFT, name.c_str());
				}

				// highlight selected character
				int select_y = menuPosition + 3;
				int base_x = (controlPane == PANE_BASE_CHARACTERS) ? 0 : SAMPLE_SCREEN_WIDTH / 2;
				for (int x = base_x; x < (base_x + SAMPLE_SCREEN_WIDTH / 2); x++)
				{
					gGame->sampleConsole->setCharBackground(x, select_y, TCODColor::white, TCOD_BKGND_SET);
					gGame->sampleConsole->setCharForeground(x, select_y, TCODColor::black);
				}

				// selected character ID
				int charID = GetSelectedCharacter(baseID);

				if (charID != -1)
				{
					// controls for selected character

					std::vector<BaseTag> bt_list = GetCharacterActionList(baseID, charID);

					for (int i = 0; i < bt_list.size(); i++)
					{
						BaseTag t = bt_list[i];
						int y = SAMPLE_SCREEN_HEIGHT + 3 + i;
						std::string outp = std::to_string(i) + " : " + t.MenuText();
						if (t.Indicator() != "")
							outp += " (" + t.Indicator() + ")";
						TCOD_bkgnd_flag_t backg = TCOD_BKGND_NONE;
						gGame->sampleConsole->printEx(3, y, backg, TCOD_LEFT, outp.c_str());
					}
				}
			}
			else
			{
				// Inventory Screen:
				// Top Left: Party Inventory
				// Top Right: Base Inventory
				// Bottom: Purchasable Items
			}
		}
	}
}

std::vector<BaseTag> BaseManager::GetCharacterActionList(int baseID, int charID)
{
	std::vector<BaseTag> output;

	int bT = baseType[baseID];
	int partyID = basePartyID[baseID];

	BaseType b = baseInfoSet.BaseTypes()[bT];

	for (int i = 0; i < b.Core().size(); i++)
	{
		std::string c = b.Core()[i];
		int tagID = reverseBaseTagDictionary[c];
		BaseTag t = baseInfoSet.Tags()[tagID];
		if(t.Type() == "CharacterAction")
		{
			if (CharacterCanUseAction(baseID, charID, tagID))
			{
				output.push_back(t);
			}
		}
	}

	return output;
}

int BaseManager::GetSelectedCharacter(int baseID)
{
	int partyID = basePartyID[baseID];
	
	// draw out the characters from the managers
	std::vector<int> base_cs;
	std::vector<int> base_pcs = gGame->mPartyManager->getPlayerCharacters(partyID);
	std::vector<int> base_h = gGame->mPartyManager->getHenchmen(partyID);
	std::copy(base_pcs.begin(), base_pcs.end(), base_cs.begin());
	std::copy(base_h.begin(), base_h.end(), base_cs.begin());

	std::vector<int> out_cs;
	std::copy(playerCharacters.begin(), playerCharacters.end(), out_cs.begin());
	std::copy(henchmen.begin(), henchmen.end(), out_cs.begin());

	
	// selected character ID
	int charID = -1;
	if (controlPane == PANE_BASE_CHARACTERS)
	{
		charID = base_cs[menuPosition];
	}
	else
	{
		charID = out_cs[menuPosition];
	}

	return charID;
}

bool BaseManager::CharacterCanUseAction(int baseID, int characterID, int tag)
{
	int bT = baseType[baseID];
	BaseType b = baseInfoSet.BaseTypes()[bT];
	std::string c = b.Core()[tag];
	BaseTag t = baseInfoSet.Tags()[reverseBaseTagDictionary[c]];
	
	if (t.Requires().size() > 0)
		return false;

	for(int i=0;i<t.Requires().size();i++)
	{
		std::string tagRequire = t.Requires()[i];
		std::string opt = "Options";
		if (std::equal(opt.begin(), opt.end(), tagRequire.begin()))
		{
			int activeTag = pcActiveTags[baseID][characterID][tagRequire.substr(9)];
		}
	}
	
	return true;
}

void BaseManager::RenderBaseMenu(int xpos, int ypos)
{
	for (int i = 0; i < baseXPos.size(); i++)
	{
		if ((xpos == baseXPos[i]) && (ypos == baseYPos[i]))
		{
			if(gGame->GetSelectedPartyID() == basePartyID[i])
				RenderBaseMenu(i);
		}
	}
}