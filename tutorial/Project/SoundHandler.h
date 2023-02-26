//
// Created by User on 2/26/2023.
//cant tie

#ifndef ENGINEREWORK_SOUNDMANAGER_H
#define ENGINEREWORK_SOUNDMANAGER_H

#include <SFML/Audio.hpp>
#include <string>

#define VOLUME 30
using namespace std;
using namespace sf;

class SoundHandler {

public:
    SoundHandler(){}

    void LoadSound(string path){
        Stop();
        soundBuffer.loadFromFile(path);
        sound.setBuffer(soundBuffer);
    }

    void PlayOneShot(){
        sound.setLoop(false);
        sound.play();
    };
    void PlayLooping(){
        sound.setLoop(true);
        sound.play();
    };
    void Play(){
        sound.play();
    }
    void Pause(){
        sound.pause();
    }
    void Stop(){
        sound.stop();
    }
    void Mute(){
        sound.setVolume(0);
    }
    void UnMute(){
        sound.setVolume(VOLUME);
    }



private:
    SoundBuffer soundBuffer;
    Sound sound;
};


#endif //ENGINEREWORK_SOUNDMANAGER_H
