#include <time.h>



#include "ksynth.h"
#include "utils.h"



#if defined(_MSC_VER) && !defined(__clang__)
float compute_velocity_sse2(float logVel, float pow, float coeff)
{
	__m128 fresult = _mm_add_ps(_mm_pow_ps(_mm_set1_ps(logVel), _mm_set1_ps(pow)), _mm_set1_ps(coeff));

	float result;
	_mm_store_ss(&result, _mm_max_ps(_mm_min_ps(fresult, _mm_set1_ps(1.0f)), _mm_setzero_ps()));

	return result;
}
#endif

void int_free_voices(struct Voice** voices, unsigned long count)
{
	if(voices)
  {
		if(count > 0)
    {
			for(unsigned long i = 0; i < count; i++)
      {
				if(voices[i]) 
        {
					free(voices[i]);
				}
			}
		}

		free(voices);
	}
}

struct Voice** int_allocate_voices(struct KSynth* ksynth_instance)
{
	if(ksynth_instance->max_polyphony < 1)
    ksynth_instance->max_polyphony = 1;
	else if(ksynth_instance->max_polyphony >= MAX_POLYPHONY)
		ksynth_instance->max_polyphony = MAX_POLYPHONY;

	struct Voice** voices = calloc(ksynth_instance->max_polyphony, sizeof(struct Voice*));

	if(voices)
  {
		for(unsigned long i = 0; i < ksynth_instance->max_polyphony; i++)
    {
			struct Voice* newVoice = calloc(1, sizeof(struct Voice));

			if(newVoice)
      {
				newVoice->killed = 1;
				voices[i] = newVoice;

				continue;
			}

			if(i > 0)
      {
				for(unsigned long j = 0; j < i; j++)
        {
					free(voices[i]);
				}
			}

			return NULL;
		}

		return voices;
	}

	return NULL;
}

void int_free_samples(struct Sample** samples, unsigned long count)
{
	if(samples)
  {
		if(count > 0)
    {
			for(unsigned long i = 0; i < count; i++)
      {
				if(samples[i])
        {
					if(samples[i]->audio_data)
            free(samples[i]->audio_data);

					free(samples[i]);
				}
			}
		}

		free(samples);
	}
}

struct Sample** int_allocate_samples(const char* path, unsigned char keys, struct KSynth* ksynth_instance)
{
	int sfrate = ksynth_instance->sample_rate;
	keys = keys > MAX_KEYS ? MAX_KEYS : keys;

	struct Sample** samples = calloc(ksynth_instance->sample_rate, sizeof(struct Sample*));
	if(samples != NULL)
  {
		// Load samples from file
		FILE* f = fopen(path, "rb");
		if(f == NULL)
    {
			fprintf(stderr, "[KSynth] Error: Failed to open sample file.\n");
			free(samples);
			return NULL;
		}

		for(int i = 0; i < keys; i++)
    {
			samples[i] = calloc(1, sizeof(struct Sample));

			if(!samples[i])
      {
				int_free_samples(samples, i);

				fprintf(stderr, "[KSynth] Error: Failed to allocate memory for sample struct.\n");
				return NULL;
			}

			struct Sample* sample = samples[i];

			sample->sample_rate = sfrate;
			sample->length = sfrate * 10;
			sample->audio_data = calloc(sfrate * 10, sizeof(int16_t));

			if(!sample->audio_data)
      {
				int_free_samples(samples, i);

				fprintf(stderr, "[KSynth] Error: Failed to allocate memory for samples data.\n");
				return NULL;
			}

			// Load samples from file end
			fread(samples[i]->audio_data, sfrate * 10 * sizeof(int16_t), 1, f);
		}
		fclose(f);

		return samples;
	}

	fprintf(stderr, "[KSynth] Error: Failed to open samples file.\n");
	return NULL;
}

