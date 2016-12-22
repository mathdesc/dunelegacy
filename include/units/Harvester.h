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

#ifndef HARVESTER_H
#define HARVESTER_H

#include <units/TrackedUnit.h>
#include <structures/Refinery.h>

typedef enum {DISTANCE, DENSITY} HarvesterPreference;

class Harvester : public TrackedUnit
{
public:


	Harvester(House* newOwner);
	Harvester(InputStream& stream);
	void init();
	virtual ~Harvester();

	virtual void save(OutputStream& stream) const;

	void blitToScreen();

	Refinery* findRefinery();
	void checkPos();
	virtual void deploy(const Coord& newLocation);
	void destroy();
	virtual void drawSelectionBox();
	void handleDamage(int damage, Uint32 damagerID, House* damagerOwner);

	void handleReturnClick();

    /**
        Order this harvester to return to a refinery.
    */
	void doReturn();

    /**
        Order this harvester to re-deploy.
    */
	void doDeploy(Coord location);

	void move();
	void setAmountOfSpice(float newSpice);
	void setReturned();

	void setDestination(int newX, int newY);
	inline void setGuardPoint(const Coord& location) { setGuardPoint(location.x, location.y); }

	void setGuardPoint(int newX, int newY);
    inline void setDestination(const Coord& location) { setDestination(location.x, location.y); }

    void setFellow(const ObjectBase* newFellow);
	void setTarget(const ObjectBase* newTarget);

	bool canAttack(const ObjectBase* object) const;

	float extractSpice(float extractionSpeed);
	virtual float getMaxSpeed() ;

	inline float getAmountOfSpice() const { return spice; }
	inline bool isReturning() const { return returningToRefinery; }
	bool isHarvesting() const;
	bool isIdle() const;

	Coord startProspection(const Coord & zone);
	bool findSpice(const Coord & zone, Coord &spicesite, bool noPropection = false);

	inline std::vector<std::pair<Coord,Uint32>> getProspectionSamples() {return prospectionSamples;}

	inline Coord getProspectionSamplesSite (int index) {
		if (!prospectionSamples.empty() && index >=0 && index <= (int)prospectionSamples.size()-1)
			return prospectionSamples[index].first;
		else
			return Coord::Invalid();
	}

	inline int getProspectionSamplesNumber () {
		return prospectionSamples.size() ;
	}

	inline bool getProspectionSamplesIsEmpty() {
		return prospectionSamples.empty() ? true : false ;
	}


	inline bool hasProspectionSamples (const Coord& site) {

		if (site.isInvalid()) return false;

		auto it=std::find_if(prospectionSamples.begin(), prospectionSamples.end(), [=](std::pair<Coord,Uint32> &item)  { return item.first == site;} );
		if (it !=  prospectionSamples.end()) {
			return true;
		} else
			return false;

	}

	inline bool addProspectionSample(const Coord& site, Uint32 extraction_speed) {


		if (site.isInvalid()) return false;

		auto it=std::find_if(prospectionSamples.begin(), prospectionSamples.end(), [=](std::pair<Coord,Uint32> &item)  { return item.first == site;} );
		if (it !=  prospectionSamples.end()) {
			// we already found a prospected site at this location...
			auto it=std::find_if(prospectionSamples.begin(), prospectionSamples.end(), [=](std::pair<Coord,Uint32> &item)  { return item.first == site && item.second == extraction_speed ;} );
			if (it ==  prospectionSamples.end()) {
				// ... but the spice density has changed since (less or more spice probably)
				// update it !
				prospectionSamples.erase(it);
				prospectionSamples.emplace_back(std::make_pair(site,extraction_speed));
				return true;
			} else 	return false;
		} else {
			prospectionSamples.emplace_back(std::make_pair(site,extraction_speed));
			return true;
		}
	}

	inline std::pair<Coord,Uint32> getProspectionSampleBestDensity() {
		Uint32 ext_speed = 0;
		Coord loc = Coord::Invalid();
		std::vector<std::pair<Coord,Uint32>>::iterator it;


			do {
				it=std::find_if(prospectionSamples.begin(), prospectionSamples.end(),
										[&ext_speed](std::pair<Coord,Uint32> &item) mutable
										{
												return item.second > ext_speed;
										}
									);

				if (it != prospectionSamples.end()) {
					loc = it->first ;
					ext_speed = it->second ;
				}
			} while (it !=  prospectionSamples.end()  );


		if (it !=  prospectionSamples.end() || ext_speed > 0 ) {
			return std::make_pair(loc,ext_speed);
		} else {
			return std::make_pair(Coord::Invalid(),0);
		}

	}

	inline std::pair<Coord,Uint32> getProspectionSampleBestDistance() {
		std::vector<std::pair<Coord,Uint32>>::iterator it;
		float distance = 100000000.0f;
		Coord loc = Coord::Invalid();
		int ext_speed = 0;
		do {
			it=std::find_if(prospectionSamples.begin(), prospectionSamples.end(),
													[&distance, this](std::pair<Coord,Uint32> &item) mutable
													{
															return  distanceFrom(item.first,location) < distance ;
													}
												);


			if (it != prospectionSamples.end()) {
				loc = it->first;
				ext_speed = it->second;
				distance = distanceFrom(it->first,location);
			}
		} while (it !=  prospectionSamples.end()  );

		if (it !=  prospectionSamples.end() || distance <  100000000.0f ) {
			return std::make_pair(loc,ext_speed);
		} else {
			return std::make_pair(Coord::Invalid(),0);
		}

	}


	inline bool removeProspectionSample(const Coord& site) {

		if (site.isInvalid()) return false;

		auto it=std::remove_if(prospectionSamples.begin(), prospectionSamples.end(),
						[=](std::pair<Coord,Uint32> & item) { return item.first == site ;} );

		if (it != prospectionSamples.end()) {
			prospectionSamples.erase(it);
			return true;
		} else
			return false;

#if 0
		auto it=std::find_if(prospectionSamples.begin(), prospectionSamples.end(),
				[=](std::pair<Coord,Uint32> & item) { return item.first == site ;} );

		if (it != prospectionSamples.end()) {
		  // swap the sample to be removed with the last element
		  // and remove the item at the end of the container
		  // to prevent moving all items after sample by one
		  //std::swap(*it, prospectionSamples.back());
		  //prospectionSamples.pop_back();
		  prospectionSamples.erase(it);

		  return true;
		} else {
			return false;
		}
#endif
	}

	inline void removeAllProspectionSample() {
		prospectionSamples.clear();
	}

	float   currentMaxSpeed;    ///< The current maximum allowed speed

	inline HarvesterPreference getPreference () {
		return preference;
	}

	inline void setPreference (HarvesterPreference pref) {
		preference = pref;
	}


private:

    virtual void setSpeeds();

    // harvester state
	bool	harvestingMode;         ///< currently harvesting
    bool    returningToRefinery;    ///< currently on the way back to the refinery
	float   spice;                  ///< loaded spice
	float   oldspice;               ///< loaded spice
	Uint32  spiceCheckCounter;      ///< Check for available spice on map to harvest
	int 	famine;					///<

	HarvesterPreference preference;

protected :
	std::vector<std::pair<Coord,Uint32>> prospectionSamples; ///< Spice samples Coordinates found during prospection
};

#endif // HARVESTER_H
