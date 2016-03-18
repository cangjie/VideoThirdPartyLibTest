#ifndef PLAYFILE_H
#define PLAYFILE_H
#include "PlayHeader.h"
#include "MediaDecode.h"
#include "sc_Thread.h"
#include "sc_CSLock.h"
//#include "SDL.h"
//#include "SDL_thread.h"
#include <list>
using namespace std;

enum 
{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef void  (*play_audio_callback)(void *udata,unsigned char *stream,int len);

class CPlayFile
{
public:
	CPlayFile();
	~CPlayFile();
public:
	bool  OpenLocalMediaFile(const char* szLocalFile);
	bool  CloseLocalMediaFile();

	void read_in_file_data_to_proc();

	bool decode_one_video_frame(VideoFrame&  vf,int& nSpan);
	bool decode_one_audio_frame(AudioFrame & af);
	int audio_open(play_audio_callback pCallback,void* dwUser);
	void  audio_play(void *udata,unsigned char *stream,int len,AudioFrame af);

	bool seek_in_file(unsigned int nSeekTime);
	unsigned int get_in_file_duration();
	unsigned int get_in_file_current_play_time();
	bool pause_in_file(bool bIsPause);
	bool switch_paly(play_audio_callback pCallback,void* dwUser,bool bflag = false);

private:
	bool open_in_file_and_get_stream_info();
	bool bIsGetMediaPacket();
	unsigned int get_audio_frame(AudioFrame af);
	bool copy_audio_frame(AVFrame * pframe,AudioFrame & af);

	AVPacket* get_free_pkt();
	void      put_free_pkt(AVPacket*);

	void clear_busy_cache_list();
	void clear_all_cache_list();

	bool open_decoder();
	bool close_decoder();

	AVPacket* pull_video_busy_pkt();
	AVPacket* pull_audio_busy_pkt();
private:
	char  m_str_in_file_url[260];
	CSCThread m_read_file_and_proc_thread;
	bool            m_push_and_play_pause;
	CMutexLock	    m_play_lock;
	int64_t         m_video_cur_pts;
	int64_t         m_audio_cur_pts;       
private:
	list<AVPacket*>   m_pool_free_avpacket_list;
	CMutexLock	      m_pool_free_avpacket_lock;

	//video
	list<AVPacket*>   m_pool_busy_video_avpacket_list;
	CMutexLock	      m_pool_busy_video_avpacket_lock;
	//audio
	list<AVPacket*>   m_pool_busy_audio_avpacket_list;
	CMutexLock	      m_pool_busy_audio_avpacket_lock;

private:
	AVFormatContext * m_in_fmt_ctx; //输入格式上下文

	AVStream*         m_in_video_st;
	AVStream*		  m_in_audio_st;

	AVCodecContext *  m_in_video_codec_ctx;
	AVCodecContext *  m_in_audio_codec_ctx ;

	AVCodec          * m_in_video_codec;
	AVCodec       	 * m_in_audio_codec;

	int               m_in_video_index;
	int				  m_in_audio_index;

	SwrContext *      m_au_convert_ctx;

	unsigned int      m_pic_size;
	unsigned char*    m_pic_bufffer;
	unsigned int      m_aud_size;
	unsigned char*    m_aud_bufffer;
private:
	AudioParams     m_audio_hw_params;
	AudioParams     m_audio_src;

	//SDL_AudioSpec   m_wanted_spec;
	unsigned char*	m_audio_buf;
	int				m_audio_buf_len;
	unsigned int    m_audio_buf_size;
	int			    m_audio_pos;


};
#endif 