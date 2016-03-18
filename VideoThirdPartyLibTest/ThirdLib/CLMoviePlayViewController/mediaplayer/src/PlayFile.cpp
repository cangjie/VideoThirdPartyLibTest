#include "PlayFile.h"
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

#define DELMEM(p) { if(p){ delete p; p = NULL;}}

void* CALLBACK _demux_read_media_loop(void *pUserData)
{
	if(pUserData)
	{
		CPlayFile * pUser = (CPlayFile *)pUserData;
		pUser->read_in_file_data_to_proc();
	}
    return NULL;
}

CPlayFile::CPlayFile()
{
	m_str_in_file_url[0] = '\0';
	m_in_fmt_ctx = NULL; //输入格式上下文

	m_in_video_st = NULL;
	m_in_audio_st = NULL;

	m_in_video_codec_ctx = NULL;
	m_in_audio_codec_ctx = NULL ;

	m_in_video_codec = NULL;
	m_in_audio_codec = NULL;

	m_in_video_index = -1;
    m_in_audio_index = -1;

	m_pic_size = 0;
	m_aud_size = 0;

	m_au_convert_ctx = NULL;

	memset(&m_audio_hw_params,0,sizeof(AudioParams));
	memset(&m_audio_src,0,sizeof(AudioParams));
	m_push_and_play_pause = false;
	m_pic_bufffer = NULL;
	m_aud_bufffer = NULL;

	m_audio_buf = NULL;
	m_audio_buf_len = 0;
	m_audio_buf_size = 0;
	m_audio_pos = 0;
	m_video_cur_pts = 0;
	m_audio_cur_pts = 0;
	m_pool_free_avpacket_list.clear();
}

CPlayFile::~CPlayFile()
{
	CloseLocalMediaFile();
}

bool  CPlayFile::OpenLocalMediaFile(const char* szLocalFile)
{
	if(szLocalFile == NULL)
		return false;

	strcpy(m_str_in_file_url,szLocalFile);

	if(!open_in_file_and_get_stream_info())
		return false;

	m_read_file_and_proc_thread.Begin(_demux_read_media_loop,this);

	return open_decoder();
}

bool  CPlayFile::CloseLocalMediaFile()
{
	m_read_file_and_proc_thread.End();
//	SDL_CloseAudio();
	if(m_in_fmt_ctx)
	{
		close_decoder();
		avformat_close_input(&m_in_fmt_ctx);
		m_in_fmt_ctx = NULL;
	}
	clear_all_cache_list();

	if(m_pic_bufffer)
	{
		delete m_pic_bufffer;
		m_pic_bufffer = NULL;
	}
	if(m_aud_bufffer)
	{
		delete m_aud_bufffer;
		m_aud_bufffer = NULL;
	}
	
	if(m_audio_buf)
	{
		av_free(m_audio_buf);
		m_audio_buf = NULL;
	}
	//close out put

	if(m_au_convert_ctx)
	{
		swr_free(&m_au_convert_ctx);
		m_au_convert_ctx = NULL;
	}
	return false;
}

