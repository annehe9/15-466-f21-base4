# Beebles Dating Simulator

Authors: Anne He and George Ralph

Design: This game gives players a unique dialog tree for each character they converse with. Each of the four Beebles have their own character art with differing expressions, to help indicate their moods.

Text Drawing: We render the text at runtime and store the texture for each glyph in a map.
As recommended/required, we used freetype and OpenGL to render the text, and harfbuzz to shape the text.
The glyphs are loaded and added to the glyph map the first time it appears, and they are looked up usin glyph ID.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Get the once in a lifetime chance to talk with all four members of one of the biggest acts in music: The Beebles! Each of the four members: John Lemon, Pete McCafferty, Geoff Harrington, and Richard Stripe have their own dialog trees to converse with.

Use the up and down arrow keys to select a dialog response option. Press enter to say that option. 

There is a bit of drama going on within the band, see if you can talk to each character to resolve the issue!

Sources:

Used 15-466 Game0 and Game4 base code.

For text rendering, referenced:

https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c

https://www.freetype.org/freetype2/docs/tutorial/step1.html

https://learnopengl.com/In-Practice/Text-Rendering

https://harfbuzz.github.io/index.html

https://github.com/ChunanGang/TextBasedGame

https://github.com/lassyla/game4


This game was built with [NEST](NEST.md).

All characters and musical groups in this game are purely fictional. Any resemblance to actual persons living or dead is purely coincidental.