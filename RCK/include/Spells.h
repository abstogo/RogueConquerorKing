#pragma once
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>
#include <jsoncons/json_type_traits_macros.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include <fstream>
#include "Character.h"
#include "Game.h"

// SPELLS! 
// These need to be fully compatible with the spell creation system from Player's Companion.
// To that end, the pre-created spells will be built from effect, duration etc, and given a level as per the corebook.
// Beyond that, corebook and PC spellcasting all works the same way. Each character has an allotment of casting at each level per day,
// and a repertoire of known spells. The main difference lies in how the repertoire is filled. Arcane types have a personal spellbook,
// while divine types have a tradition list (and Apostasy). Repertoires and spell slots are stored in the Character Manager, while the 
// spell effects are stored and managed here.

// When the spell creation system is added to the game, we will also add the ablity to output the created spell in "spellbook" JSON
// files which can be shared between games. For this reason, this is one of the Managers that will be built with the ability to load
// multiple scripts - any spellbook_*.json file in the scripts folder will be loaded.

// About Targets. Many spells have more than one targeting mode.
// Examples: 
// Sleep - 1 creature up to 4+1 HD OR 2d8 creatures of 4HD or less) 
// Light - Location OR item OR creature

// Spell descriptors:
// Name: Obvious
// Source: Arcane/Divine (maybe Eldritch later)
// Level: Casting level (for spellmaker spells this is after experimentation etc, for "common spells" in the book this is the level specified which is usually low)
// Type: Spell school. Blast, Death, Detection, Enchantment, Healing, Illusion, Movement, Protection, Summoning, Transmogrifcation, Wall. First division.
// Effect: Effect within school. Primary determinant of effect.
// Duration: Dependent on school. Variations on instant/rounds/turns/minutes/hours/days/concentration/until dispelled/permanent
// DurationModifiers: Effect ends early or late on specific contexts
// Range: Range in feet (indoors) or yards (outdoors)
// TargetModes: List of TargetModes. 
// TargetCount: Number of targets in targeting mode.
// TargetType: Targets that can be selected in targeting mode.
// TargetModifiers: Target only applies in specific contexts or in a particualr order (eg "LivingOnly","Separate")
// Dice: Dice of damage or healing, if applicable. Usually expressed as "nD6/level"
// DiceLimit: Maximum dice
// SavingThrow: "Spells","BlastBreath", "Death", "PoisonParalysis", "StaffsWands"
// AdditionalEffects: Extra effect(s) within school that apply to targets affected by primary
// EffectModifiers: Overall effect modifiers such as "SaveHalf"
// ProficiencyBonus: Spell benefits from proficiency (eg elemental spells for Elementalism,Transmogrification for transform spells, Enchantment and Illusion etc)
// ReversesTo: This spell can be reversed into the specified spell. Divine get both automatically, Arcane have to learn them both, there is a bonus for researching the reversed version

struct TargetMode
{
	std::string TargetCount_;
	std::string TargetType_;
	std::vector<std::string> TargetModifiers_;
};

class Spell
{
	std::string Name_;
	std::string Source_;
	int Level_;
	std::string Type_;
	std::string Effect_;
	std::string Duration_;
	std::vector<std::string> DurationModifiers_;
	int Range_;
	std::vector<TargetMode> TargetModes_;
	std::string Dice_;
	int DiceLimit_;
	std::vector<std::string> DiceModifiers_;
	std::string SavingThrow_;
	std::vector<std::string> AdditionalEffects_;
	std::vector<std::string> EffectModifiers_;
	std::string ProficiencyBonus_;
	std::string ReversesTo_;


public:
	Spell(const std::string Name, const std::string Source, const int Level, const std::string Type, const std::string Effect, const std::vector<std::string> AdditionalEffects, const std::vector<std::string> EffectModifiers, const std::string Duration, const std::vector<std::string> DurationModifiers, const int Range, const std::vector<TargetMode> TargetModes, const std::string Dice, const int DiceLimit, const std::vector<std::string> DiceModifiers, const std::string SavingThrow,const std::string ProficiencyBonus, const std::string ReversesTo)
		: Name_(Name), Source_(Source), Level_(Level), Type_(Type), Effect_(Effect), Duration_(Duration), DurationModifiers_(DurationModifiers), Range_(Range), TargetModes_(TargetModes), Dice_(Dice), DiceLimit_(DiceLimit), DiceModifiers_(DiceModifiers), SavingThrow_(SavingThrow), AdditionalEffects_(AdditionalEffects), EffectModifiers_(EffectModifiers), ProficiencyBonus_(ProficiencyBonus), ReversesTo_(ReversesTo)
	{}

