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

#ifndef OFILESTREAM_H
#define OFILESTREAM_H

#include "OutputStream.h"
#include <stdlib.h>
#include <string>

class OFileStream : public OutputStream
{
public:
	OFileStream();
	~OFileStream();

	bool open(const char* filename);
	bool open(std::string filename);
	void close();

	virtual void flush();

	// write operations

	void writeString(const std::string& str);

	void writeUint8(Uint8 x);
	void writeUint16(Uint16 x);
	void writeUint32(Uint32 x);
	void writeUint64(Uint64 x);
	void writeBool(bool x);
	void writeFloat(float x);
	void writeCoord(Coord c);

private:
	FILE* fp;
};

#endif // OFILESTREAM_H
