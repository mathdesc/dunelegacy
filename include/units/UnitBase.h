/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UNITBASE_H
#define UNITBASE_H

#include <ObjectBase.h>

#include <House.h>

#include <list>

// forward declarations
class Tile;

#define MAX_SALVE 10
#define BASE_SALVO_TIMER 65
#define FIND_TARGET_TIMER_RESET 100
#define FIND_OLDTARGET_TIMER_RESET FIND_TARGET_TIMER_RESET*100

class UnitBase : public ObjectBase
{
public:
	UnitBase(House* newOwner);
	UnitBase(InputStream& stream);
	void init();
	virtual ~UnitBase();

	virtual void save(OutputStream& stream) const;

	void blitToScreen();

	virtual ObjectInterface* getInterfaceContainer();

	virtual void checkPos() = 0;
	virtual void deploy(const Coord& newLocation, bool sound = false);

	virtual void destroy();
	void deviate(House* newOwner);

	virtual void drawSelectionBox();
	virtual void drawOtherPlayerSelectionBox();

	/**
		This method is called when a formation of units are simultaneously ordered by a right click
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
	*/
	virtual void handleFormationActionClick(int xPos, int yPos);

	/**
		This method is called when an unit is ordered by a right click
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
	*/
	virtual void handleActionClick(int xPos, int yPos);

	/**
		This method is called when an unit is ordered to attack
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
	*/
	virtual void handleAttackClick(int xPos, int yPos);

	/**
		This method is called when an unit is ordered to salve attack
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
	*/
	virtual void handleSalveAttackClick(int xPos, int yPos);


    /**
		This method is called when an unit is ordered to move
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
	*/
	virtual void handleMoveClick(int xPos, int yPos);

	/**
		This method is called when an unit is ordered to be in a new attack mode
		\param	newAttackMode   the new attack mode the unit is put in.
	*/
	virtual void handleSetAttackModeClick(ATTACKMODE newAttackMode);

	/**
		This method is called when an unit is ordered to cancel given order (stop moving/fellowing)
	*/
	virtual void handleCancel();


	/**
		This method is called when an unit should move to (xPos,yPos)
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doMove2Pos(int xPos, int yPos, bool bForced);

	/**
		This method is called when an unit should move to coord
		\param	coord	the position on the map
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doMove2Pos(const Coord& coord, bool bForced);

	/**
		This method is called when an unit should move to another unit/structure
		\param	followObjectID	the ID of the other unit/structure
		\param	TargetObjectID	the ID of its target
	*/
	virtual void doMove2Object(Uint32 followObjectID, Uint32 TargetObjectID, bool follow = true);

	/**
		This method is called when an unit should move to another unit/structure
		\param	pFellowObject	the other unit/structure
		\param  pTargetObject   its current target
	*/
	virtual void doMove2Object(const ObjectBase* pFellowObject, const ObjectBase* pTargetObject);

	/**
		This method is called when an unit should attack a position
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doAttackPos(int xPos, int yPos, bool bForced);

	/**
		This method is called when an unit should attack a position with salve weapon if it has otherwise fallback to doAttackPos
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doSalveAttackPos(int xPos, int yPos, bool bForced);

	/**
		This method is called when an unit should attack to another unit/structure
		\param	pTargetObject	the target unit/structure
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doAttackObject(const ObjectBase* pTargetObject, bool bForced);

	/**
		This method is called when an unit should attack to another unit/structure
		\param	TargetObjectID	the ID of the other unit/structure
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doAttackObject(Uint32 TargetObjectID, bool bForced);

	/**
		This method is called when an unit should attack to another unit/structure using salving weapons
		if it has some otherwise it fallback to doAttackObject
		\param	TargetObjectID	the ID of the other unit/structure
		\param	bForced	true, if the unit should ignore everything else
	*/
	virtual void doSalveAttackObject(Uint32 TargetObjectID, bool bForced);
	/**
		This method is called when an unit should change it's current attack mode
		\param	newAttackMode	the new attack mode
	*/
	void doSetAttackMode(ATTACKMODE newAttackMode);
	/**
		This method is called when an unit should cancel giving order (stop moving/following)
	*/
	virtual void doCancel();
    virtual void handleDamage(int damage, Uint32 damagerID, House* damagerOwner);