bool CPlayFile::open_in_file_and_get_stream_info()
{
	//打开输入文件
	if (avformat_open_input(&m_in_fmt_ctx, m_str_in_file_url, 0, 0)< 0)
	{
		return false;
	}

	//查找文件的流信息
	if (avformat_find_stream_info(m_in_fmt_ctx, 0) < 0)
	{
		return false;
	}

	av_dump_format(m_in_fmt_ctx, 0, m_str_in_file_url, 0);

	//解复用流
	for (int i = 0; i < m_in_fmt_ctx->nb_streams; i++) 
	{
		if ( m_in_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
		{
			m_in_video_index = i;
			m_in_video_st = m_in_fmt_ctx->streams[m_in_video_index];
		}
		if ( m_in_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			m_in_audio_index = i;
			m_in_audio_st = m_in_fmt_ctx->streams[m_in_audio_index];
		}
	}

	if(m_in_audio_st == NULL && m_in_audio_st == NULL)
	{
		return false;
	}

	if(m_in_video_st)
	{
		m_in_video_codec_ctx = m_in_video_st->codec;
	}
	//获取音频的编码相关信息
	if(m_in_audio_st)
	{
		m_in_audio_codec_ctx = m_in_audio_st->codec;
	}
	return true;
}


AVPacket* CPlayFile::get_free_pkt()
{
	m_pool_free_avpacket_lock.Lock();
	AVPacket* avPack = NULL;
	if(m_pool_free_avpacket_list.size() > 0)
	{
		avPack = m_pool_free_avpacket_list.front();
		m_pool_free_avpacket_list.pop_front();
	}
	else
	{
		avPack = new AVPacket();
		av_init_packet(avPack);
	}
	m_pool_free_avpacket_lock.Unlock();
	return avPack;
}

void  CPlayFile::put_free_pkt(AVPacket* pNode)
{
	if(pNode)
	{

		m_pool_free_avpacket_lock.Lock();
		if(m_pool_free_avpacket_list.size() > 10)
		{
			av_free_packet(pNode);
			DELMEM(pNode);
		}
		else
		{
			av_free_packet(pNode);
			av_init_packet(pNode);
			m_pool_free_avpacket_list.push_back(pNode);
		}
		m_pool_free_avpacket_lock.Unlock();
	}
}

void CPlayFile::clear_busy_cache_list()
{
	m_pool_busy_audio_avpacket_lock.Lock();
	list<AVPacket*> ::iterator apk_iter;
	for(apk_iter = m_pool_busy_audio_avpacket_list.begin();apk_iter != m_pool_busy_audio_avpacket_list.end();apk_iter++)
	{
		AVPacket* pkt = (*apk_iter);
		if(pkt)
		{
			av_free_packet(pkt);
			DELMEM(pkt);
		}
	}
	m_pool_busy_audio_avpacket_list.clear();
	m_pool_busy_audio_avpacket_lock.Unlock();

	m_pool_busy_video_avpacket_lock.Lock();
	for(apk_iter = m_pool_busy_video_avpacket_list.begin();apk_iter != m_pool_busy_video_avpacket_list.end();apk_iter++)
	{
		AVPacket* pkt = (*apk_iter);
		if(pkt)
		{
			av_free_packet(pkt);
			DELMEM(pkt);
		}
	}
	m_pool_busy_video_avpacket_list.clear();
	m_pool_busy_video_avpacket_lock.Unlock();
}

void CPlayFile::clear_all_cache_list()
{
	clear_busy_cache_list();
	list<AVPacket*> ::iterator apk_iter;
	m_pool_free_avpacket_lock.Lock();
	for(apk_iter = m_pool_free_avpacket_list.begin();apk_iter != m_pool_free_avpacket_list.end();apk_iter++)
	{
		AVPacket* pkt = (*apk_iter);
		if(pkt)
		{
			av_free_packet(pkt);
			DELMEM(pkt);
		}
	}
	m_pool_free_avpacket_list.clear();
	m_pool_free_avpacket_lock.Unlock();
}

bool CPlayFile::bIsGetMediaPacket()
{
	int nVideoPacketSize = 0;
	int nAudioPacketSize = 0;
	m_pool_busy_video_avpacket_lock.Lock();
	nVideoPacketSize = m_pool_busy_video_avpacket_list.size();
	m_pool_busy_video_avpacket_lock.Unlock();

	m_pool_busy_audio_avpacket_lock.Lock();
	nAudioPacketSize = m_pool_busy_audio_avpacket_list.size();
	m_pool_busy_audio_avpacket_lock.Unlock();
	if(nVideoPacketSize < 20 || nAudioPacketSize < 20)
		return true;
	return false;
}

void CPlayFile::read_in_file_data_to_proc()
{
	while(!m_read_file_and_proc_thread.GetStop())
	{
		if(bIsGetMediaPacket())
		{
			m_play_lock.Lock();
			AVPacket *pkt = get_free_pkt();
			if(av_read_frame(m_in_fmt_ctx, pkt) >= 0)
			{
				if(pkt->stream_index == m_in_video_index)
				{
					m_pool_busy_video_avpacket_lock.Lock();
					m_pool_busy_video_avpacket_list.push_back(pkt);
					m_pool_busy_video_avpacket_lock.Unlock();
				}
				else if(pkt->stream_index == m_in_audio_index)
				{
					m_pool_busy_audio_avpacket_lock.Lock();
					m_pool_busy_audio_avpacket_list.push_back(pkt);
					m_pool_busy_audio_avpacket_lock.Unlock();
				}
				m_play_lock.Unlock();
				continue;
			}
			else
			{
				m_play_lock.Unlock();
				put_free_pkt(pkt);
			}
		}
		av_usleep(50 * 1000);
	}
}


bool CPlayFile::open_decoder()
{
	if(m_in_video_codec_ctx)
	{
		m_in_video_codec = avcodec_find_decoder(m_in_video_codec_ctx->codec_id);
		if (!m_in_video_codec) 
		{
			fprintf(stderr, "could not find video decoder!\n");
			return false;
		}
		if (avcodec_open2(m_in_video_codec_ctx, m_in_video_codec,NULL) < 0) 
		{
			fprintf(stderr, "could not open video codec!\n");
			return false;
		}

		m_pic_size =m_in_video_codec_ctx->coded_width * m_in_video_codec_ctx->coded_height * 3 / 2;
		if(m_pic_size == 0)
			return false;
	}

	if(m_in_audio_codec_ctx)
	{
		m_in_audio_codec = avcodec_find_decoder(m_in_audio_codec_ctx->codec_id);
		if (!m_in_audio_codec)
		{
			fprintf(stderr, "could not find audio decoder!\n");
			return false;
		}

		if (avcodec_open2(m_in_audio_codec_ctx, m_in_audio_codec,NULL) < 0) 
		{
			fprintf(stderr, "could not open audio codec!\n");
			return false;
		}

		m_aud_size = AVCODEC_MAX_AUDIO_FRAME_SIZE ;

		int	out_linesize = 0;
		av_samples_get_buffer_size(&out_linesize,m_in_audio_codec_ctx->channels,
				m_in_audio_codec_ctx->frame_size,m_in_audio_codec_ctx->sample_fmt, 1); 
	}
	return true;
}

bool CPlayFile::close_decoder()
{
	if(m_in_video_codec)
	{
		avcodec_close(m_in_video_codec_ctx);
		m_in_audio_codec_ctx = NULL;
	}
	if(m_in_audio_codec)
	{
		avcodec_close(m_in_audio_codec_ctx);
		m_in_audio_codec_ctx = NULL;
	}
	return true;
}

AVPacket* CPlayFile::pull_video_busy_pkt()
{
	AVPacket* avp = NULL;
	m_pool_busy_video_avpacket_lock.Lock();
	if(m_pool_busy_video_avpacket_list.size() > 0)
	{
		avp = m_pool_busy_video_avpacket_list.front();
		m_pool_busy_video_avpacket_list.pop_front();
	}
	m_pool_busy_video_avpacket_lock.Unlock();
	return avp;
}

AVPacket* CPlayFile::pull_audio_busy_pkt()
{
	AVPacket* avp = NULL;
	m_pool_busy_audio_avpacket_lock.Lock();
	if(m_pool_busy_audio_avpacket_list.size() > 0)
	{
		avp = m_pool_busy_audio_avpacket_list.front();
		m_pool_busy_audio_avpacket_list.pop_front();
		
	}
	m_pool_busy_audio_avpacket_lock.Unlock();
	return avp;
}

bool CPlayFile::decode_one_video_frame(VideoFrame&  vf,int& nSpan)
{
	if(m_push_and_play_pause)
		return false;
	bool bIsOK = false;
	int got_picture;
	AVFrame *pframe = avcodec_alloc_frame();
	AVPacket* pkt = NULL;
	pkt = pull_video_busy_pkt();
	if(pkt )
	{
		if(pkt->stream_index == m_in_video_index && m_in_video_codec_ctx)
		{
			int nR = avcodec_decode_video2(m_in_video_codec_ctx, pframe, &got_picture, pkt);
			if (got_picture)
			{
				if (pkt->pts != AV_NOPTS_VALUE)
				{
					pframe->pts = pframe->pkt_pts;
				}
				else if(pkt->dts != AV_NOPTS_VALUE)
				{
					pframe->pts = pframe->pkt_dts;
				}
				else
				{
					pframe->pts = av_frame_get_best_effort_timestamp(pframe);
				}
				nSpan = pframe->pts - m_video_cur_pts;
				m_video_cur_pts = pframe->pts;

				vf.nVideoFrameHeight = pframe->height;
				vf.nVideoFrameWidth= pframe->width;
				vf.mVideoFramePts = pframe->pts;
				if(m_pic_bufffer == NULL)
				{
					m_pic_bufffer = new unsigned char[m_pic_size];
				}
				for(int i=0, nDataLen=0; i<3; i++)
				{
					int nShift = (i == 0) ? 0 : 1;
					unsigned char *pYUVData = pframe->data[i];
					unsigned int  nW = pframe->width >> nShift;
					unsigned int  nH = pframe->height >> nShift;
					for(int j = 0; j< nH;j++)
					{
						memcpy(m_pic_bufffer+nDataLen,pframe->data[i] + j * pframe->linesize[i], nW); 
						nDataLen += nW;
					}
				}
				//vf.pVideoFrameBuf = m_pic_bufffer;
				vf.nVideoFrameSize = m_pic_size;
				bIsOK = true;
			}
		}
		put_free_pkt(pkt);
	}
	avcodec_free_frame(&pframe);
	return bIsOK;
}

bool CPlayFile::decode_one_audio_frame(AudioFrame& af)
{
	if(m_push_and_play_pause)
		return false;

	bool bIsOK = false;
	AVFrame *pframe = avcodec_alloc_frame();
	int got_picture;
	AVPacket* pkt = NULL;
	pkt = pull_audio_busy_pkt();
	if(pkt)
	{
		if(pkt->stream_index == m_in_audio_index && m_in_audio_codec_ctx)
		{
			int len = 0;
			while(pkt->size > 0)
			{
				if ((len = avcodec_decode_audio4(m_in_audio_codec_ctx, pframe, &got_picture, pkt)) >= 0) 
				{
					if(got_picture)
					{
						pframe->pts = pkt->dts;
						m_audio_cur_pts = pframe->pts;
						copy_audio_frame(pframe,af);
						bIsOK = true;
					}
					pkt->size -= len;
				}
				else
				{
					break;
				}		
			}
		}
		put_free_pkt(pkt);
	}
	avcodec_free_frame(&pframe);
	return bIsOK;
}

bool CPlayFile::copy_audio_frame(AVFrame * pframe,AudioFrame & af)
{ 

	unsigned int n_buf_size = 0;
	if(pframe)
	{
		if(m_aud_bufffer == NULL)
		{
			m_aud_bufffer = new unsigned char[m_aud_size];
		}
		unsigned char* audio_buf = m_aud_bufffer;

		int data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(pframe),
                                            pframe->nb_samples,
                                            (AVSampleFormat)pframe->format, 1);
		int64_t dec_channel_layout =
									(pframe->channel_layout && av_frame_get_channels(pframe) == av_get_channel_layout_nb_channels(pframe->channel_layout)) ?
									pframe->channel_layout : av_get_default_channel_layout(av_frame_get_channels(pframe));

		int wanted_nb_samples = pframe->nb_samples;

		//int wanted_nb_samples = synchronize_audio(is, pframe->nb_samples);

		if (pframe->format        != m_audio_src.fmt            ||
			dec_channel_layout       != m_audio_src.channel_layout ||
			pframe->sample_rate   != m_audio_src.freq           ||
			!m_au_convert_ctx) 
		{
			swr_free(&m_au_convert_ctx);
			m_au_convert_ctx = NULL;
			m_au_convert_ctx = swr_alloc_set_opts(NULL,m_audio_hw_params.channel_layout, m_audio_hw_params.fmt, m_audio_hw_params.freq,
												dec_channel_layout, (AVSampleFormat)pframe->format, pframe->sample_rate,
												0, NULL);
			if (!m_au_convert_ctx || swr_init(m_au_convert_ctx) < 0)
			{
				return false;
			}

			m_audio_src.channel_layout = dec_channel_layout;
			m_audio_src.channels       = av_frame_get_channels(pframe);
			m_audio_src.freq = pframe->sample_rate;
			m_audio_src.fmt = (AVSampleFormat)pframe->format;
		}

		if(m_au_convert_ctx)
		{
			unsigned char* out_buffer = audio_buf;
			const uint8_t **in = (const uint8_t **)pframe->extended_data;
			uint8_t **out = &out_buffer;
			int out_count = (int64_t)wanted_nb_samples * m_audio_hw_params.freq / pframe->sample_rate + 256;
			int out_size  = av_samples_get_buffer_size(NULL, m_audio_hw_params.channels, out_count, m_audio_hw_params.fmt, 0);
			int len2;
			if (out_size < 0) 
			{
				return false;
			}
			if (wanted_nb_samples != pframe->nb_samples) 
			{
				if (swr_set_compensation(m_au_convert_ctx, (wanted_nb_samples - pframe->nb_samples) * m_audio_hw_params.freq / pframe->sample_rate,
											wanted_nb_samples * m_audio_hw_params.freq / pframe->sample_rate) < 0) 
				{
					return false;
				}
			}

			len2 = swr_convert(m_au_convert_ctx, out, out_count, in, pframe->nb_samples);
			if (len2 < 0)
			{
				return false;
			}
			if (len2 == out_count) 
			{
				swr_init(m_au_convert_ctx);
			}
			unsigned int nbufsize = len2 * m_audio_hw_params.channels * av_get_bytes_per_sample(m_audio_hw_params.fmt);
			n_buf_size += nbufsize;
		}
		else
		{
			memcpy(audio_buf,pframe->data[0],data_size);
			n_buf_size += data_size;
		}
		af.nPCMBufSize = n_buf_size;
		af.pPCMBuffer = m_aud_bufffer;
		af.mVideoFramePts = pframe->pts;
		return true;
	}
	return false;
}

