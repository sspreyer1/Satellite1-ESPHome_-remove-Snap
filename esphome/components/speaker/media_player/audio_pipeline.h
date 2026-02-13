#pragma once

#ifdef USE_ESP32

#include "esphome/components/audio/audio.h"
#include "esphome/components/audio/audio_reader.h"
#include "esphome/components/audio/audio_decoder.h"
#include "esphome/components/audio/chunked_ring_buffer.h"
#include "esphome/components/speaker/speaker.h"

#include "esp_err.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>

namespace esphome {
namespace speaker {

// Internal sink/source buffers for reader and decoder
static const size_t DEFAULT_TRANSFER_BUFFER_SIZE = 24 * 1024;

enum class AudioPipelineType : uint8_t {
  MEDIA,
  ANNOUNCEMENT,
};

enum class AudioPipelineState : uint8_t {
  STARTING_FILE,
  STARTING_URL,
  PLAYING,
  STOPPING,
  STOPPED,
  PAUSED,
  ERROR_READING,
  ERROR_DECODING,
};

enum class InfoErrorSource : uint8_t {
  READER = 0,
  DECODER,
};

enum class DecodingError : uint8_t {
  FAILED_HEADER = 0,
  INCOMPATIBLE_BITS_PER_SAMPLE,
  INCOMPATIBLE_CHANNELS,
};

// Used to pass information from each task.
struct InfoErrorEvent {
  InfoErrorSource source;
  optional<esp_err_t> err;
  optional<audio::AudioFileType> file_type;
  optional<audio::AudioStreamInfo> audio_stream_info;
  optional<DecodingError> decoding_err;
};

class AudioPipeline {
 public:
  AudioPipeline(speaker::Speaker *speaker, size_t buffer_size, bool task_stack_in_psram, std::string base_name,
                UBaseType_t priority);

  void start_url(const std::string &uri);
  void start_file(audio::AudioFile *audio_file);

  esp_err_t stop();
  AudioPipelineState process_state();

  void suspend_tasks();
  void resume_tasks();

  uint32_t get_playback_ms() { return this->playback_ms_; }
  void set_pause_state(bool pause_state);

 protected:
  esp_err_t allocate_communications_();
  esp_err_t start_tasks_();
  void delete_tasks_();

  std::string base_name_;
  UBaseType_t priority_;
  uint32_t playback_ms_{0};
  bool hard_stop_{false};
  bool is_playing_{false};
  bool pause_state_{false};
  bool task_stack_in_psram_;

  bool pending_url_{false};
  bool pending_file_{false};

  speaker::Speaker *speaker_{nullptr};

  std::string current_uri_{};
  audio::AudioFile *current_audio_file_{nullptr};

  audio::AudioFileType current_audio_file_type_;
  audio::AudioStreamInfo current_audio_stream_info_;

  size_t buffer_size_;
  size_t transfer_buffer_size_;

  std::shared_ptr<audio::TimedRingBuffer> reader_output_rb_;

  EventGroupHandle_t event_group_{nullptr};
  QueueHandle_t info_error_queue_{nullptr};

  static void read_task(void *params);
  TaskHandle_t read_task_handle_{nullptr};
  StaticTask_t read_task_stack_;
  StackType_t *read_task_stack_buffer_{nullptr};

  static void decode_task(void *params);
  TaskHandle_t decode_task_handle_{nullptr};
  StaticTask_t decode_task_stack_;
  StackType_t *decode_task_stack_buffer_{nullptr};
};

}  // namespace speaker
}  // namespace esphome

#endif
