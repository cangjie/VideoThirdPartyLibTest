#include "MediaDecode.h"
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

CMediaDecoder::CMediaDecoder()
{
	m_pVideoCodec = NULL;
	m_pVideoCodecContext = NULL;
	m_pOutVideoFrame = NULL;
	m_yuvbufsize = 0;
	m_yuvbuf = NULL;
	m_bVideoDecodeInitOK = false;
	m_bAudioDecodeInitOK = false;

	m_pAudioCodec = NULL;
	m_pAudioCodecContext = NULL;
	m_pOutAudioFrame = NULL;
	m_pPCMBuffer = NULL;
	m_pPCMBufferSize = 0;
	m_au_convert_ctx = NULL;
	av_register_all();
}

CMediaDecoder::~CMediaDecoder()
{
	UninitH264Decode();
	UninitAACDecode();
	if(m_yuvbuf)
	{
		free(m_yuvbuf);
		m_yuvbuf = NULL;
	}

	if(m_pPCMBuffer)
	{
		free(m_pPCMBuffer);
		m_pPCMBuffer = NULL;
	}
}

bool CMediaDecoder::InitH264Decode()
{

	if(m_bVideoDecodeInitOK)
		return true;

	if(m_pVideoCodec == NULL)
	{
		m_pVideoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
		if(m_pVideoCodec == NULL)
		{
			return false;
		}
	}

	if(m_pVideoCodecContext == NULL)
	{
		m_pVideoCodecContext = (AVCodecContext *)avcodec_alloc_context3(m_pVideoCodec);

		if(m_pVideoCodecContext == NULL)
		{
			return false;
		}

		m_pVideoCodecContext->flags |= CODEC_FLAG_EMU_EDGE;
		if(m_pVideoCodec->capabilities&CODEC_CAP_TRUNCATED)  
			m_pVideoCodecContext->flags|= CODEC_FLAG_TRUNCATED; 
	}

	if(avcodec_open2(m_pVideoCodecContext, m_pVideoCodec,NULL) < 0)
	{
		return false;
	}

	if(m_pOutVideoFrame == NULL)
		m_pOutVideoFrame = avcodec_alloc_frame();

	m_bVideoDecodeInitOK = true;
	return true;
}

bool CMediaDecoder::UninitH264Decode()
{
	if(m_pVideoCodecContext)
	{
		avcodec_close(m_pVideoCodecContext);
		av_free(m_pVideoCodecContext);
		m_pVideoCodecContext = NULL;
		m_pVideoCodec = NULL;
	}
	if(m_pOutVideoFrame)
	{
		av_frame_free(&m_pOutVideoFrame);
		m_pOutVideoFrame = NULL;
	}
	m_bVideoDecodeInitOK = false;
	return true;
}

bool CMediaDecoder::InitAACDecode(int nSampleRate,int channels)
{
	if(m_bAudioDecodeInitOK)
		return true;
	if(m_pAudioCodec == NULL)
	{
		m_pAudioCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
		if(m_pAudioCodec == NULL)
		{
			return false;
		}
	}

	if(m_pAudioCodecContext == NULL)
	{
		m_pAudioCodecContext = (AVCodecContext *)avcodec_alloc_context3(m_pAudioCodec);

		if(m_pAudioCodecContext == NULL)
		{
			return false;
		}
		m_pAudioCodecContext->sample_rate = nSampleRate;
		m_pAudioCodecContext->channels = channels;
	}

	if(avcodec_open2(m_pAudioCodecContext, m_pAudioCodec,NULL) < 0)
	{
		return false;
	}

	if(m_pOutAudioFrame == NULL)
		m_pOutAudioFrame = avcodec_alloc_frame();
	m_nSampleRate = nSampleRate;
	m_nChannels = channels;
	m_bAudioDecodeInitOK = true;
	return true;
}



bool CMediaDecoder::UninitAACDecode()
{
	if(m_pAudioCodecContext)
	{
		avcodec_close(m_pAudioCodecContext);
		av_free(m_pAudioCodecContext);
		m_pAudioCodecContext = NULL;
		m_pAudioCodec = NULL;
	}
	if(m_pOutAudioFrame)
	{
		av_frame_free(&m_pOutAudioFrame);
		m_pOutAudioFrame = NULL;
	}
	m_bAudioDecodeInitOK = false;
	return true;
}


