// ----------------------------------------------------------------------------
//
//      sound.h
//
// Copyright (C) 2006-2007
//              Dave Freese, W1HKJ
//
// Copyright (C) 2007-2008
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#ifndef _SOUND_H
#define _SOUND_H

#include <cstring>
#include <string>
#include <vector>
#if USE_SNDFILE
	#include <sndfile.h>
#endif
#if USE_PORTAUDIO
	#include <portaudio.h>
#endif
#if USE_PULSEAUDIO
	#include <pulse/simple.h>
	#include <pulse/error.h>
#endif
#include <samplerate.h>

#define MAXSC 32767.0;
#define maxsc 32000.0
//#define maxsc 16384.0
#define SCBLOCKSIZE 512

#define	SND_BUF_LEN		65536
//#define	SRC_BUF_LEN		(8*SND_BUF_LEN)


class SndException : public std::exception
{
public:
	SndException(int err_ = 0)
		: err(err_), msg(std::string("Sound error: ") + err_to_str(err_))
	{ }
	SndException(const char* msg_)
		: err(1), msg(msg_)
	{ }
	SndException(int err_, const std::string& msg_) : err(err_), msg(msg_) { }
	virtual ~SndException() throw() { }

	const char*	what(void) const throw() { return msg.c_str(); }
	int		error(void) const { return err; }

protected:
	const char* err_to_str(int e) { return strerror(e); }

	int		err;
	std::string	msg;
};

#if USE_PORTAUDIO
class SndPortException : public SndException
{
public:
	SndPortException(int err_ = 0)
		: SndException(err_, std::string("PortAudio error: ") + err_to_str(err_))
	{ }
	SndPortException(const char* msg_) : SndException(msg_) { }
protected:
	const char* err_to_str(int e) { return Pa_GetErrorText(e); }
};
#endif

#if USE_PULSEAUDIO
class SndPulseException : public SndException
{
public:
	SndPulseException(int err_ = 0)
		: SndException(err_, std::string("PulseAudio error: ") + err_to_str(err_))
	{ }
	SndPulseException(const char* msg_) : SndException(msg_) { }
protected:
	const char* err_to_str(int e) { return pa_strerror(e); }
};
#endif


class SoundBase {
protected:
	int		sample_frequency;
	int		sample_converter;
	int		txppm;
	int		rxppm;

// for interface to the samplerate resampling library
	SRC_STATE	*tx_src_state;
	SRC_DATA	*tx_src_data;
	SRC_STATE	*rx_src_state;
	SRC_DATA	*rx_src_data;
	float		*snd_buffer;
	float		*src_buffer;

#if USE_SNDFILE
	SNDFILE* ofCapture;
	SNDFILE* ifPlayback;
	SNDFILE* ofGenerate;
#endif

	bool	capture;
	bool	playback;
	bool	generate;

	void writeGenerate(double *buff, size_t count);
	void writeCapture(double *buff, size_t count);
	int  readPlayback(double *buff, size_t count);
#if USE_SNDFILE
	bool format_supported(int format);
	void tag_file(SNDFILE *sndfile, const char *title);
#endif
public:
	SoundBase();
	virtual ~SoundBase();
	virtual int	Open(int mode, int freq = 8000) = 0;
	virtual void    Close() = 0;
	virtual size_t	Write(double *, size_t) = 0;
	virtual size_t	Write_stereo(double *, double *, size_t) = 0;
	virtual size_t	Read(double *, size_t) = 0;
	virtual bool	must_close(void) = 0;
#if USE_SNDFILE
	void		get_file_params(const char* def_fname, char** fname, int* format);
	int		Capture(bool val);
	int		Playback(bool val);
	int		Generate(bool val);
#endif
	static int	get_converter(const char* name);
};


#if USE_OSS

