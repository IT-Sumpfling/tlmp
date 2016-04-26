# Torchlight Multiplayer #
Extends Runic Games' Torchlight (www.torchlightgame.com) game to allow for Multiplayer.

[r212](https://code.google.com/p/tlmp/source/detail?r=212) has been released. There are some crash cases still in addition to client-side oddities but all in all it is a lot more stable than [r185](https://code.google.com/p/tlmp/source/detail?r=185) and should be in a playable state for the most part.

## Getting Started ##
See the [Getting Started](http://code.google.com/p/tlmp/wiki/GettingStarted) page for details.

## [r212](https://code.google.com/p/tlmp/source/detail?r=212) Fixes ##
  * Fixed a bug that would cause TLMP to not load if Windows was installed on a drive other than C:.
  * Fixed a bug in Runic's code when the player had no pet and attempted to open their stash (null pointer deference).

## [r207](https://code.google.com/p/tlmp/source/detail?r=207) Features ##
  * Stability fixes - this version is much more stable due to handling Equipment better.
  * Inventory inspection - you can inspect others' inventory by simply targeting them and opening the inventory.
  * Skills are now transferred - you can now see graphical effects and clients can now damage via skills.
  * Clients are now more involved in the game - AI processing now works for client characters.
  * Many more bug fixes.