int ksynth_get_commit_number(void)
{
	const char* commit_number_str = "CommitNumber: COMMIT_NUMBER_PLACEHOLDER";
	int n = 0;
	bool found = false;

	for(int i = 0; commit_number_str[i] != 0; i++)
  {
		if(commit_number_str[i] >= '0' && commit_number_str[i] <= '9')
    {
			found = true;
			n *= 10;
			n += commit_number_str[i] - '0';
		}
    else if(found)
    {
			/* non-number char after number */
			break;
		}
	}
	if(!found) return -1;
	return n;
}

struct KSynth* ksynth_new(const char* sample_file_path, unsigned int sample_rate, unsigned char num_channel, unsigned int max_polyphony, bool release_oldest_instance_on_note_off) {
	struct KSynth* ksynth_instance = calloc(1, sizeof(struct KSynth));
	if(ksynth_instance != NULL)
  {
		// HARDCODED?
		ksynth_instance->rendering_time = 0.0f;
		ksynth_instance->sample_rate = sample_rate;
		ksynth_instance->num_channel = num_channel >= 2 ? 2 : num_channel;
		ksynth_instance->polyphony = 0;
		ksynth_instance->max_polyphony = max_polyphony;
		ksynth_instance->release_oldest_instance_on_note_off = release_oldest_instance_on_note_off;

		ksynth_instance->voices = int_allocate_voices(ksynth_instance);
		if(ksynth_instance->voices)
    {
			ksynth_instance->samples = int_allocate_samples(sample_file_path, MAX_KEYS, ksynth_instance);

			if(ksynth_instance->samples)
      {
				memset(ksynth_instance->channels, 0, 16 * sizeof(struct Chan));
				memset(ksynth_instance->polyphony_per_channel, 0, 16 * sizeof(unsigned char));

				return ksynth_instance;
			}
      else
      {
				fprintf(stderr, "[KSynth] Error: Failed to allocate memory for samples.\n");
				return NULL;
			}
		}
    else
    {
			fprintf(stderr, "[KSynth] Error: Failed to allocate memory for voices.\n");
			return NULL;
		}
	}

	fprintf(stderr, "[KSynth] Error: Failed to allocate memory for KSynth instance.\n");
	return NULL;
}

void ksynth_note_on(struct KSynth* ksynth_instance, unsigned char channel, unsigned char note, unsigned char velocity)
{
  printf("Noteon\n");
	if(!ksynth_instance || !ksynth_instance->voices || channel > 15 || note > (MAX_KEYS - 1) || velocity > 127)
  {
		fprintf(stderr, "[KSynth] Error: Invalid parameters for note on.\n");
		return;
	}

	if(velocity < 1) ksynth_note_off(ksynth_instance, channel, note);

	if(channel == 9) return;

	struct Voice* voice = 0;

	if(ksynth_instance->polyphony >= ksynth_instance->max_polyphony)
  {
		unsigned char min = ksynth_instance->voices[0]->velocity;
		unsigned long index = 0;

		for(unsigned long i = 0; i < ksynth_instance->polyphony - 1; ++i)
    {
			voice = ksynth_instance->voices[i];

			if(voice->velocity < min)
      {
				min = voice->velocity;
				index = i;
			}
		}

		voice = ksynth_instance->voices[index];

		for(unsigned long i = index; i < ksynth_instance->polyphony - 1; ++i)
    {
			ksynth_instance->voices[i] = ksynth_instance->voices[i + 1];
		}

		ksynth_instance->voices[ksynth_instance->max_polyphony - 1] = voice;
	}
  else
  {
		for(unsigned long i = 0; i < ksynth_instance->max_polyphony; ++i)
    {
			struct Voice* temp = ksynth_instance->voices[i];

			if(temp->killed)
      {
				voice = temp;
				break;
			}
		}

		ksynth_instance->polyphony++;
		ksynth_instance->polyphony_per_channel[channel]++;
	}

	if(voice)
  {
		voice_reset(voice);

		voice->killed = 0;
		voice->channel = channel;
		voice->noteNumber = note;
		voice->velocity = velocity;
	}
}

