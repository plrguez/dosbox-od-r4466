DOSBox Configuration
====================

Default config file is '$HOME/.dosbox/dosbox-SVN.conf'
First time you start DOSBox it will be created.

I do not recommend to chage this defaults and use an specific config file to launch your game and override values.

To start DOSBox with a specific config file use the 'DOSBox-OD-Conf' launcher.

In the config you can set an autoexec, mapper file and override other defaults.

Example 1: 
Use a mapperfile, mount C drive and ISO CD file and start the program.
Override joystick to disable.
--------------------------------------------------------------------------------

    [autoexec]
    cls
    mount C /media/sdcard/ROMS/DOS/example1/HD
    imgmount D /media/sdcard/ROMS/DOS/example1/CD/CD.iso -t iso
    C:
    cd example1
    call example1.exe
    exit

    [joystick]
    joysticktype = none

    [sdl]
    mapperfile = mapper-example1.map

Example 2:
Use a mapperfile, mount C drive pointing to program location and D drive
pointing to Gravis Ultrasound files
Override Gravis Ultrasound and SoundBlaster configurations. 
--------------------------------------------------------------------------------

    [autoexec]
    cls
    mount C /media/sdcard/ROMS/DOS/example2/HD
    mount D /media/sdcard/ROMS/DOS/GUS
    C:
    cd example2
    call example2.exe
    exit

    [sblaster]
    sbtype  = none

    [gus]
    gus      = true
    gusrate  = 22050
    gusbase  = 240
    gusirq   = 5
    gusdma   = 3
    ultradir = D:\411

    [sdl]
    mapperfile = mapper-example1.map

KEY CONFIGURATION
=================

Keys
----

    Handheld buttons are taken as keyboard presses. These are the default values

    Button   Keyboard               Internal value  Comments
    -------  ---------------------  --------------  ----------
    L1      TAB                     "key 9"         mod key 1
    L2      PAGE UP                 "key 273"       unmaped
    R1      BACKSPACE               "key 8"         mod key 2
    R2      PAGE DOWN               "key 281"       unmaped
    L3      KP Divide               "key 267"       unmaped
    R3      KP Period               "key 266"       unmaped
    UP      UP                      "key 273" 
    DOWN    DOWN                    "key 274"
    LEFT    LEFT                    "key 276"
    RIGHT   RIGHT                   "key 275"
    SELECT  ESCAPE                  "key 27"
    START   RETURN                  "key 13"
    A       LEFT CONTROL            "key 306"
    B       LEFT ALT                "key 308"
    X       SPACE                   "key 32"
    Y       LEFT SHIFT              "key 304"
    POWER   HOME                    "key 278"       unmaped

    Note:
    PAGE UP (L2), PAGE DOWN (R2), KP Divide (L3), KP Period (R3) and HOME (Power) keys are by default unmaped so 
    they not sent their key value to dosbox but you can remap.

Joystick
--------
    Joystick is enabled by default.
    There are also 4 joystick axis (1/2 for left joystick, 3/4 for right)
    There are not joystick buttons or hat.
    In default config 'timed' has been changed to false to avoid drift.

DOSBox Mod keys
---------------
    They are used for combo keys

    - L1 is mod key 1 
    - R1 is mod key 2

Combo keys
----------
    - Power or L1 + Select 	= Menu
    - R2 or R1 + Start  	= Virtual mouse
    - L2 or R1 + Select 	= Virtual keyboard
    - L1 + Start        	= Mapper
    - L1 + R3           	= Exit

How to use keymapper
--------------------
    See details from upstream DOSBox here https://www.dosbox.com/wiki/Mapper

    Open keyboard mapper with L1+Start

    - You can move with cursor keys or left stick.
    - Select a key to map with the 'A' button. 
    - Push 'Add' button to assign to a device button
    - Press the device button to assing to the selected key
    - Unassign default mappings for buttons and keys using 'Next' and 'Del' buttons (see example for details)
    - Repeat for other keys
    - Save to the mapper file

    Now when you press the mapped buttons then the assigned keys are sent to DOSBox.

    Take into account that device buttons are sending their own keypresses to DOSBox (See RG350 Keys) so you
    must unnassign them to avoid unwanted keypresses (see examples).

    You can also map joystick axis detected by DOSBox.

Mapper file
-----------

    Default mapper file is '$HOME/.dosbox/mapper-SVN.map'

    If you do not have configured a mapper file in your config file or you start DOSBox directly with a DOS executable
    then default mapper file is used when change mappings.

    To stablish an specific mapper file in your config file use 'mapperfile' keyword at [sdl] section, it will be
    loaded or saved at the same location that your config file.

    See examples in DOSBox Configuration section.


EXAMPLES (Adapted from upstream dosbox wiki)
--------------------------------------------
    Q1. You want to have the X button to type a Z in DOSBox.

            A. Click on the Z on the keyboard mapper. Click "Add". Now press the X button on your device.

    Q2. If you click "Next" a couple of times, you will notice that the Z on your keyboard also produces an Z in DOSBox.

            A. Therefore select the Z again, and click "Next" until you have the Z on your keyboard. Now click "Del".

    Q3. If you try it out in DOSBox, you will notice that pressing X Zspace appear.

            A. The space on your device is still mapped to the X button as well! Click on the space in the keyboard mapper and search with "Next" until you find the mapped key space. Click "Del".

    Examples about remapping the joystick:

            You want to play some keyboard-only game with the joystick (it is assumed that the game is controlled by the arrows on the keyboard):

            Start the mapper, then click on one of the left keyboard arrow. EVENT should be key_left. Now click on Add and move your joystick in the respective direction, this should add an event to the BIND.
            Repeat the above for the missing three directions, additionally the buttons of the joystick can be remapped as well (fire/jump).
            Click on Save, then on Exit and test it with some game.

    You want to swap the y-axis of the joystick because some flightsim uses the up/down joystick movement in a way you don't like, and it is not configurable in the game itself:

            Start the mapper and click on Y- in the first joystick field. EVENT should be jaxis_0_1-.
            Click on Del to remove the current binding, then click Add and move your joystick downwards. A new bind should be created.
            Repeat this for Y+, save the layout and finally test it with some game.

How to compile for OD Beta
==========================

    Put your toolchain in PATH. In the configure string below toolchain is located in /opt/opendingux-toolchain

    export PATH=/opt/opendingux-toolchain/usr/bin:$PATH
    ./autogen.sh
    ./configure --host=mipsel-linux --with-sdl-prefix=/opt/opendingux-toolchain/mipsel-gcw0-linux-uclibc/sysroot/usr --enable-mips32r2 --disable-opengl --disable-alsa-midi --disable-dynamic-x86 --disable-fpu-x86 --enable-core-inline CXXFLAGS="-std=c++14 -g0 -O3 -G0 -mips32r2 -pipe -fno-builtin -fno-common -mno-shared -ffast-math -fomit-frame-pointer -fexpensive-optimizations -frename-registers -Wno-narrowing" LIBS="-lSDL_gfx -lSDL_image"