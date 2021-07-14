# RogueConquerorKing
Roguelike based on the Adventurer Conqueror King System using LibTCOD.

Only the MSVS build system (found in buildsys/msvs/) is working. I'll get around to fixing the others one day! 
The game should build from RogueConquerorKing_LibTCOD.sln with no issues (tested with Visual Studio 2019).

At present there are two zones available - a 5ft square based indoor mode, and a 5yd hex based outdoor mode. You can move indoors using IJKL, and you can move outdoors using 89UOJK.
Other keys:
- '.' to pass time without moving
- '<' (shift-,) to pass through a stairwell or portal
- 'c' to toggle the Character Sheet (up/down and enter to equip/unequip)
- 'v' to toggle the Inventory Screen
- 'l' to enter Look Mode
- 't' to throw/shoot an equipped item
- Tab to switch between player characters.

There is also a debug "dump" key, ` which will dump out a text description of the player party (or the character under the cursor if in Look Mode).

Gameplay notes:
- Current test level creates a party containing 1 Fighter, 1 Thief, 1 Normal Man hench, and 1 mule. It also spawns 3 goblins which will attack on sight, and some items on the ground.
- Push into a monster to trigger a melee attack. Press t to trigger a ranged attack.
- Basic ACKS combat mechanics are in place, so picking up armour and shield will improve your AC, you'll roll against the enemy AC (and vice versa), and damage is weapon-based. Cleaves are also in play and will trigger if you kill an enemy (and have any available).
- NPCs (animals, henches) do not currently have attack AI, so they won't attack the goblins back.
- The currently selected character is bright, the unselected characters less bright, and unconscious characters/monsters are dimly rendered.
- Move onto a downed/unconscious character to trigger a Mortal Wound check.