void ksynth_note_off(struct KSynth* ksynth_instance, unsigned char channel, unsigned char note)
{
  printf("Noteoff\n");
	if(!ksynth_instance || channel > 15 || note > (MAX_KEYS - 1))
  {
		fprintf(stderr, "[KSynth] Error: Invalid parameters for note off.\n");
		return;
	}

	if(channel == 9) return;

	struct Voice* voice = 0;

	for(unsigned long i = ksynth_instance->polyphony - 1; i < ksynth_instance->polyphony; --i)
  {
		voice = ksynth_instance->voices[i];

		if((voice->channel == channel && note == voice->noteNumber) && (!voice->killed && !voice->tokill))
    {
			if(ksynth_instance->release_oldest_instance_on_note_off)
      {
				voice->tokill = 1;
				break;
			}
      else
      {
				voice->tokill = 1;
			}
		}
	}
}

void ksynth_note_off_all(struct KSynth* ksynth_instance)
{
	if(ksynth_instance != NULL)
  {
		for(unsigned long i = 0; i < ksynth_instance->max_polyphony; ++i)
    {
			struct Voice* voice = ksynth_instance->voices[i];
			voice_reset(voice);
		}

		for(unsigned short i = 0; i < 15; i++)
    {
			ksynth_instance->channels[i].pan = 0.0f;
			ksynth_instance->channels[i].sustain = 0;
			ksynth_instance->polyphony_per_channel[i] = 0;
		}

		ksynth_instance->polyphony = 0;
	}
}

void ksynth_cc(struct KSynth* ksynth_instance, unsigned char channel, unsigned char param1, unsigned char param2) {
	if(!ksynth_instance || channel > 15)
  {
		fprintf(stderr, "[KSynth] Error: Invalid parameters for CC.\n");
		return;
	}

	if(channel == 9) return;

	switch(param1)
  {
	  case 10:
	  	ksynth_instance->channels[channel].pan = ((param2 - 64) / 64.0f);
	  	break;
	  case 64:
	  	// Damper pedal, MUST respond to this at least
	  	ksynth_instance->channels[channel].sustain = param2 > 63 ? 1 : 0;
	  	break;
	  default:
	  	break;
	}
}

unsigned int ksynth_get_polyphony(struct KSynth* ksynth_instance)
{
	if(!ksynth_instance)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance.\n");
		return -1;
	}

	return ksynth_instance->polyphony;
};

unsigned int ksynth_get_max_polyphony(struct KSynth* ksynth_instance)
{
	if(!ksynth_instance)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance.\n");
		return -1;
	}

	return ksynth_instance->max_polyphony;
};

bool ksynth_set_max_polyphony(struct KSynth* ksynth_instance, unsigned int max_polyphony)
{
	if(!ksynth_instance) return false;

	struct Voice** old_ptr = ksynth_instance->voices;
	unsigned long old_count = ksynth_instance->max_polyphony;

	ksynth_instance->max_polyphony = max_polyphony;
	struct Voice** new_ptr = int_allocate_voices(ksynth_instance);
	if(new_ptr != NULL)
  {
		ksynth_instance->voices = new_ptr;

		ksynth_note_off_all(ksynth_instance);

		int_free_voices(old_ptr, old_count);

		return true;
	}
	ksynth_instance->max_polyphony = old_count;

	fprintf(stderr, "[KSynth] Error: Failed to allocate memory for voices.\n");
	return false;
}

bool ksynth_get_release_oldest_instance_on_note_off(struct KSynth* ksynth_instance)
{
	if(!ksynth_instance)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance.\n");
		return NULL;
	}

	return ksynth_instance->release_oldest_instance_on_note_off;
}

void ksynth_set_release_oldest_instance_on_note_off(struct KSynth* ksynth_instance, bool release_oldest_instance_on_note_off)
{
	if(!ksynth_instance)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance.\n");
		return;
	}

	ksynth_instance->release_oldest_instance_on_note_off = release_oldest_instance_on_note_off;
}

