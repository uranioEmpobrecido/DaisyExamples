#include <string.h>
#include "daisy_seed.h"
#include "dsp/dsp.h"
#include "dsp/voice.h"
#include "stmlib/algorithms/note_stack.h"
#include "stmlib/algorithms/voice_allocator.h"

#define PIN_GATE_IN_1 3
#define PIN_GATE_IN_2 4

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace plaits;
using namespace stmlib;

/** Daisy patch gate inputs */
enum GateInput
{
	GATE_IN_1,	  /**< */
	GATE_IN_2,	  /** <*/
	GATE_IN_LAST, /**< */
};

enum LEDs
{
	LED_1 = 0,	  /**< */
	LED_2,	  /**< */
	LED_3,	  /**< */
	LED_4,	  /**< */
	LED_5,	  /**< */
	LED_6,	  /**< */
	LED_7,	  /**< */
	//	LED_8,	  /**< */
	LED_LAST, /**< */
};

enum Pots
{
	POT_X = 0,	  /**< */
	POT_Y,	  /**< */
	POT_Z,	  /**< */
	POT_A,	  /**< */
	POT_B,	  /**< */
	POT_DRY,	  /**< */
	POT_LAST, /**< */
};

enum Trims
{
	TRIM_AV1 = 0,	  /**< */
	TRIM_AV2,	  /**< */
	TRIM_AV3,	  /**< */
	TRIM_AV4,	  /**< */
	TRIM_LAST, /**< */
};

#define VOICE_MAX 8
#define VOICE_NUM VOICE_MAX

#define INTERNAL_LPG 1
#define REVERB 0


// Declare a DaisySeed object called hardware
DaisySeed hardware;
MidiHandler midi;
GateIn gate_input[GATE_IN_LAST]; /**< Gate inputs  */

