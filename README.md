Just a GBA emulator I decided to work on for fun.<br>
The long-term goal is to boot most commercial games with stable 60 fps framerate.<br>
<h1>Current Progress</h1>
<img src="/Desu/non-code/mode 3 tonc demo.png" width="400">
<img src="/Desu/non-code/armwrestler pass.gif" width="400">
Passed ARMWrestler! (visual CPU test suite)<br>
<h2>System Controls</h2>
<p><b>A Button: </b>Space</p>
<p><b>B Button: </b>S</p>
<p><b>Select: </b>Right Shift</p>
<p><b>Start: </b>Enter/Return</p>
<p><b>D-pad: </b>Arrow Keys</p>
<p><b>L Button: </b>A</p>
<p><b>R Button: </b>D</p><br>
<h3>Emulator Controls</h3>
<p><b>Close Window: </b>Esc, or the window X bar</p>
<p><b>Frame Skip: </b>Tab</p>
<p><b>Fullscreen: </b>Alt+Enter</p><br>
<h2>Usage</h2>
<i>Since there are no user builds yet, this is merely how it's planned to work.</i><br><br>
<code>Desu.exe [-options] [your rom.gba]</code><br>
This emulator is only tested to be compatible with Windows.<br>
You will need to either dump your own BIOS or download a <a href="https://github.com/Nebuleon/ReGBA/blob/master/bios/gba_bios.bin">replacement</a>.<br>
Place it in the same directory as the executable.<br>
<h3>Compiling</h3>
You will need a GBA BIOS, place it in <code>Desu\Desu</code>.<br>
All other dependencies are included, so just open the project in Visual Studio and click the funny green button.  :)<br>
<h2>Tests Passed</h2>
<a href="https://github.com/DenSinH/FuzzARM">FuzzARM</a>: ALL<br>
<a href="https://github.com/destoer/armwrestler-gba-fixed">ARMWrestler</a>: ALL<br>
<a href="https://github.com/jsmolka/gba-tests">gba-tests</a>: arm, thumb, memory<br>
