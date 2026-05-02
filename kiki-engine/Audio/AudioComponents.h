#pragma once
#include <string>
#include <miniaudio.h> 

namespace Kiki {


    struct AudioListenerComponent {
        bool active = true; 
    };


    struct AudioSourceComponent {
        std::string clipPath;     


        float volume = 1.0f;         
        float pitch = 1.0f;         
        bool isLooping = false;      
        bool playOnAwake = true; 


        //Inverse Tapering
        float minDistance = 1.0f;    
        float maxDistance = 20.0f; 

        ma_sound* soundHandle = nullptr;
    };

}