	const std::string Name() {
		return Name_;
	}
	const std::string Source() {
		return Source_;
	}
	const std::string Type() {
		return Type_;
	}
	const std::string Effect() {
		return Effect_;
	}
	const std::string Duration() {
		return Duration_;
	}
	const std::vector<TargetMode> TargetModes() {
		return TargetModes_;
	}
	const std::string Dice() {
		return Dice_;
	}
	const std::string SavingThrow() {
		return SavingThrow_;
	}
	const std::string ProficiencyBonus() {
		return ProficiencyBonus_;
	}
	const std::string ReversesTo() {
		return ReversesTo_;
	}

	const int Level() { return Level_; }
	const int Range() { return Range_; }
	const int DiceLimit() { return DiceLimit_; }

	const std::vector<std::string> DurationModifiers() {
		return DurationModifiers_;
	}
	const std::vector<std::string> DiceModifiers() {
		return DiceModifiers_;
	}
	const std::vector<std::string> AdditionalEffects() {
		return AdditionalEffects_;
	}
	const std::vector<std::string> EffectModifiers() {
		return EffectModifiers_;
	}
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(Spell, Name, Source, Level, Type, Effect, AdditionalEffects, EffectModifiers, Duration, DurationModifiers, Range, TargetModes, Dice, DiceLimit, DiceModifiers, SavingThrow, ProficiencyBonus, ReversesTo)

class SpellSet
{
	std::vector<Spell> Spells_;

public:
	SpellSet(const std::vector<Spell>& Spells) : Spells_(Spells)
	{}

	SpellSet() {}

	const std::vector<Spell>& Spells() const { return Spells_; }
	std::vector<Spell>& Spells() { return Spells_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(SpellSet, Spells)

class SpellManager
{
	std::map<std::string, SpellSet> spellSets;							// spell lists by source file

	std::vector<Spell> masterSpellList;									// collected spells for the game
	int nextSpellIndex;

	std::map<std::string, int> spellLookup;								// spells by name

	std::map<std::string, std::vector<std::vector<int>>> spellMatrix;	// spell lookup by magic type and level
	
	// standing spell effects in the world
	std::vector<int> spellEffects;										// existing spell effects in the world, indexes into Spell
	std::vector<int> spellCasters;										// caster of spells
	std::vector<int> spellDurations;									// remaining spell effect duration in rounds
	std::vector<std::vector<int>> spellTargets;							// spell target entities
	std::vector<std::vector<int>> spellLocationsX;						// spell target cells X
	std::vector<std::vector<int>> spellLocationsY;						// spell target cells Y

public:
	static SpellManager* LoadSpells();

	SpellManager()
	{
		nextSpellIndex = 0;
	}

	void LoadSpellSet(std::string path);

	std::vector<int>& getSpellsAtLevel(std::string magicType, int level);

	Spell& getSpell(int index);
	Spell& getSpell(std::string name) { return getSpell(spellLookup[name]); }

	int GetDurationInRounds(std::string durationString, int casterLevel);

	// main casting function
	void CastSpell(int managerID, int casterID, int spellID);

	// system handlers
	bool TurnHandler(int entityID, double time);																		// handle spell effects that have effects over turns
	bool TargetHandler(int entityID, int returnCode, std::vector<int> targetManagersX, std::vector<int> targetIDsY);	// return point for targets for spells
	bool TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months);								// handle expiry of ongoing spells

};

// Step-by-step mechanics of spellcasting:
// 1) Caster (player, AI PC, AI NPC) calls CastSpell with its own Manager and ID, the of the spell being cast and any time multiplier (default x1.0)
// 2) Spell is stored in castingSpells, new index is generated.
// 3) Casting of the spell takes time. A timer flag is set in GameTime for when spellcasting will complete. (This may be sub-1-round or may be much longer - this helps us with Rituals too).
// 4) TurnHandler manages the return from GameTime. Each time it triggers, all castingSpells decrement by time. If the time has reached 0, then calls SpellTargeting with the index into castingSpells.
// 5) SpellTargeting checks the spell for how targeting will work. If self target, call SpellEffect with caster manager and ID. If caster is PC, go to Game::TriggerTargeting. If caster is AI, go to Character or Mob TriggerTargeting. In all targeting cases, returnCode is index into castingSpells.
// 6) Targeting system returns to TargetHandler, returnCode is castingSpells, get caster ID and manager. TargetHandler gets the list of targets.
// 7) TargetHandler calls ResolveSpell for each target.
// 8) ResolveSpell checks for saves, returns if save for nothing, flags if save for half. If spell has duration Instant, resolve effects now and return. If spell has higher duration, resolve effects and add spell to standing spell list.
// 9) TimeHandler counts spells down over time. When spell counts out, call EndSpell for each target (to eg remove conditions etc)