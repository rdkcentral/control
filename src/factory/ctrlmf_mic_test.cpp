#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <xraudio.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>
#include <ctrlmf_ws.h>
#include <ctrlmf_audio_capture.h>

#define WAVE_HEADER_SIZE_MIN (44)

#define SNR_MIN ( 30.0)
#define SNR_MAX (120.0)
#define SNR_VAR ( 10.0)

#define FILENAME_WAV_NOISE  "/opt/logs/mic_test_noise.wav"
#define FILENAME_WAV_SIGNAL "/opt/logs/mic_test_signal.wav"

#ifndef CTRLMF_CUSTOM_AUDIO_ANALYSIS
static bool ctrlmf_mic_test_audio_analyze(const char *output_filename, uint32_t level, ctrlmf_audio_frame_t audio_frames_noise, ctrlmf_audio_frame_t audio_frames_signal, uint32_t frame_qty, uint32_t mic_qty, double snr_min, double snr_max, double snr_var, ctrlmf_test_result_t *test_result);
#endif

#if defined(CTRLMF_THUNDER) && defined(CTRLMF_AUDIO_PLAYBACK)
static bool ctrlmf_mic_test_via_audio_file(uint32_t duration, const char *output_filename, uint32_t level, const char *audio_filename, double snr_min, double snr_max, double snr_var, ctrlmf_test_result_t *test_result);
#endif

static bool ctrlmf_mic_test_via_ambient(uint32_t duration, const char *output_filename, uint32_t level, double snr_min, double snr_max, double snr_var, ctrlmf_test_result_t *test_result);
static void ctrlmf_mic_test_audio_export(const char *filename, ctrlmf_audio_frame_t audio_frames, uint32_t frame_qty, uint32_t channel_qty);
static void ctrlmf_wave_header_gen(uint8_t *header, uint16_t audio_format, uint16_t num_channels, uint32_t sample_rate, uint16_t bits_per_sample, uint32_t pcm_data_size);

bool ctrlmf_mic_test_factory(uint32_t duration, const char *output_filename, uint32_t level, const char *audio_filename, double *snr_min, double *snr_max, double *snr_var, ctrlmf_test_result_t *test_result) {
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }
   double snr_min_val = (snr_min != NULL) ? *snr_min : SNR_MIN;
   double snr_max_val = (snr_max != NULL) ? *snr_max : SNR_MAX;
   double snr_var_val = (snr_var != NULL) ? *snr_var : SNR_VAR;
   
   if(audio_filename != NULL) {
      #if defined(CTRLMF_THUNDER) && defined(CTRLMF_AUDIO_PLAYBACK)
      return(ctrlmf_mic_test_via_audio_file(duration, output_filename, level, audio_filename, snr_min_val, snr_max_val, snr_var_val, test_result));
      #else
      XLOGD_ERROR("audio playback is disabled");
      return(false);
      #endif
   }
   return(ctrlmf_mic_test_via_ambient(duration, output_filename, level, snr_min_val, snr_max_val, snr_var_val, test_result));
}

