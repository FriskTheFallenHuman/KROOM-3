/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans
Copyright (c) 2010 by Chris Robinson <chris.kcat@gmail.com> (OpenAL Info Utility)
Copyright (C) 2021 George Kalmpokis

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __AL_SOUNDHARDWARE_H__
#define __AL_SOUNDHARDWARE_H__

class idSoundSample_OpenAL;
class idSoundVoice_OpenAL;
class idSoundHardware_OpenAL;

/*
================================================
idSoundHardware_OpenAL
================================================
*/

class idSoundHardware_OpenAL : public idSoundHardware
{
public:
	idSoundHardware_OpenAL();

	void			Init();
	void			Shutdown();
	void			ShutdownReverbSystem();

	void 			Update();
	void			UpdateEAXEffect( idSoundEffect* effect );

	idSoundVoice* 	AllocateVoice( const idSoundSample* leadinSample, const idSoundSample* loopingSample, const int channel ); // GK: Get the sound channel in order to filter which sound will use the Room's reverb and which the default
	void			FreeVoice( idSoundVoice* voice );

	// listDevices needs this
	ALCdevice* 		GetOpenALDevice() const
	{
		return openalDevice;
	};

	int				GetNumZombieVoices() const
	{
		return zombieVoices.Num();
	}
	int				GetNumFreeVoices() const
	{
		return freeVoices.Num();
	}

	bool			IsReverbSupported()
	{
		return hasEFX;
	}

	// OpenAL info
	static void		PrintDeviceList( const char* list );
	static void		PrintALCInfo( ALCdevice* device );
	static void		PrintALInfo();
	static void		parseDeviceName( const ALCchar* wcDevice, char* mbDevice );
	void			RestartHardware();
	ALuint slot;
	ALuint voiceslot;
	ALuint voicefilter;


protected:
	friend class idSoundSample_OpenAL;
	friend class idSoundVoice_OpenAL;

private:
	ALCdevice*			openalDevice;
	ALCcontext*			openalContext;

	// Can't stop and start a voice on the same frame, so we have to double this to handle the worst case scenario of stopping all voices and starting a full new set
	idStaticList<idSoundVoice_OpenAL, MAX_HARDWARE_VOICES * 2 > voices;
	idStaticList<idSoundVoice_OpenAL*, MAX_HARDWARE_VOICES * 2 > zombieVoices;
	idStaticList<idSoundVoice_OpenAL*, MAX_HARDWARE_VOICES * 2 > freeVoices;
	bool				hasEFX;
	ALuint				EAX;
	void SetEFX( EFXEAXREVERBPROPERTIES* rev );
};

#endif /* !__AL_SOUNDHARDWARE_H__ */