void ksynth_fill_buffer(struct KSynth* ksynth_instance, float* buffer, unsigned int buffer_size)
{
	// I decided not to limit the buffer size in ksynth_fill_buffer,
	// to make use of the new chunk system in OMv2, better strums:tm:!
	if(ksynth_instance == NULL || ksynth_instance->samples == NULL)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance or no samples loaded.\n");
		return;
	}

	if(!buffer)
  {
		fprintf(stderr, "[KSynth] Error: Target buffer is not valid.\n");
		return;
	}

	// post_decay is RELEASE_TIME of the MIDI stream, which you can take by doing (sample_rate / 1000) * RELEASE_TIME
	unsigned int post_decay = (ksynth_instance->sample_rate / 1000) * RELEASE_TIME;
	int num_channels = ksynth_instance->num_channel;

	float velocity = 1.0f;

	static struct timespec rendering_time_start, rendering_time_end;
	//clock_gettime(CLOCK_MONOTONIC, &rendering_time_start);
  get_time(&rendering_time_start);

	if(ksynth_instance->polyphony)
  {
		// Initialize buffer with zeros
		memset(buffer, 0, sizeof(float) * buffer_size);

		for(unsigned long i = 0; i < ksynth_instance->max_polyphony; i++)
    {
			struct Voice* voice = ksynth_instance->voices[i];

			if(!voice->killed)
      {
				// Voice is alive, let's store the current position and the length of the sample
				// for future reference
				unsigned int sample_length = ksynth_instance->samples[voice->noteNumber]->length;

				// CC 10 support
				float pan = ((ksynth_instance->channels[voice->channel].pan + 1.0f) / 2.0f) * (M_PI / 2.0f);
				float lpan = sinf(pan);
				float rpan = cosf(pan);

				float decay_time = post_decay * (voice->noteNumber > 64 ? ((64 - (voice->noteNumber - 64)) / 128.0f) : 1.0f);
				float nVel = 1.0f;
				float logVel = (float)voice->velocity / 127.0f;

#if defined(_MSC_VER) && !defined(__clang__)
				velocity = compute_velocity_sse2(logVel, 2.5f, 0.03f);
#else
				velocity = fminf(powf(logVel, 2.5) + 0.03, 1.0);
#endif

				for(unsigned int j = 0; j < buffer_size / num_channels; ++j)
        {
					// This is used to calculate the natural decay of the note,
					// which happens n samples before sample_length!! VERY IMPORTANT!
					bool natural_decay = sample_length - voice->sample_position < decay_time;
					bool sample_done = voice->sample_position >= sample_length;
					bool fadeout_done = voice->curfalloff >= decay_time;

					// If we ran out of samples to play, or the fade out limit from voice->tokill
					// has been reached, the voice is done and doesn't need to be played anymore.
					if(sample_done || fadeout_done)
          {
						voice->killed = 1;
						break;
					}

					// Get the sample byte you want to play
					float sample = ksynth_instance->samples[voice->noteNumber]->audio_data[voice->sample_position] / 32768.0f;

					// Check if the voice hasn't been killed yet, if it has
					// been marked as tokill, or if it is about to reach the end of the sample (natural_decay)
					if(!voice->killed && (voice->tokill || natural_decay))
          {
						// If the voice isn't sustained using CC 64 and it's time to kill it, or if we reached the natural_decay limit,
						// start the curve falloff
						if((!ksynth_instance->channels[voice->channel].sustain && voice->tokill) || natural_decay)
              voice->curfalloff++;

						// As long as curfalloff is minor than decay_time (the amount of samples it takes for the
						// sample to reach natural decay, which is "sample_length - (sample_rate / 4)"), keep
						// calculating the velocity curve, else set velocity to 0.0f.
						nVel = voice->curfalloff < decay_time ? (((float)decay_time - voice->curfalloff) / decay_time) : 0.0f;
					}

					for(int c = 0; c < num_channels; ++c)
          {
						// Fill up the buffer.
						buffer[j * num_channels + c] += sample * (velocity * nVel) * (num_channels < 2 ? 1.0f : c ? lpan : rpan);
					}

					// Move forward in sample by one.
					voice->sample_position++;
				}

				// If voice is finally done being played, and hasn't been killed, store its position
				if(voice->killed)
        {
					// Otherwise, commit homicide and reset it
					ksynth_instance->polyphony--;
					ksynth_instance->polyphony_per_channel[voice->channel]--;

					voice_reset(voice);
					continue;
				}
			}
		}
	}
  else
  {
		for(int i = 0; i < buffer_size; i++)
    {
			buffer[i] = 0.0f;
		}
	}

	//clock_gettime(CLOCK_MONOTONIC, &rendering_time_end);
  get_time(&rendering_time_end);

	float buffer_generate_elapsed_time_ns = (rendering_time_end.tv_sec - rendering_time_start.tv_sec) * 1e9 + (rendering_time_end.tv_nsec - rendering_time_start.tv_nsec);
	float buffer_generate_elapsed_time_ms = buffer_generate_elapsed_time_ns / 1e6;

	float rendering_time = (float)buffer_generate_elapsed_time_ms / buffer_size;

	ksynth_instance->rendering_time = rendering_time * 100;
}