#if defined(CTRLMF_THUNDER) && defined(CTRLMF_AUDIO_PLAYBACK)
bool ctrlmf_mic_test_via_audio_file(uint32_t duration, const char *output_filename, uint32_t level, const char *audio_filename, double snr_min, double snr_max, double snr_var, ctrlmf_test_result_t *test_result) {
   ctrlmf_audio_frame_t audio_frames_noise  = NULL;
   ctrlmf_audio_frame_t audio_frames_signal = NULL;
   bool result = false;
   bool audio_capture_initialized = false;

   // Duration in milliseconds must be a multiple of frame duration (20 ms)
   uint32_t audio_frame_qty = (duration + (AUDIO_FRAME_DURATION - 1)) / AUDIO_FRAME_DURATION;

   do {
      uint32_t audio_frame_size  = MIC_CHANNEL_QTY * AUDIO_FRAME_SIZE_PER_BEAM;
      uint32_t audio_buffer_size = audio_frame_size * audio_frame_qty;

      // Create buffers to store the audio
      audio_frames_noise = (ctrlmf_audio_frame_t)malloc(audio_buffer_size);
      if(audio_frames_noise == NULL) {
         XLOGD_ERROR("out of memory");
         break;
      }

      audio_frames_signal = (ctrlmf_audio_frame_t)malloc(audio_buffer_size);
      if(audio_frames_signal == NULL) {
         XLOGD_ERROR("out of memory");
         break;
      }

      memset(audio_frames_noise,  0, audio_buffer_size);
      memset(audio_frames_signal, 0, audio_buffer_size);

      #ifdef CTRLMF_LOCAL_MIC_TAP
      if(!ctrlmf_audio_capture_init(audio_frame_size, true)) {
      #else
      if(!ctrlmf_audio_capture_init(audio_frame_size, false)) {
      #endif
         XLOGD_ERROR("unable to init audio capture");
         break;
      }
      audio_capture_initialized = true;

      #ifdef CTRLMF_LOCAL_MIC_TAP
      const char *request_type = (MIC_RAW_AUDIO) ? "mic_factory_test" : (MIC_CHANNEL_QTY == 1) ? "mic_tap_stream_single" : "mic_tap_stream_multi";
      #else
      const char *request_type = (MIC_RAW_AUDIO) ? "mic_factory_test" : (MIC_CHANNEL_QTY == 1) ? "mic_stream_single" : "mic_stream_multi";
      #endif

      // Capture noise
      if(!ctrlmf_audio_capture_start(request_type, audio_frames_noise, audio_frame_qty, duration)) {
         XLOGD_ERROR("unable to capture noise");
         break;
      }
   
      // Play back audio
      if(!ctrlmf_audio_playback_start(audio_filename)) {
         XLOGD_ERROR("unable to playback audio");
         break;
      }
      usleep(500 * 1000);
   
      // Capture signal
      if(!ctrlmf_audio_capture_start(request_type, audio_frames_signal, audio_frame_qty, duration)) {
         XLOGD_ERROR("unable to capture signal");
         break;
      }

      XLOGD_INFO("analyze audio capture");
      #ifndef CTRLMF_CUSTOM_AUDIO_ANALYSIS
      if(!ctrlmf_mic_test_audio_analyze(output_filename, level, audio_frames_noise, audio_frames_signal, audio_frame_qty, MIC_CHANNEL_QTY, snr_min, snr_max, snr_var, test_result)) {
      #else
      if(!ctrlmf_mic_test_audio_analyze(CTRLMF_TEST_FACTORY, output_filename, level, audio_frames_noise, audio_frames_signal, audio_frame_qty, MIC_CHANNEL_QTY, test_result)) {
      #endif
         XLOGD_ERROR("unable to analyze audio");
         break;
      }
      if(!ctrlmf_is_production()) {
         ctrlmf_mic_test_audio_export(FILENAME_WAV_NOISE,  audio_frames_noise,  audio_frame_qty, MIC_CHANNEL_QTY);
         ctrlmf_mic_test_audio_export(FILENAME_WAV_SIGNAL, audio_frames_signal, audio_frame_qty, MIC_CHANNEL_QTY);
      }
      result = true;
   } while(0);
   
   if(audio_capture_initialized && !ctrlmf_audio_capture_term()) {
      XLOGD_ERROR("unable to term audio capture");
   }
   if(audio_frames_noise != NULL) {
      free(audio_frames_noise);
   }
   if(audio_frames_signal != NULL) {
      free(audio_frames_signal);
   }
   
   return(result);
}
#endif

bool ctrlmf_mic_test_via_ambient(uint32_t duration, const char *output_filename, uint32_t level, double snr_min, double snr_max, double snr_var, ctrlmf_test_result_t *test_result) {
   ctrlmf_audio_frame_t audio_frames_signal = NULL;
   bool result = false;
   bool audio_capture_initialized = false;

   // Duration in milliseconds must be a multiple of frame duration (20 ms)
   uint32_t audio_frame_qty = (duration + (AUDIO_FRAME_DURATION - 1)) / AUDIO_FRAME_DURATION;

   do {
      uint32_t audio_frame_size  = MIC_CHANNEL_QTY * AUDIO_FRAME_SIZE_PER_BEAM;
      uint32_t audio_buffer_size = audio_frame_size * audio_frame_qty;

      // Create buffers to store the audio
      audio_frames_signal = (ctrlmf_audio_frame_t)malloc(audio_buffer_size);
      if(audio_frames_signal == NULL) {
         XLOGD_ERROR("out of memory");
         break;
      }

      memset(audio_frames_signal, 0, audio_buffer_size);

      #ifdef CTRLMF_LOCAL_MIC_TAP
      if(!ctrlmf_audio_capture_init(audio_frame_size, true)) {
      #else
      if(!ctrlmf_audio_capture_init(audio_frame_size, false)) {
      #endif
         XLOGD_ERROR("unable to init audio capture");
         break;
      }
      audio_capture_initialized = true;

      #ifdef CTRLMF_LOCAL_MIC_TAP
      const char *request_type = (MIC_RAW_AUDIO) ? "mic_factory_test" : (MIC_CHANNEL_QTY == 1) ? "mic_tap_stream_single" : "mic_tap_stream_multi";
      #else
      const char *request_type = (MIC_RAW_AUDIO) ? "mic_factory_test" : (MIC_CHANNEL_QTY == 1) ? "mic_stream_single" : "mic_stream_multi";
      #endif

      // Capture signal
      if(!ctrlmf_audio_capture_start(request_type, audio_frames_signal, audio_frame_qty, duration)) {
         XLOGD_ERROR("unable to capture signal");
         break;
      }

      XLOGD_INFO("analyze audio capture");
      #ifndef CTRLMF_CUSTOM_AUDIO_ANALYSIS
      if(!ctrlmf_mic_test_audio_analyze(output_filename, level, NULL, audio_frames_signal, audio_frame_qty, MIC_CHANNEL_QTY, snr_min, snr_max, snr_var, test_result)) {
      #else
      if(!ctrlmf_mic_test_audio_analyze(CTRLMF_TEST_FACTORY, output_filename, level, NULL, audio_frames_signal, audio_frame_qty, MIC_CHANNEL_QTY, test_result)) {
      #endif
         XLOGD_ERROR("unable to analyze audio");
         break;
      }
      if(!ctrlmf_is_production()) {
         ctrlmf_mic_test_audio_export(FILENAME_WAV_SIGNAL, audio_frames_signal, audio_frame_qty, MIC_CHANNEL_QTY);
      }
      result = true;
   } while(0);

   if(audio_capture_initialized && !ctrlmf_audio_capture_term()) {
      XLOGD_ERROR("unable to term audio capture");
   }
   if(audio_frames_signal != NULL) {
      free(audio_frames_signal);
   }

   return(result);
}

#ifndef CTRLMF_CUSTOM_AUDIO_ANALYSIS
bool ctrlmf_mic_test_audio_analyze(const char *output_filename, uint32_t level, ctrlmf_audio_frame_t audio_frames_noise, ctrlmf_audio_frame_t audio_frames_signal, uint32_t frame_qty, uint32_t mic_qty, double snr_min, double snr_max, double snr_var, ctrlmf_test_result_t *test_result) {
   double pcm_sq_sum_noise[mic_qty];
   double pcm_sq_sum_signal[mic_qty];

   if(audio_frames_noise == NULL) {
      XLOGD_ERROR("ambient test is not supported");
      return(false);
   }

   XLOGD_INFO("frame qty <%u> mic qty <%u>", frame_qty, mic_qty);

   if(test_result != NULL) {
      test_result->pass = true;
   }

   // Measure signal to noise ratio for each microphone and confirm each is within acceptable range
   for(uint32_t mic = 0; mic < mic_qty; mic++) {
      pcm_sq_sum_noise[mic]  = 0.0;
      pcm_sq_sum_signal[mic] = 0.0;
      for(uint32_t frame = 0; frame < frame_qty; frame++) {
         int32_t *samples_noise  = &audio_frames_noise[frame][mic][0];
         int32_t *samples_signal = &audio_frames_signal[frame][mic][0];
         XLOGD_DEBUG("mic <%u> frame <%u> samples noise %12d %12d signal %12d %12d", mic, frame, samples_noise[0], samples_noise[1], samples_signal[0], samples_signal[1]);
         for(uint32_t index = 0; index < SAMPLES_PER_FRAME; index++) {
            pcm_sq_sum_noise[mic]  += (double)samples_noise[index]  * (double)samples_noise[index];
            pcm_sq_sum_signal[mic] += (double)samples_signal[index] * (double)samples_signal[index];
         }
      }
   }

   double power_noise[mic_qty];
   double power_signal[mic_qty];
   double snr[mic_qty];
   double snr_sum = 0.0;

   uint32_t sample_qty = frame_qty * SAMPLES_PER_FRAME;

   for(uint32_t mic = 0; mic < mic_qty; mic++) {
      power_noise[mic]  = pcm_sq_sum_noise[mic]  / (double)sample_qty;
      power_signal[mic] = pcm_sq_sum_signal[mic] / (double)sample_qty;

      // SNR = 20 log(signal / noise)
      snr[mic] = 20 * log10(power_signal[mic] / power_noise[mic]);

      if(test_result != NULL) {
         test_result->snr[mic] = snr[mic];
      }

      snr_sum += snr[mic];

      XLOGD_DEBUG("mic <%u> power noise <%5.2f> signal <%5.2f>", mic, power_noise[mic], power_signal[mic]);
      XLOGD_INFO("mic <%u> SNR <%5.2f>", mic, snr[mic]);

      if(snr[mic] < snr_min) {
         XLOGD_ERROR("mic <%u> SNR <%5.2f> less than min <%5.2f>", mic, snr[mic], snr_min);
         if(test_result != NULL) {
            test_result->pass = false;
         }
      } else if(snr[mic] > snr_max) {
         XLOGD_ERROR("mic <%u> SNR <%5.2f> greater than max <%5.2f>", mic, snr[mic], snr_max);
         if(test_result != NULL) {
            test_result->pass = false;
         }
      }
   }

   double snr_avg = snr_sum / mic_qty;
   for(uint32_t mic = 0; mic < mic_qty; mic++) {
      if(fabsf(snr[mic] - snr_avg) > snr_var) {
         XLOGD_ERROR("mic <%u> SNR <%5.2f> avg <%5.2f> variance out of tolerance - max var <%5.2f>", mic, snr[mic], snr_avg, snr_var);
         if(test_result != NULL) {
            test_result->pass = false;
         }
      }
   }
   return(true);
}
#endif

void ctrlmf_mic_test_audio_export(const char *filename, ctrlmf_audio_frame_t audio_frames, uint32_t frame_qty, uint32_t channel_qty) {
   uint8_t header[WAVE_HEADER_SIZE_MIN];
   uint32_t sample_rate = 16000;
   uint32_t sample_size = 4;

   if(sample_size > 3) { // Wave files don't seem to support 32-bit PCM so it's converted to 24-bit
      sample_size = 3;
   }
   uint32_t pcm_data_size = frame_qty * SAMPLES_PER_FRAME * channel_qty * sample_size;

   errno = 0;
   FILE *fh = fopen(filename, "w");
   if(NULL == fh) {
      int errsv = errno;
      XLOGD_ERROR("Unable to open file <%s> <%s>", filename, strerror(errsv));
      return;
   }

   XLOGD_INFO("write wave header - %u-bit PCM %u hz %u chans %lu bytes", sample_size * 8, sample_rate, channel_qty, pcm_data_size);

   ctrlmf_wave_header_gen(header, 1, channel_qty, sample_rate, sample_size * 8, pcm_data_size);

   size_t bytes_written = fwrite(header, 1, WAVE_HEADER_SIZE_MIN, fh);
   if(bytes_written != WAVE_HEADER_SIZE_MIN) {
      int errsv = errno;
      XLOGD_ERROR("wave header write <%s>", strerror(errsv));
   }

   for(uint32_t frame = 0; frame < frame_qty; frame++) {
      for(uint32_t index = 0; index < SAMPLES_PER_FRAME; index++) {
         uint32_t data_size = channel_qty * 3;
         uint8_t  tmp_buf[data_size];
         uint32_t j = 0;
         for(uint32_t mic = 0; mic < channel_qty; mic++) {
            int32_t sample = audio_frames[frame][mic][index];

            // Handle endian difference for wave container
            #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            tmp_buf[j++] = (sample >> 24) & 0xFF;
            tmp_buf[j++] = (sample >> 16) & 0xFF;
            tmp_buf[j++] = (sample >>  8) & 0xFF;
            #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            tmp_buf[j++] = (sample >>  8) & 0xFF;
            tmp_buf[j++] = (sample >> 16) & 0xFF;
            tmp_buf[j++] = (sample >> 24) & 0xFF;
            #else
            #error unhandled byte order
            #endif
         }

         size_t bytes_written = fwrite(&tmp_buf[0], 1, data_size, fh);

         if(bytes_written != data_size) {
            XLOGD_ERROR("Error (%zd)", bytes_written);
         }
      }
   }

   fclose(fh);
}

void ctrlmf_wave_header_gen(uint8_t *header, uint16_t audio_format, uint16_t num_channels, uint32_t sample_rate, uint16_t bits_per_sample, uint32_t pcm_data_size) {
   uint32_t byte_rate      = sample_rate * num_channels * bits_per_sample / 8;
   uint32_t chunk_size     = pcm_data_size + 36;
   uint16_t block_align    = num_channels * bits_per_sample / 8;
   uint32_t  fmt_chunk_size = 16;
   header[0]  = 'R';
   header[1]  = 'I';
   header[2]  = 'F';
   header[3]  = 'F';
   header[4]  = (uint8_t)(chunk_size);
   header[5]  = (uint8_t)(chunk_size >> 8);
   header[6]  = (uint8_t)(chunk_size >> 16);
   header[7]  = (uint8_t)(chunk_size >> 24);
   header[8]  = 'W';
   header[9]  = 'A';
   header[10] = 'V';
   header[11] = 'E';
   header[12] = 'f';
   header[13] = 'm';
   header[14] = 't';
   header[15] = ' ';
   header[16] = (uint8_t)(fmt_chunk_size);
   header[17] = (uint8_t)(fmt_chunk_size >> 8);
   header[18] = (uint8_t)(fmt_chunk_size >> 16);
   header[19] = (uint8_t)(fmt_chunk_size >> 24);
   header[20] = (uint8_t)(audio_format);
   header[21] = (uint8_t)(audio_format >> 8);
   header[22] = (uint8_t)(num_channels);
   header[23] = (uint8_t)(num_channels >> 8);
   header[24] = (uint8_t)(sample_rate);
   header[25] = (uint8_t)(sample_rate >> 8);
   header[26] = (uint8_t)(sample_rate >> 16);
   header[27] = (uint8_t)(sample_rate >> 24);
   header[28] = (uint8_t)(byte_rate);
   header[29] = (uint8_t)(byte_rate >> 8);
   header[30] = (uint8_t)(byte_rate >> 16);
   header[31] = (uint8_t)(byte_rate >> 24);
   header[32] = (uint8_t)(block_align);
   header[33] = (uint8_t)(block_align >> 8);
   header[34] = (uint8_t)(bits_per_sample);
   header[35] = (uint8_t)(bits_per_sample >> 8);
   header[36] = 'd';
   header[37] = 'a';
   header[38] = 't';
   header[39] = 'a';
   header[40] = (uint8_t)(pcm_data_size);
   header[41] = (uint8_t)(pcm_data_size >> 8);
   header[42] = (uint8_t)(pcm_data_size >> 16);
   header[43] = (uint8_t)(pcm_data_size >> 24);
}