bool CMediaDecoder::DecodeH264Video(const unsigned char* pBuf,unsigned int nSize,int64_t pts,VideoFrame& vf)
{
	if(!m_bVideoDecodeInitOK && !InitH264Decode())
		return false;
	if(pBuf == NULL || nSize <= 4)
		return false;

	AVPacket avpkt;
	int nUsedLen = 0;
	int nGotPicture = 0;
	av_init_packet(&avpkt);

	avpkt.size = nSize;
	avpkt.data = (uint8_t*)pBuf;
	avpkt.pts = pts;

    //拓展 防止出错
    try {
        nUsedLen = avcodec_decode_video2(m_pVideoCodecContext, m_pOutVideoFrame, &nGotPicture, &avpkt);
        
    } catch (char *str) {
        
    }
		
	if (nGotPicture != 0)
	{
		m_nInputWidth = m_pVideoCodecContext->coded_width > m_pVideoCodecContext->width ?m_pVideoCodecContext->width :  m_pVideoCodecContext->coded_width;
		m_nInputHeight = m_pVideoCodecContext->coded_height > m_pVideoCodecContext->height ? m_pVideoCodecContext->height : m_pVideoCodecContext->coded_height;

		int nSize = m_nInputWidth * m_nInputHeight * 3 / 2;
		if(m_yuvbufsize < nSize)
		{
			m_yuvbufsize = nSize;
			if(m_yuvbuf)
			{
				free(m_yuvbuf);
				m_yuvbuf = NULL;
			}
			m_yuvbuf = (unsigned char*)malloc(m_yuvbufsize);
		}

		for(int i=0, nDataLen=0; i<3; i++)
		{
			int nShift = (i == 0) ? 0 : 1;
			unsigned int  nW = m_pOutVideoFrame->width >> nShift;
			unsigned int  nH = m_pOutVideoFrame->height >> nShift;
			for(int j = 0; j< nH;j++)
			{
				memcpy(m_yuvbuf+nDataLen,m_pOutVideoFrame->data[i] + j * m_pOutVideoFrame->linesize[i], nW); 
				nDataLen += nW;
			}
		}
     

		vf.mVideoFramePts = m_pOutVideoFrame->pts;
		vf.nVideoFrameHeight = m_nInputHeight;
		vf.nVideoFrameWidth = m_nInputWidth;
        
		vf.nVideoFrameSize = m_yuvbufsize;
		vf.pVideoFrameBuf = m_yuvbuf;
        
		return true;
	}
	else
	{
		int n = 0;
	}

	return false;	
}

