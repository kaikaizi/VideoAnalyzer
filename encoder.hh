/*  Copyright (C) 2012  Liu Lukai	(liulukai@gmail.com)
    This file is part of VideoAnalyzer.

    VideoAnalyzer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 
#include "sketch.hh"

class VideoProp;
class Encoder{
public:
   Encoder(const char*[],const VideoProp&, const int&)throw(ErrMsg);
   ~Encoder();
protected:
   const static int buf_size=1024*3;
   int consumed, rem, used, ret;
   FILE *fsrc, *fdest;
   struct Xvid_t; Xvid_t* px;
   char* stream, *plane;
   Encoder(const Encoder&);
   Encoder& operator=(const Encoder&);
};
