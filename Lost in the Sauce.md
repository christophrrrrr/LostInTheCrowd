# **Lost in the Sauce — Game Vision**

## **Elevator Pitch**

**Lost in the Sauce** is a 3D hidden-character game inspired by Mario Party-style crowd minigames and Where’s Wally. The player enters a busy medieval market and must find one target character hidden among a large crowd of very similar-looking NPCs.

The core fun is simple:  
**scan the crowd, spot the correct character, click them, win the round.**

## **Core Game Feel**

This is **not** a detective game with unique people.  
This is **not** a fully procedural “find one exact person” game.  
This is a **simple, readable crowd-search game** where the target belongs to one of a small number of character types.

The challenge comes from:

* lots of NPCs on screen  
* similar silhouettes and colors  
* crowded medieval scenery  
* careful visual scanning

  ## **First Version Scope**

The first version should stay small and focused.

### **Character Types**

Use around **5 character types total**.

Example types:

* Knight  
* Guard  
* Merchant  
* Farmer  
* Monk

These characters should look **very similar in color and silhouette** so the player has to look carefully.

The target is one of these types, and the round objective is something like:

* “Find the Merchant”  
* or a portrait of the Merchant on the UI

  ## **World / Setting**

The first map should be a **medieval marketplace**.

Why this setting:

* it naturally supports crowds  
* it fits the hidden-object gameplay  
* it looks visually interesting  
* it gives lots of props and obstacles for the crowd to move around

Useful environment ideas:

* stalls  
* barrels  
* wooden carts  
* banners  
* market tents  
* stone buildings  
* town square paths

  ## **Gameplay Loop**

1. Start a round.  
2. Show the target character on the UI.  
3. Spawn a crowded market full of similar NPCs.  
4. Player scans the crowd.  
5. Player clicks an NPC.  
6. If correct, the round ends in success.  
7. If incorrect, show a simple wrong-selection reaction and let the player keep searching.

   ## **Controls**

Keep the controls simple.

Recommended:

* mouse to rotate camera  
* mouse wheel to zoom  
* left click to select an NPC

Optional later:

* controller support  
* limited player movement  
* camera pan or slow orbit controls

  ## **Game Rules**

* No timer in the first version.  
* No score in the first version.  
* No fail state for wrong clicks.  
* The player should be able to keep searching until they find the target.

The first version is about proving the core idea is fun, not about pressure.

## **Visual Style**

Target style:

* low poly  
* readable  
* colorful enough to separate character types  
* simple and charming rather than realistic

The exact asset style is flexible depending on what good assets are available.

## **Crowd Design**

The crowd should feel busy, but the NPC logic should stay simple.

NPCs can:

* walk around randomly  
* idle occasionally  
* pause near stalls  
* move along simple paths

They do not need advanced AI.

## **Important Design Goal**

The player should be able to understand the game instantly:

**“Here is the character. Now find them in the crowd.”**

That is the entire core fantasy.

## **Non-Goals for Version 1**

Do not build these yet:

* unique generated characters  
* portrait matching of exact individuals  
* clues and detective mechanics  
* timers  
* complex scoring systems  
* many maps  
* advanced crowd simulation  
* stealth or combat

  ## **Success Criteria**

The game is successful if players:

* understand the goal immediately  
* enjoy scanning the crowd  
* feel satisfied when they find the target  
* want to play another round

  ## **Future Direction**

If the simple version works, later versions can expand into:

* more character types  
* clue-based rounds  
* unique NPC generation  
* larger crowds  
* new locations like festivals or airports  
* harder visual variants

But the first game should stay focused on the simple Mario-style crowd-search concept.

## **One-Sentence Summary**

A 3D medieval crowd-search game where the player must spot one target character among many very similar NPCs and click them to win.

