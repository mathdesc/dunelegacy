# Dune Legacy

This is Dune legacy on GitHub a fork based on Dunelegacy 0.96 published on [SourceForge](http://sourceforge.net/projects/dunelegacy)


	Dune Legacy is an effort by a handful of developers to revitalize the first-ever real-time strategy game.

	It tries to be as similar as possible to the original gameplay but to integrate user interface features most modern realtime-strategy games have like selecting multiple units




IRC: #dunelegacy @ irc.freenode.net


Dunelegacy is release under GPL2 (See LICENSE), below is Dunelegacy 0.96 notice  :


	IMPORTANT:
	This software is provided as is, and you are running it at your own risk.  I'm not responsible if any harm results
	from you aquiring, or running this software.  If you distribute this software, make sure this readme file is included.
	The program Dune Legacy is an modernized clone of the excellent Westwood Studios game "Dune 2". It is ridiculusly easy to find
	Dune2 on the net anyways, but I won't provide it.  I think you can still even buy it from Westwood studios, so do that.
	There exists a mod called Superdune 2. The PAK-Files Superdune 2 provides are nearly the same except SCENARIO.PAK.
	 
	This program would not have been created without the use of the excellent SDL library and its extensions.  Thanks guys :).


# Why ?

Dunelegacy is a cool remake amongst many, it has a decent open source codebase to start from 
and I had ideas/wishes (many are also inspired by other Dune2 clones) to add to it while 
it's seems staging, without no more bugfixes/updates.


## What are the objectives in extending it ?

While Dune legacy SHOULD stay close to original Westwood Dune2 :

- it COULD better take advantage of modern RTS features but SHOULD NOT be stripped of 2D pixart graphics
- it COULD be brought closer to what Dune 2 MAY be nowdays with better, extended gameplay
- it SHOULD implement what was sketched but undone MAYBE because what was lacking to Dune 2 was... time
- it COULD be closer also to Dune's novel universe
- it SHOULD still be playable with a modest configuration notably for network games
- it SHOULD fully take advantage of the updates of the standard library C++      

## What is current status ?

Many things have been updated/upgraded, new features coming with modern RTS, some creative ideas
are in the pipe and several bugs are still to hunt down.

For example what is new :

- Urgencies Helper AI for Human player (energy shortage, silo capacity shortage)
- Auto reparing structures 
- Enhanced Mentat AI to fight against (still crude)
- Unit movement in formation & Speed regulation (tankers, rangers, scouts toe the line)
- Group Leaders behavior & unit as Followers (fire orders) 
- Salvo Fire rocket launcher (barrage firing)
- Harvesters & Worms behavior (auto-return and conservative & worms roaming across the whole map)
- many bugfixes including AI deathhand

	  

# Getting Started

The following PAK-files from the original Dune 2 are needed to play Dune Legacy:

<pre>
	HARK.PAK
	ATRE.PAK
	ORDOS.PAK
	ENGLISH.PAK
	DUNE.PAK
	SCENARIO.PAK
	MENTAT.PAK
	VOC.PAK
	MERC.PAK
	FINALE.PAK
	INTRO.PAK
	INTROVOC.PAK
	SOUND.PAK
	GERMAN.PAK (for playing in German)
	FRENCH.PAK (for playing in French)
</pre>

It depends on your system and installation where to put these files. LEGACY.PAK is supplied with Dune Legacy and is already
in the data directory. All the other files should be copied there too. If you are not allowed to copy files there you may
copy these files inside the dunelegacy configuration directory (e.g. ~/.config/dunelegacy/data/ on unix).

## Linux
It depends on how the game was compiled. Normally you should put these files under /usr/share/dunelegacy/ or /usr/local/share/dunelegacy/ .
Just look for LEGACY.PAK. If you do not have root access to your system you should put them in your home directory under ~/.config/dunelegacy/data/ .

## Windows
The installer has already asked for the files and put them in the installation directory. If not put the PAK-files inside your installation folder or 
if you do not have administrator privileges you should put them to C:\Documents and Settings\<YourName>\Application Data\dunelegacy\data\ .