class SoundOSS : public SoundBase {
private:
	std::string	device;
	int		device_fd;
	int		version;
	int		capability_mask;
	int		format_mask;
	int		channels;
	int		play_format;
	int		mode;
	bool	formatok;
	unsigned char	*cbuff;

	void	getVersion();
	void	getCapabilities();
	void	getFormats();
	void	setfragsize();
	void	Channels(int);
	void	Frequency(int);
	void	Format(int);
	int		BufferSize(int);
	bool	wait_till_finished();
	bool	reset_device();

public:
	SoundOSS(const char *dev = "/dev/dsp");
	~SoundOSS();
	int		Open(int mode, int freq = 8000);
	void	Close();
	size_t		Write(double *, size_t);
	size_t		Write_stereo(double *, double *, size_t);
	size_t		Read(double *, size_t);
	bool		must_close(void) { return true; }

private:
	int		Fd() { return device_fd; }
	int		Frequency() { return sample_frequency;};
	int		Version() {return version;};
	int		Capabilities() {return capability_mask;};
	int		Formats() { return format_mask;};
	int		Channels() { return channels;};
	int		Format() { return play_format;};
	bool	FormatOK() { return formatok;};
};

#endif // USE_OSS


#if USE_PORTAUDIO

class SoundPort : public SoundBase
{
public:
        typedef std::vector<const PaDeviceInfo*>::const_iterator device_iterator;
        static void	initialize(void);
        static void	terminate(void);
        static const std::vector<const PaDeviceInfo*>& devices(void);

public:
        SoundPort(const char *in_dev, const char *out_dev);
        ~SoundPort();
	int 		Open(int mode, int freq = 8000);
	void 		Close();
	size_t 		Write(double *buf, size_t count);
	size_t		Write_stereo(double *bufleft, double *bufright, size_t count);
	size_t 		Read(double *buf, size_t count);
	bool		must_close(void);

private:
        void		src_data_reset(int mode);
        void		resample(int mode, float *buf, size_t count, size_t max = 0);
        void 		init_stream(unsigned dir);
        void 		start_stream(unsigned dir);
        void		close_stream(unsigned dir);
        bool		stream_active(unsigned dir);
        bool		full_duplex_device(const PaDeviceInfo* dev);
        double		find_srate(unsigned dir);
        void		pa_perror(int err, const char* str = 0);

private:
        enum { STREAM_IN, STREAM_OUT };
        std::string	device[2];

        static bool                                  pa_init;
        PaStream*                                    stream[2];

        device_iterator				     idev[2];
	static std::vector<const PaDeviceInfo*>      devs;
        PaStreamParameters			     stream_params[2];

        unsigned	frames_per_buffer[2];
        double	 	req_sample_rate;
        double		dev_sample_rate[2];
        float 		*fbuf;
};

#endif // USE_PORTAUDIO

#if USE_PULSEAUDIO

class SoundPulse : public SoundBase
{
public:
	SoundPulse(const char* dev);
	virtual ~SoundPulse();

	int	Open(int mode, int freq = 8000);
	void    Close(void);
	size_t	Write(double* buf, size_t count);
	size_t	Write_stereo(double* bufleft, double* bufright, size_t count);
	size_t	Read(double *buf, size_t count);
	bool	must_close(void) { return false; }

private:
	void	src_data_reset(int mode);
        void	resample(int mode, float *buf, size_t count, size_t max = 0);

private:
	double		dev_sample_rate[2];
	pa_simple*	stream[2];
	pa_sample_spec	stream_params;

	float* fbuf;
};

#endif // USE_PULSEAUDIO


class SoundNull : public SoundBase
{
public:
	int	Open(int mode, int freq = 8000) { sample_frequency = freq; return 0; }
	void    Close(void) { }
	size_t	Write(double* buf, size_t count);
	size_t	Write_stereo(double* bufleft, double* bufright, size_t count);
	size_t	Read(double *buf, size_t count);
	bool	must_close(void) { return false; }
};

#endif // SOUND_H
