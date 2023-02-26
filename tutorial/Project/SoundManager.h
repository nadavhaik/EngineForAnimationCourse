#include <memory>
#include "SoundHandler.h"


string MUSIC_PATH = "/sounds/";
string BACKGROUND_MUSIC_FILE_NAME = "background_music.wav";

string SOUND_EFFECT_PATH = "/sounds/sound_effects/";

string BUTTON_FILE_NAME = "button_sound_effect.wav";
string WIN_FILE_NAME = "win_sound_effect.wav";
string LOSE_FILE_NAME = "lose_sound_effect.wav";

string PRIZE_FILE_NAME = "prize_sound_effect.wav";
string HIT_FILE_NAME = "hit_sound_effect.wav";

using namespace std;
using namespace sf;

class SoundManager {
public:
//    SoundManager() { InitManager(); }
    void InitManager(){
        snakeSoundHandler = make_shared<SoundHandler>(SoundHandler());
        backgroundSoundHandler = make_shared<SoundHandler>(SoundHandler());
        uiSoundHandler = make_shared<SoundHandler>(SoundHandler());

        PlayBackgroundMusic();  // START PLAYING BG MUSIC RIGHT AWAY
    }

    void PlayHitSoundEffect(){
        snakeSoundHandler->LoadSound(SOUND_EFFECT_PATH + HIT_FILE_NAME);
        snakeSoundHandler->PlayOneShot();
    };
    void PlayPrizeSoundEffect(){
        snakeSoundHandler->LoadSound(SOUND_EFFECT_PATH + PRIZE_FILE_NAME);
        snakeSoundHandler->PlayOneShot();
    };

    void PlayButtonSoundEffect(){
        uiSoundHandler->LoadSound(SOUND_EFFECT_PATH + BUTTON_FILE_NAME);
        uiSoundHandler->PlayOneShot();
    };
    void PlayWinSoundEffect(){
        uiSoundHandler->LoadSound(SOUND_EFFECT_PATH + WIN_FILE_NAME);
        uiSoundHandler->PlayOneShot();
    };
    void PlayLoseSoundEffect(){
        uiSoundHandler->LoadSound(SOUND_EFFECT_PATH + LOSE_FILE_NAME);
        uiSoundHandler->PlayOneShot();
    };

    void MuteAll(){
        snakeSoundHandler->Mute();
        backgroundSoundHandler->Mute();
        uiSoundHandler->Mute();
    }

    void UnMuteAll(){
        snakeSoundHandler->UnMute();
        backgroundSoundHandler->UnMute();
        uiSoundHandler->UnMute();
    }

private:


    void PlayBackgroundMusic(){
        backgroundSoundHandler->LoadSound(MUSIC_PATH + BACKGROUND_MUSIC_FILE_NAME);
        backgroundSoundHandler->PlayLooping();
    };



    shared_ptr<SoundHandler> snakeSoundHandler;
    shared_ptr<SoundHandler> backgroundSoundHandler;
    shared_ptr<SoundHandler> uiSoundHandler;

};