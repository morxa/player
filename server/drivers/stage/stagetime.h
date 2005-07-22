/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * this is the StageTime class, which gets current simulated time from
 * shared memory
 *
 */
#ifndef _STAGETIME_H
#define _STAGETIME_H

#include <sys/time.h>
#include "playertime.h"
#include "stage1p3.h"

class StageTime : public PlayerTime
{
  private:
    // the location in shared memory of the time feed
    struct timeval* simtimep;

    // Simulator lock bookkeeping data and init method
    //
    int lock_fd;
    int lock_byte;
    void InstallLock( int fd, int index )
              {lock_fd = fd; lock_byte = index;}
    void Lock();
    void Unlock();

  public:
    StageTime(const char *directory);
    ~StageTime() { } // empty destructor

    int GetTime(struct timeval* time);

};

#endif