bool CMediaDecoder::DecodeAACAudio(const unsigned char* pBuf,unsigned int nSize,int64_t pts,AudioFrame& af)
{
	if(m_nSampleRate != af.nSamplerate || m_nChannels != af.nChannel)
	{
		m_bAudioDecodeInitOK = false;
		UninitAACDecode();
	}

	if(!m_bAudioDecodeInitOK && !InitAACDecode(af.nSamplerate,af.nChannel))
		return false;

	if(pBuf == NULL || nSize <= 4)
		return false;

	AVPacket avpkt;
	int nUsedLen = 0;
	int nGotPcm = 0;
	av_init_packet(&avpkt);

	avpkt.size = nSize;
	avpkt.data = (uint8_t*)pBuf;
	avpkt.pts = pts;
	int len = 0;
	int nDataSize = 0;
	while(avpkt.size > 0)
	{
		len = avcodec_decode_audio4(m_pAudioCodecContext, m_pOutAudioFrame, &nGotPcm, &avpkt);
		if(len < 0)
			return false;

		if(nGotPcm)
		{
			 int data_size = av_samples_get_buffer_size(NULL, m_pAudioCodecContext->channels,
                                                       m_pOutAudioFrame->nb_samples,
                                                       m_pAudioCodecContext->sample_fmt, 1);	
			 if(data_size <= 0)
				 return false;

			  if(m_pPCMBufferSize==0 || m_pPCMBufferSize < nDataSize)
			 {
				 if(m_pPCMBuffer)
				 {
					 free(m_pPCMBuffer);
					 m_pPCMBuffer = NULL;
				 }
				 if(AVCODEC_MAX_AUDIO_FRAME_SIZE < nSize)
					m_pPCMBufferSize = AVCODEC_MAX_AUDIO_FRAME_SIZE + nDataSize;
				 else
					m_pPCMBufferSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;

				 m_pPCMBuffer = (unsigned char*)malloc(m_pPCMBufferSize);
			 }

			 int64_t dec_channel_layout =
			 (m_pOutAudioFrame->channel_layout && av_frame_get_channels(m_pOutAudioFrame) == av_get_channel_layout_nb_channels(m_pOutAudioFrame->channel_layout)) ?
			 m_pOutAudioFrame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(m_pOutAudioFrame));

			 int wanted_nb_samples = m_pOutAudioFrame->nb_samples;

		     if (m_pOutAudioFrame->format != AV_SAMPLE_FMT_S16 ||
				 dec_channel_layout !=  m_pAudioCodecContext->channel_layout||
				 m_pOutAudioFrame->sample_rate!= m_nSampleRate ||
				!m_au_convert_ctx) 
			 {
				 swr_free(&m_au_convert_ctx);
				 m_au_convert_ctx = NULL;
				 m_au_convert_ctx = swr_alloc_set_opts(NULL,4,AV_SAMPLE_FMT_S16 , m_nSampleRate,
                                            dec_channel_layout, (AVSampleFormat)m_pOutAudioFrame->format, m_pOutAudioFrame->sample_rate,
                                            0, NULL);
				 if (!m_au_convert_ctx || swr_init(m_au_convert_ctx) < 0)
				 {
					 return false;
				 }

				 int64_t channel_layout = dec_channel_layout;
				 int nchannels       = av_frame_get_channels(m_pOutAudioFrame);
				 int freq = m_pOutAudioFrame->sample_rate;
				 int fmt = (AVSampleFormat)m_pOutAudioFrame->format;
			 }

			 if(m_au_convert_ctx)
			 {
				  unsigned char* out_buffer = m_pPCMBuffer+nDataSize;
				  const uint8_t **in = (const uint8_t **)m_pOutAudioFrame->extended_data;
				  uint8_t **out = &out_buffer;
				  int out_count = (int64_t)wanted_nb_samples * m_nSampleRate / m_pOutAudioFrame->sample_rate + 256;
				  int out_size  = av_samples_get_buffer_size(NULL, m_nChannels, out_count, AV_SAMPLE_FMT_S16, 0);
				  int len2;
				  if (out_size < 0) 
				  { 
					  return false;
				  }
				  if (wanted_nb_samples != m_pOutAudioFrame->nb_samples) 
				  {
					  if (swr_set_compensation(m_au_convert_ctx, (wanted_nb_samples - m_pOutAudioFrame->nb_samples) * m_nSampleRate / m_pOutAudioFrame->sample_rate,
												wanted_nb_samples * m_nSampleRate / m_pOutAudioFrame->sample_rate) < 0) 
					  {
						  return false;
					  }
				  }

				  len2 = swr_convert(m_au_convert_ctx, out, out_count, in, m_pOutAudioFrame->nb_samples);
				  if (len2 < 0)
				  {
					  return false;
				  }
				  if (len2 == out_count) 
				  {
					  swr_init(m_au_convert_ctx);
				  }
				  unsigned int nbufsize = len2 * m_nChannels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

				 nDataSize += nbufsize;
			 }
			 else
			 {
				  memcpy(m_pPCMBuffer+nDataSize,m_pOutAudioFrame->data[0],data_size);
				  nDataSize += data_size;
			 } 
		}

		avpkt.size -= len;
		avpkt.data += len;
		avpkt.dts = avpkt.pts = AV_NOPTS_VALUE;			
	}
	af.mVideoFramePts = pts;
	af.nPCMBufSize = nDataSize;
	af.pPCMBuffer = m_pPCMBuffer;
	return true;
}