	virtual void doRepair() { };

	/**
        Is this object in a range we are guarding. If yes we shall react.
        \param  object  the object to check
	*/
	bool isInGuardRange(const ObjectBase* object) const;

    /**
        Is this object in a range we want to attack. If no, we should stop following it.
        \param  object  the object to check
    */
	bool isInAttackRange(const ObjectBase* object) const;

    /**
        Is this object in a range we can follow.
    */
	bool isInFollowingRange() const;
    /**
        Is this object in a range we can attack.
        \param  object  the object to check
    */
	bool isInWeaponRange(const ObjectBase* object) const;

    /**
        Is this object is nearer than current target
        \param  object  the object to check
    */
	bool isNearer(const ObjectBase* object) const ;

	void setAngle(int newAngle);

	virtual void setTarget(const ObjectBase* newTarget);
	virtual void setOldTarget(const ObjectBase* oldTarget);
	virtual void swapOldNewTarget();
	virtual ObjectBase* getFellow();
	virtual ObjectBase* getOldFellow();
	virtual void setFellow(const ObjectBase* newFellow);
	virtual void setOldFellow(const ObjectBase* newFellow);
	virtual void swapOldNewFellow();

	void setGettingRepaired();

	inline void setGuardPoint(const Coord& newGuardPoint) { setGuardPoint(newGuardPoint.x, newGuardPoint.y); }

	void setGuardPoint(int newX, int newY);

	void setLocation(int xPos, int yPos);

	inline void setLocation(const Coord& location) { setLocation(location.x, location.y); }

	inline void setLeader(bool lead) { isGroupLeader = lead ; }

	inline bool isFollowing() { return (bFollow && fellow && fellow.getObjPointer() !=NULL);}

    inline void setDestination(int newX, int newY) {
        if((destination.x != newX) || (destination.y != newY)) {
            ObjectBase::setDestination(newX, newY);
            clearPath();
        }
    }

    inline void setDestination(const Coord& location) { setDestination(location.x, location.y); }

	virtual void setPickedUp(UnitBase* newCarrier);

	/**
        Updates this unit.
        \return true if this unit still exists, false if it was destroyed
	*/
	virtual bool update();

	virtual bool canPass(int xPos, int yPos) const;

	virtual bool hasBumpyMovementOnRock() const { return false; }

	int getWeight();

	/**
        Returns how fast a unit can move over the specified terrain type.
        \param  terrainType the type to consider
        \return Returns a speed factor. Higher values mean slower.
	*/
	virtual float getTerrainDifficulty(TERRAINTYPE terrainType) const { return 1.0f; }

	virtual int getCurrentAttackAngle() const;

    virtual float getMaxSpeed() const;


	inline void clearPath() {
        pathList.clear();
        nextSpotFound = false;
        recalculatePathTimer = 0;
        nextSpotAngle = INVALID;
        noCloserPointCount = 0;
    }

	inline bool isDestoyed() const { return destroyed; }

	inline bool isTracked() const { return tracked; }

	inline bool isTurreted() const { return turreted; }

	inline bool isLeader() const { return isGroupLeader; }

	inline bool isMoving() const { return moving || justStoppedMoving; }

	inline bool isSalving() const { return salving; }

	inline virtual bool isIdle() const { return respondable && !target && !moving && !justStoppedMoving && attackMode !=STOP &&
			destination==location && idle && !goingToRepairYard ; }

	inline bool wasDeviated() const { return (owner->getHouseID() != originalHouseID); }

	inline int getAngle() const { return drawnAngle; }

	inline ATTACKMODE getAttackMode() const { return attackMode; }

	inline const Coord& getGuardPoint() const { return guardPoint; }

	virtual void playAttackSound();

	inline float getxSpeed() const { return xSpeed; }
	inline float getySpeed() const { return ySpeed; }

	inline float getRegulatedSpeed() 			const { return regulatedSpeed; }
	inline void  setRegulatedSpeed(float speed) 	  { regulatedSpeed = speed;}

