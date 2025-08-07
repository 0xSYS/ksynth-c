#include <stdio.h>
#include <ksynth.h>
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


int main(int argc, char** argv) {
    printf("hello world!\n");
    Test1();
    return 0;
}