float* ksynth_generate_buffer(struct KSynth* ksynth_instance, unsigned int buffer_size)
{
	// buffer still limited here for compatibility purposes
	if(!ksynth_instance || !ksynth_instance->samples)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance or no samples loaded.\n");
		return NULL;
	}

	unsigned int num_channels = ksynth_instance->num_channel;
	unsigned int min_buffer_size = MIN_BUF * num_channels;

	printf("[KSynth] Debug: buffer_size = %u\n", buffer_size);

	if(buffer_size < min_buffer_size)
  {
		fprintf(stderr, "[KSynth] Warning: buffer_size is less than %u! Returning empty buffer...\n", min_buffer_size);

		float* empty_buffer = malloc(min_buffer_size * sizeof(float));
		if(empty_buffer)
    {
			memset(empty_buffer, 0, min_buffer_size * sizeof(float));
			return empty_buffer;
		}
    else
    {
			fprintf(stderr, "[KSynth] Error: Failed to create empty buffer!\n");
			return NULL;
		}
	}
  else if(buffer_size > MAX_BUF)
  {
		fprintf(stderr, "[KSynth] Warning: buffer_size is greater than %u! Returning empty buffer...\n", MAX_BUF);

		float* empty_buffer = malloc(MAX_BUF * sizeof(float));
		if(empty_buffer)
    {
			memset(empty_buffer, 0, MAX_BUF * sizeof(float));
			return empty_buffer;
		}
    else
    {
			fprintf(stderr, "[KSynth] Error: Failed to create empty buffer!\n");
			return NULL;
		}
	}

	float* buffer = malloc(buffer_size * sizeof(float));

	if(buffer)
  {
		ksynth_fill_buffer(ksynth_instance, buffer, buffer_size);
		return buffer;
	}

	fprintf(stderr, "[KSynth] Error: Failed to create buffer!\n");
	return NULL;
}

float ksynth_get_rendering_time(struct KSynth* ksynth_instance)
{
	if(!ksynth_instance)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance.\n");
		return -1.0f;
	}

	return ksynth_instance->rendering_time;
}

unsigned int ksynth_get_polyphony_for_channel(struct KSynth* ksynth_instance, unsigned char channel)
{
	if(!ksynth_instance)
  {
		fprintf(stderr, "[KSynth] Error: Invalid KSynth instance.\n");
		return -1;
	}

	if(channel < 0 || channel > 15)
  {
		fprintf(stderr, "[KSynth] Error: Invalid channel number (%d). Channel number must be between 0 and 15.\n", channel);
		return -1;
	}

	return ksynth_instance->polyphony_per_channel[channel];
}

void ksynth_buffer_free(float* buffer)
{
	if(buffer != NULL)
  {
		free(buffer);
	}
}

void ksynth_free(struct KSynth* ksynth_instance)
{
	if(ksynth_instance != NULL)
  {
		int_free_samples(ksynth_instance->samples, MAX_KEYS);
		int_free_voices(ksynth_instance->voices, ksynth_instance->max_polyphony);
		free(ksynth_instance);
	}
}