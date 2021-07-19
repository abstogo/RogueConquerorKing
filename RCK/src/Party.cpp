#include "Party.h"

PartyManager::PartyManager()
{
	
}

PartyManager::~PartyManager()
{
	
}

PartyManager* PartyManager::LoadPartyData()
{
	gLog->Log("Party Loader", "Started");
	
	PartyManager* output = new PartyManager();

	output->nextPartyID = 0;

	gLog->Log("Party Loader", "Completed");
	
	return output;
}

void PartyManager::RemoveCharacter(int partyID, int entityID)
{
	if(std::remove(playerCharacters[partyID].begin(), playerCharacters[partyID].end(), entityID) == playerCharacters[partyID].end())
	{
		// not found in the PC set, check Henches too
		std::remove(henchmen[partyID].begin(), henchmen[partyID].end(), entityID);
	}
}

void PartyManager::RemoveAnimal(int partyID, int entityID)
{
	std::remove(animals[partyID].begin(), animals[partyID].end(), entityID);
}

int PartyManager::shellGenerate()
{
	int output = nextPartyID++;

	DebugLog("Generating Empty Party #" + std::to_string(output));
	
	std::vector<int> pcs;
	playerCharacters.push_back(pcs);
	std::vector<int> henchs;
	henchmen.push_back(henchs);
	std::vector<int> anims;
	animals.push_back(anims);
	std::vector<std::pair<int, int>> inv;
	partyInventory.push_back(inv);
	totalCarryCapacity.push_back(0);
	totalSuppliesFood.push_back(0);
	partyXPos.push_back(-1);
	partyYPos.push_back(-1);

	return output;
}


int PartyManager::GenerateBaseTestParty()
{
	DebugLog("Creating base test party: 1 Fighter and 1 Mule");
	int output = shellGenerate();

	GeneratePlayerCharacter(output, "BaseTestFighter1", "Fighter");
	GenerateAnimal(output, "Mule");
	
	return output;
}

int PartyManager::GenerateAITestParty()
{
	DebugLog("Creating base test party: 2 Fighters, 1 Henchman (NM) and 1 Mule");
	int output = shellGenerate();

	GeneratePlayerCharacter(output, "BaseTestFighter1", "Fighter");
	GeneratePlayerCharacter(output, "BaseTestThief1", "Thief");
	GenerateHenchman(output, "Hench");
	GenerateAnimal(output, "Mule");

	return output;
}

// Generators
void PartyManager::AddPlayerCharacter(int partyID, int characterID)
{
	playerCharacters[partyID].push_back(characterID);
	DebugLog("PC (character ID #" + std::to_string(characterID) + ") added to party #" + std::to_string(partyID));
}
void PartyManager::GeneratePlayerCharacter(int partyID, std::string name, const std::string _class)
{
	int newCharID = gGame->mCharacterManager->GenerateTestCharacter(name, _class);
	AddPlayerCharacter(partyID, newCharID);
}

void PartyManager::AddHenchman(int partyID, int characterID)
{
	henchmen[partyID].push_back(characterID);
	DebugLog("Henchman (character ID #" + std::to_string(characterID) + ") added to party #" + std::to_string(partyID));
}
void PartyManager::GenerateHenchman(int partyID, std::string name)
{
	int henchID = gGame->mCharacterManager->GenerateNormalMan(name);
	AddHenchman(partyID, henchID);
}

void PartyManager::AddAnimal(int partyID, int mobID)
{
	animals[partyID].push_back(mobID);
	DebugLog("Animal (mob ID #" + std::to_string(mobID) + ") added to party #" + std::to_string(partyID));
}
void PartyManager::GenerateAnimal(int partyID, std::string name)
{
	int animalID = gGame->mMobManager->GenerateMonster(name, -1, -1, -1, false);
	AddAnimal(partyID, animalID);
}

int PartyManager::getNextPlayerCharacter(int partyID, int currentPC)
{
	std::vector<int>& partyPCs = getPlayerCharacters(partyID);
	// sanity check
	if (partyPCs.size() == 0) return -1;

	// if the current PC is not specified, get the first one
	if (currentPC == -1) return partyPCs[0];


	int output = -1;
	auto found = std::find(partyPCs.begin(), partyPCs.end(), currentPC);
	bool done = false;

	while (!done)
	{
		// iterate to next one
		found++;
		// lap around if needed
		if (found == partyPCs.end()) found = partyPCs.begin();
		output = *found;
		if (output == currentPC)
		{
			// lapped right back round
			done = true;
			output = -1;
		}
		else
		{
			// found another element. check if it's conscious
			if(!gGame->mCharacterManager->getCharacterHasCondition(output,"Unconscious"))
			{
				done = true;
			}
		}
	}

	return output;
}

bool PartyManager::IsInParty(int partyID, int managerType, int entityID)
{
	if(managerType == MANAGER_CHARACTER)
	{
		return (IsAPC(partyID, entityID) || IsAHenchman(partyID, entityID));
	}

	if(managerType == MANAGER_MOB)
	{
		return IsAnAnimal(partyID, entityID);
	}

	// otherwise it's not in the party (maybe items?)
	return false;
}

bool PartyManager::IsAPC(int partyID, int entityID)
{
	return std::find(playerCharacters[partyID].begin(), playerCharacters[partyID].end(), entityID) != playerCharacters[partyID].end();
}

bool PartyManager::IsAHenchman(int partyID, int entityID)
{
	return std::find(henchmen[partyID].begin(), henchmen[partyID].end(), entityID) != henchmen[partyID].end();
}

bool PartyManager::IsAnAnimal(int partyID, int entityID)
{
	return std::find(animals[partyID].begin(), animals[partyID].end(), entityID) != animals[partyID].end();
}

void PartyManager::DebugLog(std::string message)
{
	gLog->Log("Party Manager", message);
}

bool PartyManager::TurnHandler(int entityID, double time)
{
	return true;
}

bool PartyManager::TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months)
{
	return true;
}

void PartyManager::DumpParty(int partyID)
{
	DebugLog("Dumping Party");
	
	for(int character : playerCharacters[partyID])
	{
		gGame->mCharacterManager->DumpCharacter(character);
	}

	for(int hench : henchmen[partyID])
	{
		gGame->mCharacterManager->DumpCharacter(hench);
	}

	for(int beast : animals[partyID])
	{
		gGame->mMobManager->DumpMob(beast);
	}
}
