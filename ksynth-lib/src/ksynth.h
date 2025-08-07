#ifndef KSYNTH_H
#define KSYNTH_H
#ifdef __cplusplus
extern "C" {
#endif

#define _USE_MATH_DEFINES


#define MAX_KEYS 128
#define MAX_POLYPHONY 100000
#define MIN_BUF 2
#define MAX_BUF 65536

#define RELEASE_TIME 300



#include "utils.h"
#include "core/chan.h"
#include "core/sample.h"
#include "core/voice.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef KSYNTH_MSVC_TESTING
// HEEDOOPY BY BREE
//#include "breewashere.h"
#include <emmintrin.h>
#include <intrin.h>
#else
#include <time.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

struct KSynth {
	struct Sample** samples;
	unsigned int sample_rate;
	unsigned char num_channel;
	unsigned int polyphony;
	unsigned char polyphony_per_channel[16];
	unsigned int max_polyphony;
	float rendering_time;
	struct Chan channels[16];
	struct Voice** voices;
	bool release_oldest_instance_on_note_off;
};

/**
 * @~english
 * @brief Returns the commit count of the KSynth.
 *
 * This function returns the commit count of the KSynth.
 *
 * @return Commit count of the KSynth, returns -1 on fail.
 *
 * @~japanese
 * @brief KSynthのコミット数を返します。
 *
 * この関数はKSynthのコミット数を返します。
 *
 * @return KSynthのコミット数、失敗時には-1
 */
KSYNTH_API int ksynth_get_commit_number(void);

/**
 * @~english
 * @brief Creates a new KSynth instance.
 *
 * This function creates a new KSynth instance with the specified parameters.
 * The `sample_file_path` parameter specifies the path to the sample file which will be loaded and used for synthesis.
 * The `sample_rate` parameter defines the audio sample rate in Hz.
 * The `num_channel` parameter specifies the number of audio channels (1 for mono, 2 for stereo).
 * The `max_polyphony` parameter sets the maximum number of simultaneous notes that can be played.
 * Values exceeding 100,000 will be clamped to 100,000.
 *
 * @param sample_file_path Path to the sample file
 * @param sample_rate Sample rate
 * @param num_channel Number of channels (1-2)
 * @param max_polyphony Maximum polyphony (1-100,000)
 * @param release_oldest_instance_on_note_off If true, only the oldest instance of the note will be released on note_off; otherwise, all instances of the note on the given channel will be released.
 * @return A pointer to the newly created KSynth instance on success, NULL on failure.
 *
 * @~japanese
 * @brief 新しいKSynthインスタンスを作成します。
 *
 * この関数は、指定されたパラメータで新しいKSynthインスタンスを作成します。
 * `sample_file_path`パラメータは、合成に使用されるサンプルファイルへのパスを指定します。
 * `sample_rate`パラメータは、オーディオサンプルレート（Hz）を定義します。
 * `num_channel`パラメータは、オーディオチャンネルの数を指定します（モノラルの場合は1、ステレオの場合は2）。
 * `max_polyphony`パラメータは、同時に再生できる最大ノート数を設定します。
 * 100,000を超える値は100,000にクランプされます。
 *
 * @param sample_file_path サンプルファイルへのパス
 * @param sample_rate サンプルレート
 * @param num_channel チャンネル数（1〜2）
 * @param max_polyphony 最大ポリフォニー（1〜100,000）
 * @param release_oldest_instance_on_note_off ノートオフの際に最も古いインスタンスのみをリリースする場合は true、すべてのインスタンスをリリースする場合は false。
 * @return 成功した場合は新しいKSynthインスタンスへのポインタ、失敗した場合はNULL
 */
KSYNTH_API struct KSynth* ksynth_new(const char* sample_file_path, unsigned int sample_rate, unsigned char num_channel, unsigned int max_polyphony, bool release_oldest_instance_on_note_off);

/**
 * @~english
 * @brief Turns a note on for the specified MIDI channel and note.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param channel MIDI channel
 * @param note Note number
 * @param velocity Velocity
 *
 * @~japanese
 * @brief 指定されたMIDIチャンネルとノートのノートオンを行います。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param channel MIDIチャンネル
 * @param note ノート番号
 * @param velocity ベロシティ
 */
KSYNTH_API void ksynth_note_on(struct KSynth* ksynth_instance, unsigned char channel, unsigned char note, unsigned char velocity);

/**
 * @~english
 * @brief Turns a note off for the specified MIDI channel and note.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param channel MIDI channel
 * @param note Note number
 *
 * @~japanese
 * @brief 指定されたMIDIチャンネルとノートのノートオフを行います。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param channel MIDIチャンネル
 * @param note ノート番号
 */
KSYNTH_API void ksynth_note_off(struct KSynth* ksynth_instance, unsigned char channel, unsigned char note);

/**
 * @~english
 * @brief Turns off all notes.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 *
 * @~japanese
 * @brief すべてのノートをオフにします。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 */
KSYNTH_API void ksynth_note_off_all(struct KSynth* ksynth_instance);

/**
 * @~english
 * @brief Handles MIDI Control Change messages for a synthesizer. (supporting only pan and sustain for now)
 *
 * Updates pan and sustain pedal information based on the given MIDI channel and data.
 *
 * @param ksynth_instance Pointer to the KSynth instance.
 * @param channel MIDI channel (0 to 15).
 * @param param1 is CC ID
 * @param param2 is CC value
 *
 * @~japanese
 * @brief シンセサイザーの MIDI コントロールチェンジメッセージを処理します。(現在はパンとサスティンのみをサポートしています。)
 *
 * 与えられた MIDI チャンネルとデータに基づいて、パンとサスティンペダルの情報を更新します。
 *
 * @param ksynth_instance KSynth インスタンスへのポインタ
 * @param channel MIDI チャンネル（0 から 15）
 * @param param1 コントロールチェンジの ID
 * @param param2 コントロールチェンジの値
 */
KSYNTH_API void ksynth_cc(struct KSynth* ksynth_instance, unsigned char channel, unsigned char param1, unsigned char param2);

/**
 * @~english
 * @brief Gets the current polyphony.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @return The current polyphony
 *
 * @~japanese
 * @brief 現在のポリフォニーを取得します。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @return 現在のポリフォニー
 */
KSYNTH_API unsigned int ksynth_get_polyphony(struct KSynth* ksynth_instance);

/**
 * @~english
 * @brief Gets the maximum polyphony.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @return The maximum polyphony
 *
 * @~japanese
 * @brief 最大ポリフォニーを取得します。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @return 最大ポリフォニー
 */
KSYNTH_API unsigned int ksynth_get_max_polyphony(struct KSynth* ksynth_instance);

/**
 * @~english
 * @brief Sets the maximum polyphony.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param max_polyphony Maximum polyphony (Note: Values exceeding 100,000 will be clamped)
 * @return Returns true on success, false on failure.
 *
 * @~japanese
 * @brief 最大ポリフォニーを設定します。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param max_polyphony 最大ポリフォニー（注：100,000を超える値はクランプされます）
 * @return 成功した場合はtrue、失敗した場合はfalse
 */
KSYNTH_API bool ksynth_set_max_polyphony(struct KSynth* ksynth_instance, unsigned int max_polyphony);

/**
 * @~english
 * @brief Retrieves the setting for releasing only the oldest note instance on note off.
 *
 * This function checks the KSynth instance setting for handling note-off events.
 * It returns whether only the oldest instance of a note is released when a note-off event occurs.
 * If the setting is `true`, only the oldest instance of the note will be released.
 * If `false`, all instances of the note on the specified channel are released.
 *
 * @param ksynth_instance Pointer to the KSynth instance.
 * @return `true` if only the oldest note instance is released on note off, `false` if all instances are released.
 *
 * @~japanese
 * @brief ノートオフ時に最も古いノートインスタンスのみをリリースする設定を取得します。
 *
 * この関数は、KSynthインスタンスにおけるノートオフ時の設定を確認します。
 * ノートオフイベントが発生した際に、最も古いノートインスタンスのみがリリースされるかどうかを返します。
 * この設定が`true`の場合、最も古いインスタンスのみがリリースされます。
 * `false`の場合、指定されたチャンネルのすべてのインスタンスがリリースされます。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ。
 * @return 最も古いノートインスタンスのみがリリースされる場合は`true`、すべてのインスタンスがリリースされる場合は`false`を返します。
 */
KSYNTH_API bool ksynth_get_release_oldest_instance_on_note_off(struct KSynth* ksynth_instance);

/**
 * @~english
 * @brief Sets whether to release only the oldest note instance on note off.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param release_oldest_instance_on_note_off If true, only the oldest instance of the note is released on note off. If false, all instances of the note on the specified channel are released.
 *
 * @~japanese
 * @brief ノートオフ時に最も古いノートインスタンスのみをリリースするかどうかを設定します。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param release_oldest_instance_on_note_off trueの場合、ノートオフ時に指定されたチャンネルのノートの最も古いインスタンスのみがリリースされます。falseの場合、指定されたチャンネルのノートのすべてのインスタンスがリリースされます。
 */
KSYNTH_API void ksynth_set_release_oldest_instance_on_note_off(struct KSynth* ksynth_instance, bool release_oldest_instance_on_note_off);

/**
 * @~english
 * @brief Generates a buffer with the specified size.
 *
 * This function generates audio data for a buffer of a given size.
 * The size of the buffer determines the duration of audio that will be generated.
 * Smaller buffer sizes allow for more granularity in the audio, making strummed notes
 * sound better.
 * Larger buffer sizes typically result in more audio data being generated per call,
 * potentially reducing the frequency of function calls to generate audio.
 * However, extremely large buffer sizes may increase memory usage and latency, and make
 * the audio sound choppy as a result.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param buffer Pointer to the buffer you want to fill up with data
 * @param buffer_size Size of the buffer
 *
 * @~japanese
 * @brief 指定されたサイズのバッファを生成します。
 *
 * この関数は、指定されたサイズのバッファーのオーディオ データを生成します。
 * バッファのサイズによって、生成されるオーディオの長さが決まります。
 * バッファ サイズを小さくすると、オーディオの粒度が向上し、かき鳴らされたノートの音が良くなります。
 * 通常、バッファ サイズが大きいと、呼び出しごとに生成される音声データが多くなり、音声を生成するための関数呼び出しの頻度が減少する可能性があります。
 * ただし、バッファ サイズが非常に大きいと、メモリ使用量と遅延が増加し、結果として音声が途切れ途切れになる可能性があります。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param buffer データで埋めたいバッファへのポインタ
 * @param buffer_size バッファのサイズ
 */
KSYNTH_API void ksynth_fill_buffer(struct KSynth* ksynth_instance, float* buffer, unsigned int buffer_size);

/**
 * @~english
 * @brief Generates a buffer with the specified size.
 *
 * This function generates audio data for a buffer of a given size.
 * It internally points to ksynth_fill_buffer, but instead of having the user manage the buffer,
 * this function prepares it for them and gives them a pointer as a return value.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param buffer_size Size of the buffer (Note: Values are typically between 32 and 16384)
 * @return A pointer to the generated buffer on success, NULL on failure.
 *
 * @~japanese
 * @brief 指定されたサイズのバッファを生成します。
 *
 * この関数は、指定されたサイズのバッファーのオーディオ データを生成します。
 * この関数は内部的に ksynth_fill_buffer を指しますが、ユーザーにバッファーを管理させる代わりに、この関数はユーザーのためにバッファーを準備し、戻り値としてポインターを与えます。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param buffer_size バッファのサイズ（注：通常は32から16384の値です）
 * @return 成功した場合は生成されたバッファへのポインタ、失敗した場合はNULL
 */
KSYNTH_API float* ksynth_generate_buffer(struct KSynth* ksynth_instance, unsigned int buffer_size);

/**
 * @~english
 * @brief Gets the rendering time.
 *
 * The rendering time represents the time taken by the synthesis engine to generate audio data for the buffer.
 * When the rendering time reaches 100%, it indicates that the synthesis process is taking as long as the buffer duration,
 * and any longer processing time may result in buffer underrun, leading to audio glitches or gaps.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @return Rendering time as a percentage of the buffer duration
 *
 * @~japanese
 * @brief レンダリング時間を取得します。
 *
 * レンダリング時間は、合成エンジンがバッファのためのオーディオデータを生成するのにかかる時間を表します。
 * レンダリング時間が100%に達すると、合成プロセスがバッファの期間と同じくらいの時間を取ることを示し、
 * より長い処理時間はバッファのアンダーランを引き起こし、オーディオのグリッチやギャップを引き起こす可能性があります。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @return バッファの期間の割合としてのレンダリング時間
 */
KSYNTH_API float ksynth_get_rendering_time(struct KSynth* ksynth_instance);

/**
 * @~english
 * @brief Gets the number of active polyphony for the specified MIDI channel.
 *
 * @param ksynth_instance Pointer to the KSynth instance
 * @param channel MIDI channel
 * @return Number of active polyphony for the specified MIDI channel
 *
 * @~japanese
 * @brief 指定されたMIDIチャンネルのアクティブなポリフォニーの数を取得します。
 *
 * @param ksynth_instance KSynthインスタンスへのポインタ
 * @param channel MIDIチャンネル
 * @return 指定されたMIDIチャンネルのアクティブなポリフォニーの数
 */
KSYNTH_API unsigned int ksynth_get_polyphony_for_channel(struct KSynth* ksynth_instance, unsigned char channel);

/**
 * @~english
 * @brief Frees the buffer.
 *
 * @param buffer Pointer to the buffer to be freed
 *
 * @~japanese
 * @brief バッファを解放します。
 *
 * @param buffer 解放するバッファへのポインタ
 */
KSYNTH_API void ksynth_buffer_free(float* buffer);

/**
 * @~english
 * @brief Frees the KSynth instance.
 *
 * @param ksynth_instance Pointer to the KSynth instance to be freed
 *
 * @~japanese
 * @brief KSynthインスタンスを解放します。
 *
 * @param ksynth_instance 解放するKSynthインスタンスへのポインタ
 */
KSYNTH_API void ksynth_free(struct KSynth* ksynth_instance);

#ifdef __cplusplus
}
#endif
#endif