#ifndef _AUDIOCONTROLLER_H_
# define _AUDIOCONTROLLER_H_

# include "IAudio.hh"
# include "AudioCodec.h"
# include "AudioInput.h"
# include "AudioOutput.h"

class AudioController
{
public:
	AudioController();
	~AudioController();

private:
	AudioController(const AudioController &) {};
	const AudioController &operator=(const AudioController &) {return *this;};

public:
	Sound::Encoded	SoundEvent();
	void			player(const Sound::Encoded &);

public:
	void	startPlay();
	void	stopPlay();
	void	startRecord();
	void	stopRecord();

private:
	IAudio		*inputD;
	IAudio		*outputD;
	ICodec		*codec;
};


#endif
