// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2019 Intel Corporation. All rights reserved.
//
// Author: Daniel Bogdzia <danielx.bogdzia@linux.intel.com>
//         Janusz Jankowski <janusz.jankowski@linux.intel.com>

#include "util.h"

#include <sof/audio/component.h>
#include <sof/audio/format.h>
#include <sof/audio/mux.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <cmocka.h>

struct test_data {
	uint32_t format;
	uint8_t mask[MUX_MAX_STREAMS][PLATFORM_MAX_CHANNELS];
	void *output;
	struct comp_dev *dev;
	struct comp_buffer *sources[MUX_MAX_STREAMS];
	struct comp_buffer *sink;
};

static int16_t input_16b[MUX_MAX_STREAMS][PLATFORM_MAX_CHANNELS] = {
	{ 0x101, 0x102, 0x104, 0x108,
	  0x111, 0x112, 0x114, 0x118, },
	{ 0x201, 0x202, 0x204, 0x208,
	  0x211, 0x212, 0x214, 0x218, },
	{ 0x301, 0x302, 0x304, 0x308,
	  0x311, 0x312, 0x314, 0x318, },
	{ 0x401, 0x402, 0x404, 0x408,
	  0x411, 0x412, 0x414, 0x418, },
};

static int32_t input_32b[MUX_MAX_STREAMS][PLATFORM_MAX_CHANNELS] = {
	{ 0xd1a1001, 0xd2a2002, 0xd4a4004, 0xd8a8008,
	  0xe1b1011, 0xe2b2012, 0xe4b4014, 0xe8b8018, },
	{ 0xd1a1101, 0xd2a2102, 0xd4a4104, 0xd8a8108,
	  0xe1b1111, 0xe2b2112, 0xe4b4114, 0xe8b8118, },
	{ 0xd1a1201, 0xd2a2202, 0xd4a4204, 0xd8a8208,
	  0xe1b1211, 0xe2b2212, 0xe4b4214, 0xe8b8218, },
	{ 0xd1a1401, 0xd2a2402, 0xd4a4404, 0xd8a8408,
	  0xe1b1411, 0xe2b2412, 0xe4b4414, 0xe8b8418, },
};

static uint16_t valid_formats[] = {
	SOF_IPC_FRAME_S16_LE,
	SOF_IPC_FRAME_S24_4LE,
	SOF_IPC_FRAME_S32_LE,
};

static uint8_t masks[][MUX_MAX_STREAMS][PLATFORM_MAX_CHANNELS] = {
	{ { 0x01, }, },
	{ { 0x01, },
	  { 0x01, },
	  { 0x01, },
	  { 0x01, }, },
	{ { 0x00, 0x00, 0x00, 0x01, },
	  { 0x00, 0x00, 0x01, },
	  { 0x00, 0x01, },
	  { 0x01, }, },
	{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, },
	  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, },
	  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, },
	  { 0x00, 0x00, 0x00, 0x00, 0x01, }, },
	{ { 0x02, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, },
	  { 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, },
	  { 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x3f, },
	  { 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xff, }, },
	{ { 0x0f, 0x00, 0xf0, 0x00, 0x0f, 0x00, 0xf0, 0x00, },
	  { 0x00, 0x0f, 0x00, 0xf0, 0x00, 0x0f, 0x00, 0xf0, },
	  { 0x0f, 0x00, 0xf0, 0x00, 0x0f, 0x00, 0xf0, 0x00, },
	  { 0x00, 0x0f, 0x00, 0xf0, 0x00, 0x0f, 0x00, 0xf0, }, },
	{ { 0xf0, 0x00, 0x0f, 0x00, 0xf0, 0x00, 0x0f, 0x00, },
	  { 0x00, 0xf0, 0x00, 0x0f, 0x00, 0xf0, 0x00, 0x0f, },
	  { 0xf0, 0x00, 0x0f, 0x00, 0xf0, 0x00, 0x0f, 0x00, },
	  { 0x00, 0xf0, 0x00, 0x0f, 0x00, 0xf0, 0x00, 0x0f, }, },
	{ { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, },
	  { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, },
	  { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, },
	  { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, }, },
	{ { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, },
	  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, },
	  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, },
	  { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, }, },
};

