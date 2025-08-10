#include <stdio.h>
#include <ksynth.h>
#include <soundfont/sf2.h>
#include <windows.h>
#include <conio.h>







#if defined(_WIN32) || defined(_WIN64)
void SetTerminal()
{
    static HANDLE stdoutHandle, stdinHandle;
    static DWORD outModeInit, inModeInit;

    DWORD outMode = 0, inMode = 0;
    stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    stdinHandle = GetStdHandle(STD_INPUT_HANDLE);

    if(stdoutHandle == INVALID_HANDLE_VALUE || stdinHandle == INVALID_HANDLE_VALUE) 
    {
        exit(GetLastError());
    }

    if(!GetConsoleMode(stdoutHandle, &outMode) || !GetConsoleMode(stdinHandle, &inMode)) 
    {
        exit(GetLastError());
    }

    outModeInit = outMode;
    inModeInit = inMode;

    // Enable ANSI escape codes
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    // Set stdin as no echo and unbuffered
    inMode = (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

    if(!SetConsoleMode(stdoutHandle, outMode) || !SetConsoleMode(stdinHandle, inMode)) 
    {
        exit(GetLastError());
    }
    SetConsoleOutputCP(CP_UTF8); //Enabling unicode charset on windows console
}
#endif



struct KSynth* synth;

void Test1()
{
    char key;
    int i = 0;
    synth = ksynth_new("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2", 48000, 2, MAX_POLYPHONY, true);
    while(1)
    {
        //printf("Sending notes... (%d Times)\n", i++);
        //key = _getch();
        //if(key == 'q')
        //    break;
        ksynth_note_on(synth, 1, 2, 127);
        Sleep(1);
        ksynth_note_off(synth, 1, 2);
        Sleep(1);
    }
    ksynth_free(synth);
    // Stuff
}

void Test2()
{
    //struct Sample **samples;
    //ksynth_load_soundfont_samples("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2", samples);
    //if(load_preset_to_sample("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2", 0, 0, 60, 2.0f, &sample))
    //{
    //  printf("Loaded sample: %u frames @ %u Hz\n", sample.length, sample.sample_rate);
    //  free(sample.audio_data);
    //}

    /*
    struct Sample* sf2_samples = NULL;
    size_t sample_count = 0;

    // Load samples from a SoundFont
    ksynth_load_sf2_samples("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Arachno SoundFont Version 1.0.sf2", &sf2_samples, &sample_count);

    if(sf2_samples == NULL || sample_count == 0)
    {
        printf("No samples loaded.\n");
        return 1;
    }

    printf("Loaded %zu samples.\n", sample_count);

    // Iterate through the samples
    for(size_t i = 0; i < sample_count; i++)
    {
        printf("[%zu] Sample rate: %u Hz, length: %u samples\n", i, sf2_samples[i].sample_rate, sf2_samples[i].length);
    }

    // Free the audio data for each sample
    for(size_t i = 0; i < sample_count; i++)
    {
        //printf("Freeing stuff...\n");
        free(sf2_samples[i].audio_data);
    }

    // Free the array of samples
    free(sf2_samples);
    */

    /*
    struct Sample* sf2_samples = NULL;
    size_t sample_count = 0;
    
    ksynth_load_sf2_samples("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Arachno SoundFont Version 1.0.sf2", &sf2_samples, &sample_count);
    
    printf("Loaded %zu samples\n", sample_count);
    
    //for (size_t i = 0; i < sample_count; i++)
    //{
    //    printf("Sample %zu: rate=%u length=%u keys=%u-%u\n", i, sf2_samples[i].sample_rate, sf2_samples[i].length, sf2_samples[i].low_key, sf2_samples[i].hi_key);
    //}
    
    // Don't forget to free the allocated audio_data buffer too
    // The audio_data pointers all point into one big malloc block allocated as sample_data inside the loader.
    // So free sf2_samples[0].audio_data (or samples[0].audio_data) once you're done:
    
    free(sf2_samples[0].audio_data);
    free(sf2_samples);
    */
}

void Test3()
{
    struct Sample samples[MAX_KEYS];

   // for (int note = 0; note < MAX_KEYS; ++note)
   // {
   //     if (load_preset_to_sample("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Keppys Steinway Piano 7.2.sf2", 0, 0, note, 2.0f, &samples[note]))
   //     {
   //         printf("Note %d loaded: %u frames\n", note, samples[note].length);
   //     }
   //     else
   //     {
   //         samples[note].audio_data = NULL;
   //     }
   // }
}

void Test4()
{
    struct Sample* samples = NULL;
    unsigned int sample_count = 0;
    PresetEffects* presets = NULL;
    unsigned int preset_count = 0;
    int16_t** preset_gens = NULL;

    int r = ksynth_sf2_load("C:\\Users\\acer\\Desktop\\BM\\Soundfonts\\Arachno SoundFont Version 1.0.sf2", &samples, &sample_count, &presets, &preset_count, &preset_gens);
    if(r != 0)
    {
        fprintf(stderr, "SF2 load failed: %d\n", r);
        //return 1;
    }

    printf("Loaded %u samples, %u presets\n", sample_count, preset_count);

    // inspect first sample
    for(int i = 0; i < sample_count; i++)
    {
        printf("Sample Index: %d | SR: %u | LEN: %u | KR -> lk: %u - hk: %u | LOOP -> start: %u - end: %u\n", i, samples[i].sample_rate, samples[i].length, i, samples[i].low_key, samples[i].hi_key, i, samples[i].loop_start, samples[i].loop_end);
    }

    // inspect preset 0 basic effects
    for(int i = 0; i < preset_count; i++)
    {
        printf("Preset Index: %d | pan: %d | reverb: %d | chorus: %d | Preset Name: %s\n", i, presets[i].pan, presets[i].reverbSend, presets[i].chorusSend, presets[i].name);
    }

    // free everything
    for(unsigned int i = 0; i < sample_count; ++i)
    {
        if(samples[i].audio_data)
            free(samples[i].audio_data);
    }
    free(samples);

    if(preset_gens)
    {
        for(unsigned int p = 0; p < preset_count; ++p) free(preset_gens[p]);
            free(preset_gens);
    }
    free(presets);

}

void Test5()
{

}


int main(int argc, char** argv) {
#if defined(_WIN32) || defined(_WIN64)
    SetTerminal();
#endif
    printf("hello world!\n");
    Test1();
    //Test2();
    //Test3();
    //Test4();
    //Test5();
    return 0;
}
