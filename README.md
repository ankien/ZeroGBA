<h1>ZeroGBA</h1>
ZeroGBA is a GBA emulator I decided to work on for fun during Summer 2021 (the beginning of my last year in university).<br>
Initially the goal was just to boot most commercial games with stable 60 fps framerate.<br>
However, things moved quicker than I expected and I ended up making a fairly accurate and performant emulator.<br><br>
<img src="/ZeroGBA/non-code/screenshots/doom.png" width="400">
<h2>Some Demos</h2>
  <p float="left">
    <img src="/ZeroGBA/non-code/screenshots/m7Demo.gif" width="400">
    <img src="/ZeroGBA/non-code/screenshots/mmbn.gif" width="400">
    <img src="/ZeroGBA/non-code/screenshots/kirby.gif" width="400">
    <img src="/ZeroGBA/non-code/screenshots/armwrestler pass.gif" width="400">
  </p>
<h2>System Controls</h2>
  Below "Xbox 360" controls describe position, they should work for other controllers too.<br><br>
  <p><b>A Button: </b>Space / Xbox 360 Controller B</p>
  <p><b>B Button: </b>S / Xbox 360 Controller A</p>
  <p><b>Select: </b>Right Shift / Xbox 360 Controller Back</p>
  <p><b>Start: </b>Enter aka Return / Xbox 360 Controller Start</p>
  <p><b>D-pad: </b>Arrow Keys / Xbox 360 Controller D-pad</p>
  <p><b>L Button: </b>A / Xbox 360 Left Shoulder Button</p>
  <p><b>R Button: </b>D / Xbox 360 Right Shoulder Button</p><br>
<h3>Emulator Controls</h3>
  <p><b>Close Window: </b>Esc, or the window X bar</p>
  <p><b>Frame Skip: </b>Tab / Xbox 360 Right Trigger Button / Xbox 360 Left Stick Button (Toggle)</p>
  <p><b>Toggle Fullscreen: </b>Alt+Enter</p>
  <p><b>Trace Log: </b>T (Toggle, when enabled in GBAMemory.hpp)</p><br>
  <h2>Usage</h2>
  <i>Since there are no user builds yet, this is merely how it's planned to work.</i><br><br>
  <code>ZeroGBA.exe [-options] [your rom.gba]</code><br>
  This emulator is only tested to be compatible with Windows. ZeroGBA is a 64-bit program, so your CPU will need to be compatible.<br>
  You will need to either dump your own BIOS or download a <a href="https://github.com/Nebuleon/ReGBA/blob/master/bios/gba_bios.bin">replacement</a>.<br>
  Place it in the same directory as the executable.<br>
<h3>Compiling</h3>
  You will need Visual Studio with C++ Clang components installed.<br>
  You will also need a GBA BIOS, place it in <code>ZeroGBA\ZeroGBA</code>.<br>
  All other dependencies are included, so just open the project in Visual Studio and click the funny green button.  :-)<br>
<h2>Tests Passed</h2>
  <a href="https://github.com/DenSinH/FuzzARM">FuzzARM</a>: ALL<br>
  <a href="https://github.com/destoer/armwrestler-gba-fixed">ARMWrestler</a>: ALL<br>
  <a href="https://github.com/jsmolka/gba-tests">gba-tests</a>: arm, thumb, memory, ppu, bios, save<br>