static int setup_group(void **state)
{
	sys_comp_init();
	sys_comp_mux_init();

	return 0;
}

static struct sof_ipc_comp_process *create_mux_comp_ipc(struct test_data *td)
{
	size_t ipc_size = sizeof(struct sof_ipc_comp_process);
	size_t mux_size = sizeof(struct sof_mux_config)
			  + MUX_MAX_STREAMS * sizeof(struct mux_stream_data);
	struct sof_ipc_comp_process *ipc = calloc(1, ipc_size + mux_size);
	struct sof_mux_config *mux = (struct sof_mux_config *)&ipc->data;
	int i, j;

	ipc->comp.hdr.size = sizeof(struct sof_ipc_comp_process);
	ipc->comp.type = SOF_COMP_MUX;
	ipc->config.hdr.size = sizeof(struct sof_ipc_comp_config);
	ipc->size = mux_size;

	mux->frame_format = td->format;
	mux->num_channels = PLATFORM_MAX_CHANNELS;
	mux->num_streams = MUX_MAX_STREAMS;

	for (i = 0; i < MUX_MAX_STREAMS; ++i) {
		mux->streams[i].pipeline_id = i;
		mux->streams[i].num_channels = PLATFORM_MAX_CHANNELS;
		for (j = 0; j < PLATFORM_MAX_CHANNELS; ++j)
			mux->streams[i].mask[j] = td->mask[i][j];
	}

	return ipc;
}

static void prepare_sink(struct test_data *td, size_t sample_size)
{
	td->sink = create_test_sink(td->dev,
				    MUX_MAX_STREAMS + 1,
				    td->format,
				    PLATFORM_MAX_CHANNELS);
	td->sink->free = sample_size * PLATFORM_MAX_CHANNELS;
	td->output = malloc(sample_size * PLATFORM_MAX_CHANNELS);
	td->sink->w_ptr = td->output;
}

static void prepare_sources(struct test_data *td, size_t sample_size)
{
	int i;

	for (i = 0; i < MUX_MAX_STREAMS; ++i) {
		td->sources[i] = create_test_source(td->dev,
						    i,
						    td->format,
						    PLATFORM_MAX_CHANNELS);
		td->sources[i]->avail = sample_size * PLATFORM_MAX_CHANNELS;

		if (td->format == SOF_IPC_FRAME_S16_LE)
			td->sources[i]->r_ptr = input_16b[i];
		else
			td->sources[i]->r_ptr = input_32b[i];
	}
}

static int setup_test_case(void **state)
{
	struct test_data *td = *((struct test_data **)state);
	struct sof_ipc_comp_process *ipc = create_mux_comp_ipc(td);
	size_t sample_size = td->format == SOF_IPC_FRAME_S16_LE ?
			     sizeof(int16_t) : sizeof(int32_t);
	int ret = 0;

	td->dev = comp_new((struct sof_ipc_comp *)ipc);
	free(ipc);

	if (!td->dev)
		return -EINVAL;

	prepare_sink(td, sample_size);

	prepare_sources(td, sample_size);

	ret = comp_prepare(td->dev);
	if (ret)
		return ret;

	return 0;
}

static int teardown_test_case(void **state)
{
	struct test_data *td = *((struct test_data **)state);
	int i;

	for (i = 0; i < MUX_MAX_STREAMS; ++i)
		free_test_source(td->sources[i]);

	free(td->output);
	free_test_sink(td->sink);

	comp_free(td->dev);

	return 0;
}

static void test_mux_copy_proc_16(void **state)
{
	struct test_data *td = *((struct test_data **)state);
	int16_t expected_result[PLATFORM_MAX_CHANNELS];
	int i, j, k;

	assert_int_equal(comp_copy(td->dev), 0);

	for (i = 0; i < PLATFORM_MAX_CHANNELS; ++i) {
		int32_t sample = 0;

		for (j = 0; j < MUX_MAX_STREAMS; ++j) {
			for (k = 0; k < PLATFORM_MAX_CHANNELS; ++k) {
				if (td->mask[j][i] & BIT(k))
					sample += input_16b[j][k];
			}
		}
		expected_result[i] = sat_int16(sample);
	}

	assert_memory_equal(td->output, expected_result,
			    sizeof(expected_result));
}

