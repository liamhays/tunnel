# tunnel
My first devkitSMS project, a small arcade game for the Game Gear

# Building
Install SDCC (probably from source, though it's available in the Arch
Linux package repos) and follow the directions at
[[devkitSMS][https://github.com/sverx/devkitSMS]] to set up
devkitSMS. I keep SMSlib.h, SMSlib\_GG.h, and crt0\_sms.rel checked
into this repository, and I will try to keep them up-to-date as
devkitSMS is updated.

Once devkitSMS' tools are installed, the Makefile should do the
rest. Compile with `make`, and the output ROM is `tunnel.gg`.

# Playing
You are the pilot of the space cruiser *Purple*, and your job is to
get as far as possible into deep space without colliding with the
yellowite stone columns of the evil alien forces, the Zragchlomphs.

At the title screen, select your desired difficulty level. "Hard" adds
a Zragchlomph ship you have to avoid.

Once in the game, the great cruiser *Purple* is constantly falling
unless you press the 2 button to make it rise again. Avoid hitting the
columns and the green ship, if enabled. The START button will let you
pause, indicated by a small sound.

# Known bugs
Something's wrong when the ship collides with columns near the
ceiling: the position of the explosion seems off. I am yet to try this
on real hardware, so it's possible (though unlikely) that it's an
error in Emulicious, the emulator I use to test.

