# (TODO: your game's title)

Author: (TODO: your name)

Design: (TODO: In two sentences or fewer, describe what is new and interesting about your game.)

Text Drawing: We render the text at runtime and store the texture for each glyph in a map.
As recommended/required, we used freetype and OpenGL to render the text, and harfbuzz to shape the text.
The glyphs are loaded and added to the glyph map the first time it appears, and they are looked up usin glyph ID.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

(TODO: describe the controls and (if needed) goals/strategy.)

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

