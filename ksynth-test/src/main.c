#include <stdio.h>
#include <ksynth.h>
#include <soundfont/sf2.h>
#include <windows.h>
#include <conio.h>



struct KSynth* synth;

void Test1()
{
    char key;
    int i = 0;
    synth = ksynth_new("PLM060.wav", 48000, 2, MAX_POLYPHONY, true);
    while(1)
    {
        //printf("Sending notes... (%d Times)\n", i++);
        //key = _getch();
        //if(key == 'q')
        //    break;
        ksynth_note_on(synth, 1, 60, 127);
        Sleep(1);
        ksynth_note_off(synth, 1, 60, 127);
        Sleep(1);
    }
    ksynth_free(synth);
    // Stuff
}

void Test2()
{
    struct Sample sample;
    //ksynth_load_soundfont("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2");
    if(load_preset_to_sample("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2", 0, 0, 60, 2.0f, &sample))
    {
      printf("Loaded sample: %u frames @ %u Hz\n", sample.length, sample.sample_rate);
      free(sample.audio_data);
    }
}

void Test3()
{
    struct Sample samples[MAX_KEYS];

    for (int note = 0; note < MAX_KEYS; ++note)
    {
        if (load_preset_to_sample("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2", 0, 0, note, 2.0f, &samples[note]))
        {
            printf("Note %d loaded: %u frames\n", note, samples[note].length);
        }
        else
        {
            samples[note].audio_data = NULL;
        }
    }
}


int main(int argc, char** argv) {
    printf("hello world!\n");
    //Test1();
    //Test2();
    Test3();
    return 0;
}