	virtual inline BulletID_enum getBulletType() { return (BulletID_enum)bulletType;}

protected:

	virtual void attack();
	virtual void salveAttack(Coord Pos, Coord Target);
	virtual bool checkSalveRealoaded(bool stillSalvingWhenReloaded);

    virtual void releaseTarget();
    virtual void engageFollow(ObjectBase* pTarget);
	virtual void engageTarget();
	virtual void move();

    virtual void bumpyMovementOnRock(float fromDistanceX, float fromDistanceY, float toDistanceX, float toDistanceY);

    virtual void party();
	virtual void navigate();

    /**
        When the unit is currently idling this method is called about every 5 seconds.
    */
	virtual void idleAction();

	virtual void setSpeeds();

	virtual void targeting();

	virtual void turn();
	void turnLeft();
	void turnRight();

	void quitDeviation();

	bool SearchPathWithAStar();

	void drawFire(int x, int y);
    void drawSmoke(int x, int y);

	// constant for all units of the same type
    bool    tracked;                ///< Does this unit have tracks?
	bool    turreted;               ///< Does this unit have a turret?
	bool 	destroyed;				///< Does this unit is already destroyed ?
    int     numWeapons;             ///< How many weapons do we have?
    int 	salveWeapon;
    int     bulletType;             ///< Type of bullet to shot with

    // unit state/properties
    Coord   guardPoint;             ///< The guard point where to return to after the micro-AI hunted some nearby enemy unit
	Coord	attackPos;              ///< The position to attack
    bool	goingToRepairYard;      ///< Are we currently going to a repair yard?
	bool    pickedUp;               ///< Were we picked up by a carryall?
    bool    bFollow;                ///< Do we currently follow some other unit (specified by ObjectBase::fellow)?

    bool	isGroupLeader;			///< Is this unit a group leader of unit selection
    bool    moving;                 ///< Are we currently moving?
    bool    turning;                ///< Are we currently turning?
    bool    justStoppedMoving;      ///< Do we have just stopped moving?
    bool	salving;				///< If the unit is in a salving mode
    bool	idle;					///< If the unit is idle
    float	regulatedSpeed;			///< Do the unit is under speed regulation (i.e formation moves) [0,++]
    float	xSpeed;                 ///< Speed in x direction
    float	ySpeed;                 ///< Speed in y direction
    float   bumpyOffsetX;           ///< The bumpy offset in x direction which is already included in realX
    float   bumpyOffsetY;           ///< The bumpy offset in y direction which is already included in realY

    float	targetDistance;         ///< Distance to the target
    float   oldtargetDistance;		///< Distance to the oldtarget
    Sint8   targetAngle;            ///< Angle to the target

    // path finding
    Uint8   noCloserPointCount;     ///< How often have we tried to dinf a path?
    bool    nextSpotFound;          ///< Is the next spot to move to already found?
    Sint8   nextSpotAngle;          ///< The angle to get to the next spot
	Sint32  recalculatePathTimer;   ///< This timer is for recalculating the best path after x ticks
    Coord	nextSpot;               ///< The next spot to move to
    std::list<Coord>    pathList;   ///< The path to the destination found so far

    Sint32  findTargetTimer;        ///< When to look for the next target?
	Sint32  primaryWeaponTimer;     ///< When can the primary weapon shot again?
	Sint32  secondaryWeaponTimer;   ///< When can the secondary weapon shot again?
	Sint32	salveWeaponTimer[MAX_SALVE]; ///< When can the salve weapons shot again?
	Sint32	salveWeaponDelaybase;	///< What is the delay between each salving weapons shot (init time)
	Sint32	salveWeaponDelay;		///< What is the delay between each salving weapons shot
	Sint32  oldTargetTimer;			///< When can we forgot the previous target ?
	Sint32  destroyedCountdown;		///< This is the final countdown

    // deviation
	Sint32          deviationTimer; ///< When to revert back to the original owner?

    // drawing information
	int drawnFrame;                 ///< Which row in the picture should be drawn
};

#endif //UNITBASE_H