int CPlayFile::audio_open(play_audio_callback pCallback,void* dwUser)
{
/*	static SDL_AudioSpec  spec;
    const char *env;
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

	int wanted_sample_rate = m_in_audio_codec_ctx->sample_rate;
	int wanted_nb_channels = m_in_audio_codec_ctx->channels;
	int64_t wanted_channel_layout = m_in_audio_codec_ctx->channel_layout ;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) 
	{
        wanted_nb_channels = atoi(env);
        wanted_nb_channels = av_get_default_channel_layout(wanted_nb_channels);
    }
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) 
	{
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    m_wanted_spec.channels = wanted_nb_channels;
    m_wanted_spec.freq = wanted_sample_rate;
    if (m_wanted_spec.freq <= 0 || m_wanted_spec.channels <= 0) 
	{
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= m_wanted_spec.freq)
        next_sample_rate_idx--;

    m_wanted_spec.format = AUDIO_S16SYS;
    m_wanted_spec.silence = 0;
    m_wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(m_wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    m_wanted_spec.callback = pCallback;
    m_wanted_spec.userdata = dwUser;

	while (SDL_OpenAudio(&m_wanted_spec, &spec) < 0)
	{
		m_wanted_spec.channels = next_nb_channels[FFMIN(7, m_wanted_spec.channels)];
		if (!m_wanted_spec.channels) 
		{
			m_wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
			m_wanted_spec.channels = wanted_nb_channels;
			if (!m_wanted_spec.freq) 
			{
				return -1;
			}
		}
			wanted_channel_layout = av_get_default_channel_layout(m_wanted_spec.channels);
	}

    if (spec.format != AUDIO_S16SYS) 
	{
        return -1;
    }
    if (spec.channels != m_wanted_spec.channels) 
	{
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) 
		{
            return -1;
        }
    }

    m_audio_hw_params.fmt = AV_SAMPLE_FMT_S16;
    m_audio_hw_params.freq = spec.freq;
    m_audio_hw_params.channel_layout = wanted_channel_layout;
    m_audio_hw_params.channels =  spec.channels;
    m_audio_hw_params.frame_size = av_samples_get_buffer_size(NULL, m_audio_hw_params.channels, 1, m_audio_hw_params.fmt, 1);
    m_audio_hw_params.bytes_per_sec = av_samples_get_buffer_size(NULL, m_audio_hw_params.channels, m_audio_hw_params.freq, m_audio_hw_params.fmt, 1);

    if (m_audio_hw_params.bytes_per_sec <= 0 || m_audio_hw_params.frame_size <= 0) 
	{
        return -1;
    }

	m_audio_src = m_audio_hw_params;
	if(spec.size >= 0)
		SDL_PauseAudio(0); 
    return spec.size;*/
    return 0;
}


