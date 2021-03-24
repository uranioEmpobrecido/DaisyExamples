# DIGI-Poly-Plaits

Polyphonic synth with Plaits audio engine.

Set INTERNAL_LPG to 1 to use Plaits internal LPG. This allows to use more voices.
Set INTERNAL_LPG to 0 to use 1 moog ladder filter per voice. 


IDEAS:
- Añadir SD para almacenar formas de onda de los modos Wavetable
- Utilizar un tipo de filtro más eficiente, la propia clase filter.h de mutable.
- Añadir estéreo, tanto paneo como detune de las voces.
- Cvs para controlar los parámetros
- MIDI CC para controlar los parámetros


Notes:

In order to compile:
-  add method to swith.h class :  inline bool NotPressed() const { return state_ == 0; }
- comment out #include "hid/parameter.h" in daisy.h
- Replace libdaisy/core/Makefile with libdaisy_core_Makefile, rename and set proper compiler path