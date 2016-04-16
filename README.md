% MUtilities &ndash; README
% Created by LoRd_MuldeR &lt;<mulder2@gmx>&gt; &ndash; check <http://muldersoft.com/> for news and updates!

# Introduction #

The **MUtilities** library is a collection of routines and classes to extend the [*Qt cross-platform framework*](http://qt-project.org/). It contains various convenience and utility functions as well as wrappers for OS-specific functionalities. The library was originally created as a "side product" of the [**LameXP**](http://lamexp.sourceforge.net/) application: Over the years, a lot of code, **not** really specific to *LameXP*, had accumulated in the *LameXP* code base. Some of that code even had been used in other projects too, in a "copy & paste" fashion &ndash; which had lead to redundancy and much complicated maintenance. In order to clean-up the LameXP code base, to eliminate the ugly redundancy and to simplify maintenance, the code in question has finally been refactored into the **MUtilities** (aka "MuldeR's Utilities for Qt") library. This library now forms the foundation of *LameXP* and various [*other OpenSource projects*](https://github.com/lordmulder).


# License

This library is free software. It is released under the terms of the [**GNU Lesser General Public License (LGPL), Version 2.1**](https://www.gnu.org/licenses/lgpl-2.1.html).

    MUtilities - MuldeR's Utilities for Qt
    Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>. Some rights reserved.
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.


# Acknowledgement

The following third-party code is used in the MUtilities library:

* **Keccak/SHA-3 Reference Implementation**
  Implementation by the Keccak, Keyak and Ketje Teams, namely, Guido Bertoni, Joan Daemen, Michaël Peeters, Gilles Van Assche and Ronny Van Keer
  No Copyright / Dedicated to the Public Domain

* **Natural Order String Comparison**
  Copyright (C) 2000, 2004 by Martin Pool <mbp@sourcefrog.net>
  Released under the zlib License

* **Adler-32 Checksum Algorithm (from zlib)**
  Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler
  Released under the zlib License

&nbsp;  

**e.o.f.**
