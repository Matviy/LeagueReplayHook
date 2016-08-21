## League of Legends Replay UI Hook

This library allows you to programatically interact with the League of Legends replay client using a very simple UDP channel.

## How to use it:
Compile the dynamic library. Inject the DLL into a running League of Legends process. By default, the client will listen for commands on UDP port 6000, and send you data on UDP port 7000.

## How it works:

When running in replay/spectator mode, the League of Legends client uses a Flash-based user interface. This Flash interface is simply a flash movie, which is ran and rendered onto your DirectX surface using the Scaleform flash engine.

The League of Legends graphics engine communicates bidirectionally with the Flash engine using simple text-based commands via an Invoke() function called on the flash Movie object provided by Scaleform, and via registered call-back methods which are passed string data via GetURL() from the Flash side.

All updates to the interface (timers, health bars, creep scores...etc) are sent to the interface using the Invoke() function.
All user interaction with the interface (camera toggling, clicking pause/play, time seeking...etc) is sent from the Flash movie via the GetURL() Flash function.

By hooking these two functions, we create a valuable data source we can use to capture in-game status and events, as well as inject our own commands to programatically control the replay client.

## What commands can i send? How do i send them?
Commands are sent to the client by writing them directly to the listening UDP port 6000. 

Commands are in the format: "replayui_function=parameter," **(Note the trailing comma)**

The following commands are currently available:

| Function Name        |Purpose  |
| ------------- |-----:|
|	replayui_setReplayScrubberPosition=\<positive integers>	|	Jumps to X seconds into the game.	|
|	replayui_replayPressPlay=0	|	Press the Play button.	|
|	replayui_replayPressPause=0	|	Press the Pause button.	|
|	replayui_replayPressJumpForward=0	|	Press the Jump-to-live button.	|
|	replayui_replayPressInstantReplay=0	|	Press the instant-replay button.	|
|	replayui_replayPressSpeedUp=0	|	Press the Speed Up button.	|
|	replayui_replayPressSpeedDown=0	|	Press the Slow Down button.	|
|	replayui_setDirectedCameraMode=\<0-9>	|	Make directed camera follow a player.	|
|	replayui_setDirectedCameraEnabled=\<0\|1>	|	Enable/Disable directed camera.	|
|	replayui_setFogOfWarTeam=\<0\|1\|2>	|	Set fog mode for team red, blue, or both.	|
|	replayui_setFogOfWarEnabled=\<0\|1>	|	Enable/Disable Fog of war	|
|	replayui_setLockCameraEnabled=\<0\|1>	|	Locks camera onto currently selected player.	|
|	replayui_setTargetedChampion=\<0-9>	|	Selects a player.	|
|	replayui_setCharInfoEnabled=\<0\|1>	|	Enable/Disable hidden stats menu for current player. (http://i.imgur.com/MAxBuUm.png)	|
|	replayui_setActiveChampion=\<0-9>	|	Selects a player.	|
|	replayui_playBurstDamageSound=\<0\|1>	|	Play ping sound with left/right stereo balance.	|
|	replayui_moveCameraToChampion=\<0-9>	|	Instant jump camera to player.	|
|	replayui_minimapYMod=\<+/- integers>	|	Move minimap along Y-axis.	|
|	replayui_minimapAlpha=\<+/- numbers>	|	Adjust transparency of minimap.	|
|	replayui_setVisibilityOption_MinionHealth=\<0/1>	|	Enable/Disable minion health bars.	|
|	replayui_setVisibilityOption_Chat=\<0/1>	|	Enable/Disable chatbox.	|
|	replayui_setVisibilityOption_ChampionNames=\<0/1>	|	Enable/Disable champion names.	|


## What can i receive? How do i receive them?
Simply listen on UDP port 7000 to receive the text data.

You receive all commands that are sent by the League of Legends game engine to the interface. This includes stats for all players, gold, spell/skill cooldowns, buff cooldowns, health/mana bars, death timers, and so on.

At the start of a replay, the game engine sends the necessary commands and data for the UI to set itself up with the correct data (summoner names, icons, spell images...etc). During the duration of the game, the UI receives data that represent in-game events.

Usually, many pieces of data and multiple commands are concated into a single call, and are all comma-dilimited. The text is well-structured and rather easily parsed, and is mostly self-explanitory. 

A sample of the data can be seen here: http://pastebin.com/kLUejtCV

## How stable/reliable is this?
The function signatures used to locate the functions and write hooks were tested on versions of the client as early as 5.1, and are unlikely to change in the future. Also, it is unlikely Riot will be making major changes to their Scaleform implementation, since all signs suggest that they are more likely to simply to abandon flash (like they did for the modern UI when you're in a live game, which is no longer Flash-based), since Flash is a **masive** performance killer. (Disabling Scaleform increases FPS by 50%+)

## Isn't this a script? Can i get banned for this?
Almost certainly no. None of this will work during a live game (read "How stable/reliable is this?"). This only works for spectate/replay sessions, and cannot provide any form of competitive advantage.