unsigned int CPlayFile::get_audio_frame(AudioFrame af)
{
	 
	unsigned char* audio_buf = NULL;
	int audio_len = -1;
	unsigned int n_buf_size = 0;
	n_buf_size = af.nPCMBufSize;
	audio_buf = af.pPCMBuffer;
	if(n_buf_size > 0 && audio_buf)
	{
		audio_len = n_buf_size;  
		av_fast_malloc(&m_audio_buf, &m_audio_buf_size, audio_len);
		if(m_audio_buf)
		{
			memcpy(m_audio_buf,audio_buf,audio_len);
		}
		else
		{
			audio_len = -1;
		}
	}
	else
	{
		int n = 0;
	}
	m_audio_pos = 0;
	return  audio_len;
}


void  CPlayFile::audio_play(void *udata,unsigned char *stream,int len,AudioFrame af)
{
	int audio_len = 0;
	int len1 = 0;
	while(len > 0)
	{
		if(m_audio_pos >= m_audio_buf_len)
		{
			audio_len = get_audio_frame(af);
			if(audio_len < 0)
			{
				m_audio_buf_len = 1024;
				av_fast_malloc(&m_audio_buf, &m_audio_buf_size, m_audio_buf_len);
				memset(m_audio_buf,0,m_audio_buf_len);
			}
			else
			{
				m_audio_buf_len = audio_len;
			}
		}

		len1 = m_audio_buf_len - m_audio_pos;
		if(len1 > len)
		{
			len1 = len;
		} 
		//SDL_MixAudio(stream,(uint8_t *)m_audio_buf + m_audio_pos,len1,SDL_MIX_MAXVOLUME);
		//memcpy(stream,(uint8_t *)m_audio_buf + m_audio_pos,len1);
		m_audio_pos += len1;
		len -= len1;
		stream += len1;
	}
}