## MAC OS X
The PAK-files have to be copied inside the application bundle. If you have followed the steps in the supplied dmg you have already copied them there.
Otherwise just right-click on the bundle and select "Show Bundle Content". Then navigate into "Contents" and then into "Resources". There you will 
find LEGACY.PAK. Put the other PAK-files there too. Alternativly you can put them in your home directory under ~/.config/dunelegacy/data/ but putting 
them inside the application bundle is the preferred way.

# Install

## Linux

See INSTALL file, please note that a libstdc++11 is also part of the prerequirements. 

```sh
autoreconf --install
./configure CPPFLAGS="-std=c++11"
make
make install
```

## Windows 

TO BE TESTED

## MacOS


TO BE TESTED


# Keyboard Shortcuts

<pre>
General Keyboard Shortcuts:
 Escape							-	Go to menu
 Pause							-	Pause game
 Alt + Enter						-	Toggle fullscreen
 Alt + Tab						-	Switch to other application
 Print Key or Ctrl + P					-	Save screenshot as ScreenshotX.bmp with increasing numbers for X
 Enter							-	Start/Stop chatting

 Key F1							-	Zoomlevel x1
 Key F2							-	Zoomlevel x2
 Key F3							-	Zoomlevel x3

 Key T							-	Toggle display of current game time
 Key F10						-	Toggle sound effects and voice
 Key F11						-	Toggle music
 Key F12						-	Toggle display of FPS
 Key Up, Down, Left or Right				-	Move on the map

 Key F5							-	Skip 30 seconds (only in singleplayer)
 Key F6							-	Skip 2 minutes (only in singleplayer)
 Key -							-	Decrease gamespeed (only in singleplayer)
 Key +							-	Increase gamespeed (only in singleplayer)

 Ctrl + (Key 1 to Key 9)				-	Save the list of selected units as unit group 1 to 9
 Key 1 to Key 9						-	Select units from unit group 1 to 9
 Shift + (Key 1 to Key 9)				-	Add all units from unit group 1 to 9 to the list of currently selected units
 Key 0							-	Deselect all currently selected units
 Ctrl + Key 0						-	Remove currently selected units form all unit groups (group 1-9)


 Key M							-	Order unit to move to some position
 Key A							-	Order selected units to attack some unit, structure or position
 Key s							- 	Order to cancel previous order and stop immediatly
 Key S							-	Order to attack in salvo fire (for able units)							
 Key R							-	Repair selected structure/unit or return selected harvester
 Key U							-	Upgrade selected structure
 Key P							-	Place a structure (if a construction yard is selected)
 Key x							-	Order 5 units to scatter a bit around
 Key C							-	Order infantry to capture a building

While Unit Selection Box :


 Key TAB						-	Spot potential Target
 Key z							-	Cycle/Change group leader
 Ctrl + LeftMouseButton					-	Move in formation
 Ctrl + Shift + LeftMouseButton				-	Move in formation, rotate 45°

Map Editor:
 Print Key or Ctrl + P					-	Save a picture of the whole map as <Mapname>.bmp
 Ctrl + Z						-	Undo last edit
 Ctrl + Y						-	Redo last edit
</pre>


# Configuration file

If you want to fine tune the configuration of Dune Legacy you might want to take a look at the configuration file "Dune Legacy.ini". Depending on your system it 
is either placed in ~/.config/dunelegacy (on Linux), ~/Library/Application Support/Dune Legacy (on Mac OS X) or in C:\Documents and Settings\<YourName>\Application Data\dunelegacy\ (on Windows).


# Internet Game

To play online via Internet you have to manually enable port forwarding if you use a NAT Router. Forward the Dune Legacy Server Port (Default is 28747) from your NAT Router to your computer. Use the same port on your router as configured in Dune Legacy.
Example: If your machine has IP 192.168.123.1 and your using the default Dune Legacy Port, than forward port 28747 from your router to 192.168.123.1:28747.


