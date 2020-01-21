#
# Topology for JasperLake with rt5682 and rt1015.
#

# Include topology builder
include(`utils.m4')
include(`dai.m4')
include(`pipeline.m4')
include(`ssp.m4')

# Include TLV library
include(`common/tlv.m4')

# Include Token library
include(`sof/tokens.m4')

# Include Icelake DSP configuration

include(`platform/intel/icl.m4')
include(`platform/intel/dmic.m4')

# SSP0 - Headset RT5682 and SSP1 - Speaker RT1015

define('SSP_INDEX', '0')
define('SSP_NAME', 'SSP0-Codec')
define(`SPK_INDEX', `1')
define(`SPK_NAME', `SSP1-Codec')

define('SSP_MCLK_RATE', '24000000')

#
# Define the pipelines
#
# PCM0 <---> volume -----> SSP(SPK_INDEX, BE link 0)
# PCM1 <---> volume <----> SSP(SSP_INDEX, BE link 1)
# PCM5 <---- DMIC0  <----- dmic (DMIC01, BE link 2)
# PCM2 ----> volume -----> iDisp1 (HDMI/DP playback, BE link 3)
# PCM3 ----> Volume -----> iDisp2 (HDMI/DP playback, BE link 4)
# PCM4 ----> volume -----> iDisp3 (HDMI/DP playback, BE link 5)
#
#

dnl PIPELINE_PCM_ADD(pipeline,
dnl     pipe id, pcm, max channels, format,
dnl     period, priority, core,
dnl     pcm_min_rate, pcm_max_rate, pipeline_rate,
dnl     time_domain, sched_comp)

# Low Latency playback pipeline 1 on PCM 1 using max 2 channels of s32le.
# 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	1, 1, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency capture pipeline 2 on PCM 1 using max 2 channels of s32le.
# 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-volume-capture.m4,
	2, 1, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)
	
# Low Latency capture pipeline 3 on PCM 5 using max 4 channels of s32le.
# Schedule 48 frames per 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-passthrough-capture.m4,
	3, 5, 4, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency playback pipeline 4 on PCM 0 using max 2 channels of s32le.
# 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	4, 0, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency playback pipeline 5 on PCM 2 using max 2 channels of s32le.
# Schedule 48 frames per 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	5, 2, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency playback pipeline 6 on PCM 3 using max 2 channels of s32le.
# Schedule 48 frames per 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	6, 3, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency playback pipeline 7 on PCM 4 using max 2 channels of s32le.
# Schedule 48 frames per 1000us deadline on core 0 with priority 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	7, 4, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

#
# DAIs configuration
#

dnl DAI_ADD(pipeline,
dnl     pipe id, dai type, dai_index, dai_be,
dnl     buffer, periods, format,
dnl     deadline, priority, core, time_domain)

# playback DAI is SSP(SPP_INDEX) using 2 periods
# Buffers use s24le format, 1000us deadline on core 0 with priority 0
DAI_ADD(sof/pipe-dai-playback.m4,
	1, SSP, SSP_INDEX, SSP_NAME,
	PIPELINE_SOURCE_1, 2, s24le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# capture DAI is SSP(SSP_INDEX) using 2 periods
# Buffers use s24le format, 1000us deadline on core 0 with priority 0
DAI_ADD(sof/pipe-dai-capture.m4,
	2, SSP,SSP_INDEX, SSP_NAME,
	PIPELINE_SINK_2, 2, s24le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# capture DAI is DMIC0 using 2 periods
# Buffers use s32le format, with 48 frame per 1000us on core 0 with priority 0
DAI_ADD(sof/pipe-dai-capture.m4,
	3, DMIC, 0, dmic01,
	PIPELINE_SINK_3, 2, s32le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)
	
# playback DAI is SSP(SPK_INDEX) using 2 periods
# Buffers use s24le format, 1000us deadline on core 0 with priority 0
DAI_ADD(sof/pipe-dai-playback.m4,
	4, SSP, SPK_INDEX, SPK_NAME,
	PIPELINE_SOURCE_4, 2, s24le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# playback DAI is iDisp1 using 2 periods
# Buffers use s16le format, with 48 frame per 1000us on core 0 with priority 0
DAI_ADD(sof/pipe-dai-playback.m4,
	5, HDA, 0, iDisp1,
	PIPELINE_SOURCE_5, 2, s16le,
	1000, 0, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

DAI_ADD(sof/pipe-dai-playback.m4,
	6, HDA, 1, iDisp2,
	PIPELINE_SOURCE_6, 2, s16le,
	1000, 0, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

DAI_ADD(sof/pipe-dai-playback.m4,
	7, HDA, 2, iDisp3,
	PIPELINE_SOURCE_7, 2, s16le,
	1000, 0, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# PCM Low Latency, id 0
dnl PCM_PLAYBACK_ADD(name, pcm_id, playback)
PCM_PLAYBACK_ADD(Speaker, 0, PIPELINE_PCM_4)
PCM_DUPLEX_ADD(Headset, 1, PIPELINE_PCM_1, PIPELINE_PCM_2)
PCM_CAPTURE_ADD(DMIC01, 5, PIPELINE_PCM_3)
PCM_PLAYBACK_ADD(HDMI1, 2, PIPELINE_PCM_5)
PCM_PLAYBACK_ADD(HDMI2, 3, PIPELINE_PCM_6)
PCM_PLAYBACK_ADD(HDMI3, 4, PIPELINE_PCM_7)


#
# BE configurations - overrides config in ACPI if present
#

#SSP SSP_INDEX (ID: 0)
DAI_CONFIG(SSP, SSP_INDEX, 0, SSP_NAME,
	SSP_CONFIG(I2S, SSP_CLOCK(mclk, SSP_MCLK_RATE, codec_mclk_in),
		      SSP_CLOCK(bclk, 2400000, codec_slave),
		      SSP_CLOCK(fsync, 48000, codec_slave),
		      SSP_TDM(2, 25, 3, 3),
		      SSP_CONFIG_DATA(SSP, SSP_INDEX, 24)))
			  
# SSP SPK_INDEX (ID: 1)
# 48000 * 24 * 2 = 2304000
# 48000 * 25 * 2 = 2400000
DAI_CONFIG(SSP, SPK_INDEX, 0, SPK_NAME,
	   dnl SSP_CONFIG(format, mclk, bclk, fsync, tdm, ssp_config_data)
	   SSP_CONFIG(I2S, SSP_CLOCK(mclk, 24000000, codec_mclk_in),
		      SSP_CLOCK(bclk, 2400000, codec_slave),
		      SSP_CLOCK(fsync, 48000, codec_slave),
		      SSP_TDM(2, 25, 3, 3),
		      SSP_CONFIG_DATA(SSP, SPK_INDEX, 24)))

# dmic01 (ID: 2)
DAI_CONFIG(DMIC, 0, 2, dmic01,
	DMIC_CONFIG(1, 500000, 4800000, 40, 60, 48000,
		DMIC_WORD_LENGTH(s32le), 400, DMIC, 0,
		PDM_CONFIG(DMIC, 0, FOUR_CH_PDM0_PDM1)))

# 3 HDMI/DP outputs (ID: 3,4,5)
DAI_CONFIG(HDA, 0, 3, iDisp1)
DAI_CONFIG(HDA, 1, 4, iDisp2)
DAI_CONFIG(HDA, 2, 5, iDisp3)