bool CPlayFile::pause_in_file(bool bIsPause)
{
	m_play_lock.Lock();
	if(m_in_fmt_ctx)
	{
		m_push_and_play_pause = bIsPause;
		//SDL_PauseAudio(bIsPause);
		if(bIsPause)
		{
			av_read_pause(m_in_fmt_ctx);
			clear_busy_cache_list();
		}
		else
		{
			av_read_play(m_in_fmt_ctx);
		}
	}
	m_play_lock.Unlock();
	return 0;
}

bool CPlayFile::seek_in_file(unsigned int nSeekTime)
{
	m_play_lock.Lock();
	clear_busy_cache_list();
	if(m_in_fmt_ctx)
	{
		int64_t seet_time = ((int64_t)nSeekTime) * AV_TIME_BASE;
		if(avformat_seek_file(m_in_fmt_ctx,-1,0,seet_time,m_in_fmt_ctx->duration,0)== 0)
		{
			m_play_lock.Unlock();
			return true;
		}
	}
	m_play_lock.Unlock();
	return false;
}

unsigned int CPlayFile::get_in_file_duration()
{
	if(m_in_fmt_ctx)
	{
		return m_in_fmt_ctx->duration / AV_TIME_BASE;
	}
	return 0;
}

unsigned int CPlayFile::get_in_file_current_play_time()
{
	if(m_in_audio_st)
		return m_audio_cur_pts/1000;
	else if(m_in_video_st)
		return m_video_cur_pts/1000;
	else
		return 0;
}

bool CPlayFile::switch_paly(play_audio_callback pCallback,void* dwUser,bool bflag)
{
	/*if(SDL_GetAudioStatus() == SDL_AUDIO_STOPPED)
	{
		if(m_in_audio_codec_ctx && bflag)
		{
			if(audio_open(pCallback,dwUser) < 0)
				return false;

			SDL_Delay(10);
			SDL_PauseAudio(0); 
		}
	}
	else
	{
			SDL_CloseAudio();
	}*/
	return true;
}