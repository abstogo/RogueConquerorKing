#include "Spells.h"
#include <filesystem>

namespace fs = std::filesystem;

SpellManager* SpellManager::LoadSpells()
{
	gLog->Log("Spell Loader", "Started");

	std::string scriptsFolder = "RCK/scripts";
	
	SpellManager* sm = new SpellManager();
	for (const auto& entry : fs::directory_iterator(scriptsFolder))
	{
		std::string filename = entry.path().filename().string();
		std::string pathString = entry.path().string();
		// see if it starts with "spellbook_"
		std::string prefix = "spellbook_";
		if (filename.substr(0, prefix.size()).compare(prefix) == 0)
		{
			gLog->Log("Spell Loader", "Decoding " + pathString);
			sm->LoadSpellSet(pathString);
		}
		
	}
	
	gLog->Log("Spell Loader", "Completed");

	return sm;
}

void SpellManager::LoadSpellSet(std::string path)
{
	std::ifstream is(path);

	SpellSet& spellSet = jsoncons::decode_json<SpellSet>(is);

	// add spellset to selection
	spellSets[path] = spellSet;

	// now add each spell to the master set

	for (Spell s : spellSet.Spells())
	{
		// spells by index
		masterSpellList.push_back(s);
		int index = nextSpellIndex++;

		// name lookup
		spellLookup[s.Name()] = index;

		// add magic type to spell matrix if not exists
		if (spellMatrix.find(s.Source()) == spellMatrix.end())
		{
			std::vector<std::vector<int>> spellSource;
			for (int i = 0; i < 6; i++)
			{
				spellSource.push_back(std::vector<int>());
			}
			spellMatrix[s.Source()] = spellSource;
		}
		// add spell to matrix
		spellMatrix[s.Source()][s.Level()].push_back(index);
	}
}

std::vector<int>& SpellManager::getSpellsAtLevel(std::string magicType, int level)
{
	return spellMatrix[magicType][level];
}

Spell& SpellManager::getSpell(int index)
{
	return masterSpellList[index];
}

void SpellManager::CastSpell(int managerID, int casterID, int spellID)
{
	// set off the spell. In all cases (apart from pure self-range spells) this triggers a targeting routine.

	Spell spl = getSpell(spellID);

	// first filter: spell type
	// second filter: effect
	if (spl.Type() == "Blast")
	{
		// blast spells all either inflict damage or some other physical negative effect
	}
	else if (spl.Type() == "Enchantment")
	{
		// enchantment spells almost all either inflict conditions, control/constrain behaviours or even swap sides
	}
	else if (spl.Type() == "Healing")
	{
		// healing removes things: HP damage, conditions, or stuff like poison or disease
	}
	else if (spl.Type() == "Death")
	{
		// death spells are a bit eclectic. Either direct damage (no physical element, no animation when they happen), or undead-related stuff
	}
	else if (spl.Type() == "Detection")
	{
		// detection spells create look-mode which displays specific information, or just give informational messages in some cases (commune?)
	}
	else if (spl.Type() == "Illusion")
	{
		// very variable illusion effects
	}
	else if (spl.Type() == "Protection")
	{
		// protection spells are mostly long duration and serve to prevent some effects from happening, or increase AC, or reduce damage, or even prevent some attacks entirely
	}
	else if (spl.Type() == "Summoning")
	{
		// summoning spells create new entities
	}
	else if (spl.Type() == "Transmogrification")
	{
		// transmogrification spells change the target - either granting abilities, or completely changing the target. Can change capabilities entirely!
	}
	else if (spl.Type() == "Wall")
	{
		// wall spells create objects within the world. (complication - hex maps vs square maps!)
	}
}

// handle spell effects that have effects over turns
bool SpellManager::TurnHandler(int entityID, double time)
{
	return true;
}

// return point for targets for spells
bool SpellManager::TargetHandler(int entityID, int returnCode)
{
	return true;
}

// handle expiry of ongoing spells
bool SpellManager::TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months)
{
	return true;
}