//Analog variables
float POTReadings[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float TRIMMERReadings[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float CVReadings[4] = {0.0f, 0.0f, 0.0f, 0.0f};

uint8_t midiCVNotes[4] = {0, 0, 0, 0};

Led LEDs[LED_LAST];
//Configure and initialize button
Switch button;

//Trigger variables
uint32_t triggerState[2] = {0, 0};

//daisysp::Limiter limiter;
stmlib::Limiter limiter;

// Voices
volatile uint8_t ui_cpu_check = 0;
volatile int16_t voices_used = VOICE_MAX;

// Poly allocator
VoiceAllocator<VOICE_NUM> voice_allocator;

// Plaits stuff
Voice voice[VOICE_NUM];
Modulations modulations[VOICE_NUM];
Patch patch[VOICE_NUM];
Voice::Frame output[48];

float transposition_;
float octave_;
char shared_buffer[VOICE_NUM][16384];

static void PlaitsUiInit(void)
{
	BufferAllocator allocator;

	//	note_stack.Init();
	voice_allocator.Init();
	voice_allocator.set_size(VOICE_NUM);
	for (uint16_t i = 0; i < VOICE_NUM; i++)
	{
		allocator.Init(shared_buffer[i], 16384);
		voice[i].Init(&allocator);
		patch[i].decay = 0.5f;
		patch[i].engine = 0;
		patch[i].frequency_modulation_amount = 0.0f;
		patch[i].harmonics = 0.0f;
		patch[i].lpg_colour = 0.5f;
		patch[i].morph = 0.0f;
		patch[i].morph_modulation_amount = 0.0f;
		transposition_ = 0.0f;
		patch[i].note =  72.0f;
		patch[i].timbre = 0.0f;
		patch[i].timbre_modulation_amount = 0.0f;

#if INTERNAL_LPG
		modulations[i].level_patched = 0;
		modulations[i].trigger_patched = 1;
#else
		modulations[i].level_patched = 0;
		modulations[i].trigger_patched = 0;
#endif
	}

	octave_ = 0.2f;
}

/**************************************************/
/*         AUDIO PROCESSING                       */
/**************************************************/


static void Callback(float *in, float *out, size_t size)
{
	// CPU VOICE CONTROL !
	if (ui_cpu_check > 2) {
		if (voices_used > 1) {
			voices_used--;
			voice_allocator.set_size(voices_used);
			voice_allocator.Clear();
		}
	} else {
		ui_cpu_check++;
	}

	memset(out, 0.f, size * sizeof(out));

	for (uint16_t voice_index = 0; voice_index < voices_used; voice_index++)
	{
		voice[voice_index].Render(patch[voice_index], modulations[voice_index], (Voice::Frame*)(output), size/2);

#if INTERNAL_LPG
		for (size_t i = 0; i < size; i += 2)
		{
			out[i] += (static_cast<float>(output[i/2].out) / 32768.0f) * 0.5f;
			out[i + 1] += (static_cast<float>(output[i/2].aux) / 32768.0f) * 0.5f;
		}
#else
#endif

	}
}

/**************************************************/
/*         MIDI PROCESSING                       */
/**************************************************/
// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
	
}

int main(void)
{
	// Configure and Initialize the Daisy Seed
	hardware.Configure();

	hardware.Init();
	hardware.SetAudioBlockSize(12);

	// Initialize MIDI
	// midi.Init(MidiHandler::INPUT_MODE_UART1, MidiHandler::OUTPUT_MODE_NONE);
	// Caution with LED8, don't use it if you're using MIDI, they share the same pin
	// LED8.Init(hardware.GetPin(14), false);

	// Start stuff.
	// midi.StartReceive();

	// Initialize leds
	Led buttonLED;
	//	Led LED1, LED2, LED3, LED4, LED5, LED6, LED7;// LED8;

	buttonLED.Init(hardware.GetPin(6), false);

	for (uint16_t i = 0; i < LED_LAST; i++)
	{
		LEDs[i].Init(hardware.GetPin(7+i), false);
	}

	// Gate Inputs
	dsy_gpio_pin pin;
	pin = hardware.GetPin(PIN_GATE_IN_1);
	gate_input[GATE_IN_1].Init(&pin);

	//Set button to pin 28, to be updated at a 1kHz  samplerate
	button.Init(hardware.GetPin(5), 1000);

	AdcChannelConfig cfg[8];

	// Init ADC channels
	// CV inputs
	cfg[0].InitSingle(hardware.GetPin(15));
	cfg[1].InitSingle(hardware.GetPin(16));
	cfg[2].InitSingle(hardware.GetPin(17));
	cfg[3].InitSingle(hardware.GetPin(18));
	// Potentiometer
	cfg[4].InitSingle(hardware.GetPin(19));
	cfg[5].InitSingle(hardware.GetPin(21));
	// Multiplexed potentiometers.
	cfg[6].InitMux(hardware.GetPin(20), 4, hardware.GetPin(1), hardware.GetPin(2));
	/// Multiplexed attenuverters.
	cfg[7].InitMux(hardware.GetPin(22), 4, hardware.GetPin(1), hardware.GetPin(2));

	// Initialize the adc with the config we just made
	hardware.adc.Init(cfg, 8);

	// Start reading values
	hardware.adc.Start();

	PlaitsUiInit();

	// Start audio callback
	hardware.StartAudio(Callback);

	// Endless loop
	while (1)
	{
		//midi.Listen();
		// Handle MIDI Events
		while (midi.HasEvents())
		{
			//HandleMidiMessage(midi.PopEvent());
		}

		// Read the gate input 1
		if(gate_input[GATE_IN_1].State()){
			int8_t voice = voice_allocator.NoteOn(midiCVNotes[0]);
			if (voice < voices_used) {
				modulations[voice].trigger = 1.0f;
				patch[voice].lpg_colour = static_cast<float>(64) / 127;
				if ((voice < LED_LAST) && (button.NotPressed()))
				{
					LEDs[voice].Set(1.0f);
				}

				patch[voice].note = static_cast<float>(midiCVNotes[0]);
				}
		} else {
			uint8_t voice = voice_allocator.NoteOff(midiCVNotes[0]);
			if (voice < voices_used)
			{
				modulations[voice].trigger = 0.0f;
				if ((voice < LED_LAST) && (button.NotPressed()))
				{
					LEDs[voice].Set(0.0f);
				}
			}
		}

		//Debounce the button
		button.Debounce();
		// Update the led to reflect the set value
		buttonLED.Update();

		// Just read all the interface
		CVReadings[0] = 1.0f - hardware.adc.GetFloat(0);
		CVReadings[1] = 1.0f - hardware.adc.GetFloat(1);
		CVReadings[2] = 1.0f - hardware.adc.GetFloat(2);
		CVReadings[3] = 1.0f - hardware.adc.GetFloat(3);

		//Calculate CV note to MIDI
		midiCVNotes[0] = (uint8_t)(CVReadings[0] * 127 - 64);

		POTReadings[POT_X] = 1.0f - hardware.adc.GetFloat(4);
		POTReadings[POT_DRY] = 1.0f - hardware.adc.GetFloat(5);

		POTReadings[POT_Y] = 1.0f - hardware.adc.GetMuxFloat(6, 0);
		POTReadings[POT_Z] = 1.0f - hardware.adc.GetMuxFloat(6, 1);
		POTReadings[POT_A] = 1.0f - hardware.adc.GetMuxFloat(6, 2);
		POTReadings[POT_B] = 1.0f - hardware.adc.GetMuxFloat(6, 3);

		TRIMMERReadings[TRIM_AV1] = 1.0f - hardware.adc.GetMuxFloat(7, 0); // * 2.0f;
		TRIMMERReadings[TRIM_AV2] = 1.0f - hardware.adc.GetMuxFloat(7, 1); // * 2.0f;
		TRIMMERReadings[TRIM_AV3] = 1.0f - hardware.adc.GetMuxFloat(7, 2); // * 2.0f;
		TRIMMERReadings[TRIM_AV4] = 1.0f - hardware.adc.GetMuxFloat(7, 3); // * 2.0f;

		for (uint16_t i = 0; i < VOICE_NUM; i++)
		{
			patch[i].decay = POTReadings[POT_A];
			patch[i].frequency_modulation_amount = 0.0f;
			patch[i].harmonics = POTReadings[POT_X];
			patch[i].morph = POTReadings[POT_Z];
			patch[i].morph_modulation_amount = 0.0f;
			patch[i].timbre = POTReadings[POT_Y];
			patch[i].timbre_modulation_amount = 0.0f;
		}
		if (button.RisingEdge())
		{
			for (uint16_t i = 0; i < VOICE_NUM; i++)
			{
				patch[i].engine = (patch[i].engine + 1) % voice[i].engine_size();
			}

			// reset number of voices
			if (voices_used != VOICE_MAX)
			{
				voices_used = VOICE_MAX;
				voice_allocator.set_size(voices_used);
			}

			buttonLED.Set(1.f);
		} else if (button.FallingEdge()) {
			buttonLED.Set(0.f);
		} else if ((button.Pressed())) {
			for (uint16_t i = 0; i < 7; i++)
			{
				LEDs[i].Set(0.0f);
			}
			LEDs[patch[0].engine].Set(1.0f);
			buttonLED.Set(0.f);
		}

		for (uint16_t i = 0; i < LED_LAST; i++)
		{
			LEDs[i].Update();
		}

		ui_cpu_check = 0;

		transposition_ = POTReadings[POT_X] * 2.0f - 1.0f;
	}
}
