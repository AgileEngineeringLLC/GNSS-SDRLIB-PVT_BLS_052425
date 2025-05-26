# GNSS-SDRLIB-PVT_BLS_052425
Modified version of GNSS-SDRLIB

Versions:
1. GNSS-SDRLIB-PVT_BLS_042725. Released 4/27/25. Uses SDL2 for GUI and has BLS for the PVT 
estimation.
2. GNSS-SDRLIB-PVT_BLS_052425. Released 5/25/25. Several modifications that significantly
improve robustness. SDL2 has been replaced by Ncurses for GUI. The SV channels now reset
themselves by zeroing out structs and flags, rather than closing and restarting each
channel (thread).

Purpose. This is a significantly-modifed version of GNSS-SDRLIB. Instead of requiring a 
TCP link to RTKLIB for positioning, it has its own least squares solver built-in. 

Limitations. This is a highly stripped-down version of GNSS-SDRLIB, in an attempt to 
create a more truly real-time and robust GPS SDR. This version is GPS L1 only, Linux only 
(not Windows), and only currently works with RTLSDR or bladeRF front ends.

Videos. The following videos are available on Youtube:
- Overview of Version 052425: https://www.youtube.com/watch?v=-zpYcuX1XJM
- Three part series on GNSS-SDRLIB-PVT:
-- Part 1. Overview of Features and Modifications. Link: https://youtu.be/VLlu77O3v
-- Part 2. Hardware Used, and Software Builds. Link: https://youtu.be/R3IFhFoblQU 
-- Part 3. Demo of Playback and Real-Time Modes. Link: https://youtu.be/8pV1hX6IkFk 

Acknowledgements. This software is a modified version of the original 2014 GNSS-SDRLIB 
code (https://github.com/taroz/GNSS-SDRLIB). Please refer to the videos listed above for 
a description of features and the modifications made. The author is very grateful for 
Taro Suzuki's original development of GNSS-SDRLIB, as it is a terrific learning tool, 
is thoughtfully written, with clear variables and code comments. 

License. This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software Foundation; 
either version 2 of the License, or (at your option) any later version. This program is 
distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details. You should have received a copy of the 
GNU General Public License along with this program; if not, write to the Free Software 
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

Keywords. RTLSDR, bladeRF, SDR, software defined radio, GPS, GNSS, L1, GNSS-SDRLIB, 
GNSS-SDRLIB-PVT, PVT, C, SDL2, NML.

To Do List (not in any particular order)
- Tighten up mutexes for the data accessed by the GUI
- Add L5 signals
- Incorporate SBAS corrections to pseudoranges
- Test the Bancroft function and include it at startup if helpful
- Fully implement the Ubuntu real-time kernal