static void test_mux_copy_proc_24(void **state)
{
	struct test_data *td = *((struct test_data **)state);
	int32_t expected_result[PLATFORM_MAX_CHANNELS];
	int i, j, k;
	int32_t val;

	assert_int_equal(comp_copy(td->dev), 0);

	for (i = 0; i < PLATFORM_MAX_CHANNELS; ++i) {
		int64_t sample = 0;

		for (j = 0; j < MUX_MAX_STREAMS; ++j) {
			for (k = 0; k < PLATFORM_MAX_CHANNELS; ++k) {
				if (td->mask[j][i] & BIT(k)) {
					val = input_32b[j][k];
					sample += sign_extend_s24(val);
				}
			}
		}
		expected_result[i] = sat_int24(sample);
	}

	assert_memory_equal(td->output, expected_result,
			    sizeof(expected_result));
}

static void test_mux_copy_proc_32(void **state)
{
	struct test_data *td = *((struct test_data **)state);
	int32_t expected_result[PLATFORM_MAX_CHANNELS];
	int i, j, k;

	assert_int_equal(comp_copy(td->dev), 0);

	for (i = 0; i < PLATFORM_MAX_CHANNELS; ++i) {
		int64_t sample = 0;

		for (j = 0; j < MUX_MAX_STREAMS; ++j) {
			for (k = 0; k < PLATFORM_MAX_CHANNELS; ++k) {
				if (td->mask[j][i] & BIT(k))
					sample += input_32b[j][k];
			}
		}
		expected_result[i] = sat_int32(sample);
	}

	assert_memory_equal(td->output, expected_result,
			    sizeof(expected_result));
}

static char *get_test_name(int mask_index, const char *format_name)
{
	int length = snprintf(NULL, 0, "test_mux_copy_%s_mask_%d",
			      format_name, mask_index) + 1;
	char *buffer = malloc(length);

	snprintf(buffer, length, "test_mux_copy_%s_mask_%d",
		 format_name, mask_index);

	return buffer;
}

int main(void)
{
	int i, j;
	struct CMUnitTest tests[ARRAY_SIZE(valid_formats) * ARRAY_SIZE(masks)];

	for (i = 0; i < ARRAY_SIZE(valid_formats); ++i) {
		for (j = 0; j < ARRAY_SIZE(masks); ++j) {
			int ti = i * ARRAY_SIZE(masks) + j;
			struct test_data *td = malloc(sizeof(struct test_data));

			td->format = valid_formats[i];

			memcpy_s(td->mask, sizeof(td->mask),
				 masks[j], sizeof(masks[0]));

			switch (td->format) {
#if CONFIG_FORMAT_S16LE
			case SOF_IPC_FRAME_S16_LE:
				tests[ti].name = get_test_name(j, "s16le");
				tests[ti].test_func = test_mux_copy_proc_16;
				break;
#endif /* CONFIG_FORMAT_S16LE */
#if CONFIG_FORMAT_S24LE
			case SOF_IPC_FRAME_S24_4LE:
				tests[ti].name = get_test_name(j, "s24_4le");
				tests[ti].test_func = test_mux_copy_proc_24;
				break;
#endif /* CONFIG_FORMAT_S24LE */
#if CONFIG_FORMAT_S32LE
			case SOF_IPC_FRAME_S32_LE:
				tests[ti].name = get_test_name(j, "s32le");
				tests[ti].test_func = test_mux_copy_proc_32;
				break;
#endif /* CONFIG_FORMAT_S32LE */
			default:
				return -EINVAL;
			}

			tests[ti].initial_state = td;
			tests[ti].setup_func = setup_test_case;
			tests[ti].teardown_func = teardown_test_case;
		}
	}

	cmocka_set_message_output(CM_OUTPUT_TAP);

	return cmocka_run_group_tests(tests, setup_group, NULL);
}
